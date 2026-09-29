/*
 * lib/ptk/src/wlapp.c: -- Wayland application driver.
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include "wlapp.h"

#ifdef PTK_HAS_LIBWAYLAND

#include <errno.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <yutil.h>

/** Wake up periodically even without input, to drive animations and timers */
#define WAIT_TIMEOUT_MS 10

ptk_wlapp_t wlapp;

/** One display fd, plus one more when libdecor is in use */
#define MAX_POLL_FDS 2

ptk_window_t *ptk_wlapp_find_window(struct wl_surface *surface)
{
        ptk_window_t *wnd;
        list_node_t *node;

        if (!surface) {
                return NULL;
        }
        for (list_each(node, &wlapp.windows)) {
                wnd = node->data;
                if (wnd->surface == surface) {
                        return wnd;
                }
        }
        return NULL;
}

static void wl_shm_format(void *data, struct wl_shm *shm, uint32_t format)
{
        if (format != WL_SHM_FORMAT_XRGB8888 &&
            format != WL_SHM_FORMAT_ARGB8888) {
                logger_debug("[wl] shm format: %u\n", format);
        }
}

static const struct wl_shm_listener shm_listener = {
        .format = wl_shm_format,
};

static void wl_output_geometry(void *data, struct wl_output *output, int32_t x,
                               int32_t y, int32_t physical_width,
                               int32_t physical_height, int32_t subpixel,
                               const char *make, const char *model,
                               int32_t transform)
{
        (void)data;
        (void)output;
        (void)x;
        (void)y;
        (void)physical_width;
        (void)physical_height;
        (void)subpixel;
        (void)make;
        (void)model;
        (void)transform;
}

static void wl_output_mode(void *data, struct wl_output *output,
                           uint32_t flags, int32_t width, int32_t height,
                           int32_t refresh)
{
        (void)data;
        (void)output;
        (void)refresh;
        if (flags & WL_OUTPUT_MODE_CURRENT) {
                wlapp.screen_width = width;
                wlapp.screen_height = height;
        }
}

static void wl_output_done(void *data, struct wl_output *output)
{
        (void)data;
        (void)output;
        logger_debug("[wl] output size: %dx%d, scale: %d\n",
                     wlapp.screen_width, wlapp.screen_height,
                     wlapp.output_scale);
}

static void wl_output_scale(void *data, struct wl_output *output,
                            int32_t factor)
{
        (void)data;
        (void)output;
        wlapp.output_scale = factor > 0 ? factor : 1;
}

static const struct wl_output_listener output_listener = {
        .geometry = wl_output_geometry,
        .mode = wl_output_mode,
        .done = wl_output_done,
        .scale = wl_output_scale,
};

static void wl_wm_base_ping(void *data, struct xdg_wm_base *wm_base,
                            uint32_t serial)
{
        (void)data;
        xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
        .ping = wl_wm_base_ping,
};

static void wl_registry_global(void *data, struct wl_registry *registry,
                               uint32_t name, const char *interface,
                               uint32_t version)
{
        (void)data;
        if (strcmp(interface, wl_compositor_interface.name) == 0) {
                wlapp.compositor = wl_registry_bind(
                    registry, name, &wl_compositor_interface,
                    version < 4 ? version : 4);
        } else if (strcmp(interface, wl_shm_interface.name) == 0) {
                wlapp.shm = wl_registry_bind(registry, name,
                                             &wl_shm_interface, 1);
                wl_shm_add_listener(wlapp.shm, &shm_listener, NULL);
        } else if (strcmp(interface, wl_seat_interface.name) == 0) {
                wlapp.seat = wl_registry_bind(
                    registry, name, &wl_seat_interface,
                    version < 5 ? version : 5);
        } else if (strcmp(interface, wl_output_interface.name) == 0) {
                wlapp.output = wl_registry_bind(
                    registry, name, &wl_output_interface,
                    version < 3 ? version : 3);
                wl_output_add_listener(wlapp.output, &output_listener, NULL);
        } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
                wlapp.wm_base = wl_registry_bind(
                    registry, name, &xdg_wm_base_interface, 1);
                xdg_wm_base_add_listener(wlapp.wm_base, &wm_base_listener,
                                         NULL);
        }
}

static void wl_registry_global_remove(void *data,
                                      struct wl_registry *registry,
                                      uint32_t name)
{
        (void)data;
        (void)registry;
        (void)name;
}

static const struct wl_registry_listener registry_listener = {
        .global = wl_registry_global,
        .global_remove = wl_registry_global_remove,
};

#ifdef PTK_HAS_LIBDECOR

static void wl_decor_error(struct libdecor *context, enum libdecor_error error,
                           const char *message)
{
        (void)context;
        logger_error("[wl] libdecor error %d: %s\n", (int)error,
                     message ? message : "");
}

static struct libdecor_interface decor_interface = {
        .error = wl_decor_error,
};

#endif

static int ptk_wlapp_poll(int timeout_ms)
{
        int i, n = 0, ret;
        struct pollfd fds[MAX_POLL_FDS];

        fds[0].fd = wl_display_get_fd(wlapp.display);
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        ++n;
#ifdef PTK_HAS_LIBDECOR
        if (wlapp.decor) {
                int fd = libdecor_get_fd(wlapp.decor);

                if (fd >= 0 && fd != fds[0].fd) {
                        fds[n].fd = fd;
                        fds[n].events = POLLIN;
                        fds[n].revents = 0;
                        ++n;
                }
        }
#endif
        ret = poll(fds, n, timeout_ms);
        if (ret <= 0) {
                return ret;
        }
        for (i = 0; i < n; ++i) {
                if (fds[i].revents & (POLLERR | POLLHUP)) {
                        return -1;
                }
        }
        return ret;
}

/**
 * Wait for and process events.
 * Without libdecor this follows the usual wl_display
 * prepare_read/read_events sequence.  With libdecor, libdecor_dispatch()
 * reads the events instead: as libdecor.h puts it, its dispatch function
 * should be called once the fd returned by libdecor_get_fd() is readable.
 */
static int ptk_wlapp_wait(int timeout_ms)
{
        struct pollfd pfd;
        int ret;

#ifdef PTK_HAS_LIBDECOR
        if (wlapp.decor) {
                if (wl_display_flush(wlapp.display) < 0 &&
                    errno != EAGAIN) {
                        return -1;
                }
                if (ptk_wlapp_poll(timeout_ms) > 0) {
                        ret = libdecor_dispatch(wlapp.decor, 0);
                        if (ret < 0) {
                                return -1;
                        }
                }
                return wl_display_dispatch_pending(wlapp.display);
        }
#endif
        while (wl_display_prepare_read(wlapp.display) != 0) {
                if (wl_display_dispatch_pending(wlapp.display) < 0) {
                        wl_display_cancel_read(wlapp.display);
                        return -1;
                }
        }
        if (wl_display_flush(wlapp.display) < 0 && errno != EAGAIN) {
                wl_display_cancel_read(wlapp.display);
                return -1;
        }
        pfd.fd = wl_display_get_fd(wlapp.display);
        pfd.events = POLLIN;
        pfd.revents = 0;
        if (poll(&pfd, 1, timeout_ms) > 0 && (pfd.revents & POLLIN)) {
                if (wl_display_read_events(wlapp.display) < 0) {
                        return -1;
                }
        } else {
                wl_display_cancel_read(wlapp.display);
        }
        return wl_display_dispatch_pending(wlapp.display);
}

static int ptk_wlapp_process_events(ptk_process_events_option_t option)
{
        int ret = 0;

        wlapp.exit_code = 0;
        if (option == PTK_PROCESS_EVENTS_ONE_IF_PRESENT ||
            option == PTK_PROCESS_EVENTS_ALL_IF_PRESENT) {
                /*
                 * Process whatever is already queued and return; never
                 * loop here.  ui_server_connect() calls this once and
                 * expects it to come back, otherwise initialisation would
                 * spin forever inside it.
                 */
                ptk_tick();
                ptk_wlapp_wait(0);
                ptk_process_events();
                return wlapp.exit_code;
        }
        do {
                ptk_tick();
                ptk_process_events();
                ret = ptk_wlapp_wait(WAIT_TIMEOUT_MS);
                if (ret < 0) {
                        break;
                }
                ptk_process_events();
        } while (option == PTK_PROCESS_EVENTS_UNTIL_QUIT && !wlapp.should_quit);
        return wlapp.exit_code;
}

static int ptk_wlapp_add_native_event_listener(
    int type, ptk_native_event_handler_t handler, void *data)
{
        ptk_native_event_listener_t *listener;

        listener = malloc(sizeof(ptk_native_event_listener_t));
        if (!listener) {
                return -1;
        }
        listener->handler = handler;
        listener->data = data;
        listener->type = type;
        list_append(&wlapp.listeners, listener);
        return 0;
}

static int ptk_wlapp_remove_native_event_listener(
    int type, ptk_native_event_handler_t handler)
{
        ptk_native_event_listener_t *listener;
        list_node_t *node, *prev;

        for (list_each(node, &wlapp.listeners)) {
                prev = node->prev;
                listener = node->data;
                if (listener->handler == handler && listener->type == type) {
                        list_delete_node(&wlapp.listeners, node);
                        free(listener);
                        node = prev;
                        return 0;
                }
        }
        return -1;
}

static int ptk_wlapp_get_screen_width(void)
{
        int scale = wlapp.output_scale > 0 ? wlapp.output_scale : 1;

        return wlapp.screen_width / scale;
}

static int ptk_wlapp_get_screen_height(void)
{
        int scale = wlapp.output_scale > 0 ? wlapp.output_scale : 1;

        return wlapp.screen_height / scale;
}

static int ptk_wlapp_init(const wchar_t *name)
{
        (void)name;
        wlapp.display = wl_display_connect(NULL);
        if (!wlapp.display) {
                return -1;
        }
        wlapp.output_scale = 1;
        list_create(&wlapp.windows);
        list_create(&wlapp.listeners);
        wlapp.registry = wl_display_get_registry(wlapp.display);
        wl_registry_add_listener(wlapp.registry, &registry_listener, NULL);
        /* The first round trip binds the globals, the second one delivers the
           wl_output size and scale */
        if (wl_display_roundtrip(wlapp.display) < 0 ||
            wl_display_roundtrip(wlapp.display) < 0) {
                return -2;
        }
        if (!wlapp.compositor || !wlapp.shm || !wlapp.wm_base) {
                logger_error("[wl] missing required globals (compositor=%p "
                             "shm=%p xdg_wm_base=%p)\n",
                             (void *)wlapp.compositor, (void *)wlapp.shm,
                             (void *)wlapp.wm_base);
                return -3;
        }
        if (!wlapp.seat) {
                logger_error("[wl] the compositor provides no seat, the "
                             "application cannot receive input\n");
        }
        if (!wlapp.screen_width || !wlapp.screen_height) {
                logger_error("[wl] failed to get the output size\n");
                return -4;
        }
        if (wlapp.output_scale != 1) {
                logger_error("[wl] output scale %d is not supported yet, "
                             "the window will be blurry\n",
                             wlapp.output_scale);
        }
#ifdef PTK_HAS_LIBDECOR
        wlapp.decor = libdecor_new(wlapp.display, &decor_interface);
        if (!wlapp.decor) {
                logger_debug("[wl] libdecor is unavailable, the window will "
                             "have no decorations\n");
        }
#endif
        ptk_wlinput_init();
        wlapp.active = true;
        wlapp.should_quit = false;
        return 0;
}

static int ptk_wlapp_destroy(void)
{
        if (!wlapp.active) {
                return -1;
        }
        wlapp.active = false;
        ptk_wlinput_destroy();
#ifdef PTK_HAS_LIBDECOR
        if (wlapp.decor) {
                libdecor_unref(wlapp.decor);
                wlapp.decor = NULL;
        }
#endif
        list_destroy_without_node(
            &wlapp.windows, (list_item_destructor_t)ptk_wlwindow_destroy);
        list_destroy(&wlapp.listeners, free);
        if (wlapp.output) {
                wl_output_destroy(wlapp.output);
        }
        if (wlapp.seat) {
                wl_seat_destroy(wlapp.seat);
        }
        if (wlapp.wm_base) {
                xdg_wm_base_destroy(wlapp.wm_base);
        }
        if (wlapp.shm) {
                wl_shm_destroy(wlapp.shm);
        }
        if (wlapp.compositor) {
                wl_compositor_destroy(wlapp.compositor);
        }
        wl_registry_destroy(wlapp.registry);
        wl_display_disconnect(wlapp.display);
        wlapp.display = NULL;
        return 0;
}

static void ptk_wlapp_present(void)
{
        ptk_wlwindow_present_all();
}

static void ptk_wlapp_exit(int exit_code)
{
        wlapp.exit_code = exit_code;
        wlapp.should_quit = true;
}

void ptk_wlapp_driver_init(ptk_app_driver_t *driver)
{
        driver->init = ptk_wlapp_init;
        driver->destroy = ptk_wlapp_destroy;
        driver->process_events = ptk_wlapp_process_events;
        driver->on_event = ptk_wlapp_add_native_event_listener;
        driver->off_event = ptk_wlapp_remove_native_event_listener;
        driver->get_screen_width = ptk_wlapp_get_screen_width;
        driver->get_screen_height = ptk_wlapp_get_screen_height;
        driver->create_window = ptk_wlwindow_create;
        driver->get_window = ptk_wlwindow_get_by_handle;
        driver->present = ptk_wlapp_present;
        driver->exit = ptk_wlapp_exit;
}

#endif
