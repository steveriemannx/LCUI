/*
 * lib/ptk/src/wlwindow.c: -- Wayland window driver.
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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <yutil.h>

#define MIN_WINDOW_WIDTH 100
#define MIN_WINDOW_HEIGHT 40

static void ptk_wlwindow_set_size(ptk_window_t *wnd, int width, int height);
static void ptk_wlwindow_present(ptk_window_t *wnd);

/** Create an anonymous memory block shareable with the compositor */
static int ptk_wl_create_shm_file(size_t size)
{
        char name[64];
        int fd;

        snprintf(name, sizeof(name), "/lcui-wayland-%d", (int)getpid());
        fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd < 0) {
                logger_error("[wl] shm_open failed: %s\n", strerror(errno));
                return -1;
        }
        /* Unlink right away: the fd keeps it alive and it is reclaimed when
           the process exits */
        shm_unlink(name);
        if (ftruncate(fd, (off_t)size) < 0) {
                logger_error("[wl] ftruncate failed: %s\n", strerror(errno));
                close(fd);
                return -1;
        }
        return fd;
}

void ptk_wlwindow_damage_all(ptk_window_t *wnd)
{
        pd_rect_t *rect = malloc(sizeof(pd_rect_t));

        if (!rect) {
                return;
        }
        rect->x = 0;
        rect->y = 0;
        rect->width = wnd->width;
        rect->height = wnd->height;
        list_append(&wnd->pending, rect);
}

static void ptk_wlbuffer_release(void *data, struct wl_buffer *buffer)
{
        ptk_wl_buffer_t *buf = data;

        (void)buffer;
        buf->busy = false;
}

static const struct wl_buffer_listener buffer_listener = {
        .release = ptk_wlbuffer_release,
};

static void ptk_wlwindow_free_buffers(ptk_window_t *wnd)
{
        int i;

        for (i = 0; i < wnd->buffer_count; ++i) {
                if (wnd->buffers[i].handle) {
                        wl_buffer_destroy(wnd->buffers[i].handle);
                        wnd->buffers[i].handle = NULL;
                }
                wnd->buffers[i].busy = false;
                wnd->buffers[i].data = NULL;
        }
        wnd->buffer_count = 0;
        if (wnd->pool_data) {
                munmap(wnd->pool_data, wnd->pool_size);
                wnd->pool_data = NULL;
                wnd->pool_size = 0;
        }
}

/**
 * Rebuild the shared memory buffer pool for the current window size.
 * The compositor holds its own copy of the pool fd, so unmapping here does
 * not disturb what it is currently displaying.
 */
static int ptk_wlwindow_create_buffers(ptk_window_t *wnd)
{
        int i, fd, stride;
        size_t size, total;
        unsigned char *data;
        struct wl_shm_pool *pool;

        ptk_wlwindow_free_buffers(wnd);
        if (wnd->width < 1 || wnd->height < 1 || !wlapp.shm) {
                return -1;
        }
        stride = wnd->width * 4;
        size = (size_t)stride * wnd->height;
        total = size * PTK_WL_MAX_BUFFERS;
        fd = ptk_wl_create_shm_file(total);
        if (fd < 0) {
                return -2;
        }
        data = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (data == MAP_FAILED) {
                logger_error("[wl] mmap failed: %s\n", strerror(errno));
                close(fd);
                return -3;
        }
        memset(data, 0, total);
        pool = wl_shm_create_pool(wlapp.shm, fd, (int32_t)total);
        for (i = 0; i < PTK_WL_MAX_BUFFERS; ++i) {
                wnd->buffers[i].window = wnd;
                wnd->buffers[i].busy = false;
                wnd->buffers[i].data = data + size * i;
                wnd->buffers[i].handle = wl_shm_pool_create_buffer(
                    pool, (int32_t)(size * i), wnd->width, wnd->height,
                    stride, WL_SHM_FORMAT_XRGB8888);
                wl_buffer_add_listener(wnd->buffers[i].handle,
                                       &buffer_listener, &wnd->buffers[i]);
        }
        wnd->buffer_count = PTK_WL_MAX_BUFFERS;
        /* The buffers exist now, the pool object itself can go away */
        wl_shm_pool_destroy(pool);
        wnd->pool_data = data;
        wnd->pool_size = total;
        close(fd);
        return 0;
}

static ptk_wl_buffer_t *ptk_wlwindow_get_free_buffer(ptk_window_t *wnd)
{
        int i;

        for (i = 0; i < wnd->buffer_count; ++i) {
                if (!wnd->buffers[i].busy) {
                        return &wnd->buffers[i];
                }
        }
        return NULL;
}

static void ptk_wlwindow_on_configure(ptk_window_t *wnd, int width, int height,
                                      bool maximized)
{
        ptk_event_t e = { 0 };
        bool first = !wnd->configured;
        bool resized = false;

        wnd->maximized = maximized;
        wnd->configured = true;
        /* The compositor chose a different size: recreate the canvas with it */
        if (width >= 1 && height >= 1 &&
            (width != wnd->width || height != wnd->height)) {
                ptk_wlwindow_set_size(wnd, width, height);
                resized = true;
        }
        /*
         * The widget tree learns the window size from this event alone, so
         * the first one always goes out.  Later ones only when the size
         * really changed, otherwise every configure would repaint the whole
         * window.
         */
        if (!first && !resized) {
                return;
        }
        e.type = PTK_EVENT_SIZE;
        e.window = wnd;
        e.size.width = wnd->width;
        e.size.height = wnd->height;
        ptk_post_event(&e);
}

static void ptk_wlwindow_post_close(ptk_window_t *wnd)
{
        ptk_event_t e = { 0 };

        e.type = PTK_EVENT_CLOSE;
        e.window = wnd;
        ptk_post_event(&e);
}

/*
 * The xdg-shell listeners are always compiled: even with libdecor enabled
 * at build time it can be unavailable at run time, and the window then
 * falls back to a plain, undecorated xdg_toplevel.
 */
static void xdg_surface_configure(void *data, struct xdg_surface *surface,
                                  uint32_t serial)
{
        ptk_window_t *wnd = data;

        xdg_surface_ack_configure(surface, serial);
        ptk_wlwindow_on_configure(wnd, wnd->configure_pending_width,
                                  wnd->configure_pending_height,
                                  wnd->maximized);
}

static const struct xdg_surface_listener xdg_surface_listener = {
        .configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                                   int32_t width, int32_t height,
                                   struct wl_array *states)
{
        ptk_window_t *wnd = data;
        uint32_t *state;

        (void)toplevel;
        wnd->maximized = false;
        wl_array_for_each(state, states) {
                if (*state == XDG_TOPLEVEL_STATE_MAXIMIZED) {
                        wnd->maximized = true;
                }
        }
        /*
         * A size of 0 means the client may choose; keep the current size
         * in that case so the window is not shrunk to nothing.
         */
        wnd->configure_pending_width = width > 0 ? width : wnd->width;
        wnd->configure_pending_height = height > 0 ? height : wnd->height;
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
        (void)toplevel;
        ptk_wlwindow_post_close(data);
}

static void xdg_toplevel_configure_bounds(void *data,
                                          struct xdg_toplevel *toplevel,
                                          int32_t width, int32_t height)
{
        (void)data;
        (void)toplevel;
        (void)width;
        (void)height;
}

static void xdg_toplevel_wm_capabilities(void *data,
                                         struct xdg_toplevel *toplevel,
                                         struct wl_array *capabilities)
{
        (void)data;
        (void)toplevel;
        (void)capabilities;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
        .configure = xdg_toplevel_configure,
        .close = xdg_toplevel_close,
        .configure_bounds = xdg_toplevel_configure_bounds,
        .wm_capabilities = xdg_toplevel_wm_capabilities,
};

#ifdef PTK_HAS_LIBDECOR

static void wl_decor_frame_configure(struct libdecor_frame *frame,
                                     struct libdecor_configuration *config,
                                     void *data)
{
        ptk_window_t *wnd = data;
        int width, height;
        bool maximized = false;
        enum libdecor_window_state state;
        struct libdecor_state *frame_state;

        if (!libdecor_configuration_get_content_size(config, frame, &width,
                                                     &height)) {
                width = wnd->width;
                height = wnd->height;
        }
        if (libdecor_configuration_get_window_state(
                config, &state)) {
                maximized = state & LIBDECOR_WINDOW_STATE_MAXIMIZED;
        }
        frame_state = libdecor_state_new(width, height);
        libdecor_frame_commit(frame, frame_state, config);
        libdecor_state_free(frame_state);
        ptk_wlwindow_on_configure(wnd, width, height, maximized);
}

static void wl_decor_frame_close(struct libdecor_frame *frame, void *data)
{
        (void)frame;
        ptk_wlwindow_post_close(data);
}

static void wl_decor_frame_commit(struct libdecor_frame *frame, void *data)
{
        ptk_window_t *wnd = data;

        (void)frame;
        /* The decoration wants the main surface redrawn; mark it damaged and let
           the next frame submit it */
        ptk_wlwindow_damage_all(wnd);
}

static void wl_decor_frame_dismiss_popup(struct libdecor_frame *frame,
                                         const char *seat_name, void *data)
{
        (void)frame;
        (void)seat_name;
        (void)data;
}

static struct libdecor_frame_interface decor_frame_interface = {
        .configure = wl_decor_frame_configure,
        .close = wl_decor_frame_close,
        .commit = wl_decor_frame_commit,
        .dismiss_popup = wl_decor_frame_dismiss_popup,
};

#endif

static void ptk_wlwindow_apply_title(ptk_window_t *wnd)
{
        char *utf8;
        size_t len;

        if (!wnd->title) {
                return;
        }
        len = encode_utf8(NULL, wnd->title, 0) + 1;
        utf8 = malloc(sizeof(char) * len);
        if (!utf8) {
                return;
        }
        encode_utf8(utf8, wnd->title, len);
#ifdef PTK_HAS_LIBDECOR
        if (wnd->decor_frame) {
                libdecor_frame_set_title(wnd->decor_frame, utf8);
        } else
#endif
            if (wnd->xdg_toplevel) {
                xdg_toplevel_set_title(wnd->xdg_toplevel, utf8);
        }
        free(utf8);
}

ptk_window_t *ptk_wlwindow_create(const wchar_t *title, int x, int y,
                                  int width, int height, ptk_window_t *parent)
{
        ptk_window_t *wnd;

        (void)parent;
        if (!wlapp.compositor || !wlapp.wm_base) {
                return NULL;
        }
        wnd = calloc(1, sizeof(ptk_window_t));
        if (!wnd) {
                return NULL;
        }
        wnd->node.data = wnd;
        wnd->x = x;
        wnd->y = y;
        wnd->width = 0;
        wnd->height = 0;
        pd_canvas_init(&wnd->fb);
        wnd->fb.color_type = PD_COLOR_TYPE_ARGB;
        list_create(&wnd->rects);
        list_create(&wnd->pending);
        wnd->surface = wl_compositor_create_surface(wlapp.compositor);
        if (!wnd->surface) {
                goto fail;
        }
#ifdef PTK_HAS_LIBDECOR
        if (wlapp.decor) {
                wnd->decor_frame = libdecor_decorate(
                    wlapp.decor, wnd->surface, &decor_frame_interface, wnd);
                if (!wnd->decor_frame) {
                        goto fail;
                }
                libdecor_frame_set_app_id(wnd->decor_frame, "lcui");
        } else
#endif
        {
                wnd->xdg_surface = xdg_wm_base_get_xdg_surface(
                    wlapp.wm_base, wnd->surface);
                if (!wnd->xdg_surface) {
                        goto fail;
                }
                xdg_surface_add_listener(wnd->xdg_surface,
                                         &xdg_surface_listener, wnd);
                wnd->xdg_toplevel =
                    xdg_surface_get_toplevel(wnd->xdg_surface);
                if (!wnd->xdg_toplevel) {
                        goto fail;
                }
                xdg_toplevel_add_listener(wnd->xdg_toplevel,
                                          &xdg_toplevel_listener, wnd);
                xdg_toplevel_set_app_id(wnd->xdg_toplevel, "lcui");
        }
        wnd->title = title ? y_wcsdup(title) : NULL;
        ptk_wlwindow_apply_title(wnd);
        list_append_node(&wlapp.windows, &wnd->node);
        ptk_wlwindow_set_size(wnd, width, height);
        return wnd;

fail:
        list_destroy(&wnd->rects, free);
        list_destroy(&wnd->pending, free);
        if (wnd->xdg_toplevel) {
                xdg_toplevel_destroy(wnd->xdg_toplevel);
        }
        if (wnd->xdg_surface) {
                xdg_surface_destroy(wnd->xdg_surface);
        }
        if (wnd->surface) {
                wl_surface_destroy(wnd->surface);
        }
        free(wnd);
        return NULL;
}

ptk_window_t *ptk_wlwindow_get_by_handle(void *handle)
{
        return ptk_wlapp_find_window((struct wl_surface *)handle);
}

void ptk_wlwindow_destroy(ptk_window_t *wnd)
{
        if (!wnd) {
                return;
        }
        list_unlink(&wlapp.windows, &wnd->node);
        if (wlapp.pointer_window == wnd) {
                wlapp.pointer_window = NULL;
        }
        if (wlapp.focused_window == wnd) {
                wlapp.focused_window = NULL;
        }
        ptk_wlwindow_free_buffers(wnd);
        list_destroy(&wnd->rects, free);
        list_destroy(&wnd->pending, free);
        pd_canvas_destroy(&wnd->fb);
        free(wnd->title);
#ifdef PTK_HAS_LIBDECOR
        if (wnd->decor_frame) {
                libdecor_frame_unref(wnd->decor_frame);
        }
#endif
        if (wnd->xdg_toplevel) {
                xdg_toplevel_destroy(wnd->xdg_toplevel);
        }
        if (wnd->xdg_surface) {
                xdg_surface_destroy(wnd->xdg_surface);
        }
        if (wnd->surface) {
                wl_surface_destroy(wnd->surface);
        }
        free(wnd);
}

static void ptk_wlwindow_set_size(ptk_window_t *wnd, int width, int height)
{
        width = y_max(MIN_WINDOW_WIDTH, width);
        height = y_max(MIN_WINDOW_HEIGHT, height);
        if (width == wnd->width && height == wnd->height &&
            wnd->buffer_count > 0) {
                return;
        }
        wnd->width = width;
        wnd->height = height;
        pd_canvas_create(&wnd->fb, width, height);
        ptk_wlwindow_create_buffers(wnd);
        ptk_wlwindow_damage_all(wnd);
}

static void ptk_wlwindow_set_position(ptk_window_t *wnd, int x, int y)
{
        /*
         * Wayland gives clients no say in the global window position, so
         * the value is only recorded here; the compositor decides where the
         * window actually shows up.
         */
        wnd->x = x;
        wnd->y = y;
}

static void ptk_wlwindow_get_position(ptk_window_t *wnd, int *x, int *y)
{
        if (x) {
                *x = wnd->x;
        }
        if (y) {
                *y = wnd->y;
        }
}

static void ptk_wlwindow_set_title(ptk_window_t *wnd, const wchar_t *title)
{
        if (!title) {
                return;
        }
        free(wnd->title);
        wnd->title = y_wcsdup(title);
        ptk_wlwindow_apply_title(wnd);
}

/** Only forward the hints once both dimensions are set, to avoid an
 * invalid min > max state */
static void ptk_wlwindow_apply_size_hints(ptk_window_t *wnd)
{
#ifdef PTK_HAS_LIBDECOR
        if (wnd->decor_frame) {
                if (wnd->min_width > 0 && wnd->min_height > 0) {
                        libdecor_frame_set_min_content_size(
                            wnd->decor_frame, wnd->min_width,
                            wnd->min_height);
                }
                if (wnd->max_width > 0 && wnd->max_height > 0) {
                        libdecor_frame_set_max_content_size(
                            wnd->decor_frame, wnd->max_width,
                            wnd->max_height);
                }
                return;
        }
#endif
        if (!wnd->xdg_toplevel) {
                return;
        }
        if (wnd->min_width > 0 && wnd->min_height > 0) {
                xdg_toplevel_set_min_size(wnd->xdg_toplevel, wnd->min_width,
                                          wnd->min_height);
        }
        if (wnd->max_width > 0 && wnd->max_height > 0) {
                xdg_toplevel_set_max_size(wnd->xdg_toplevel, wnd->max_width,
                                          wnd->max_height);
        }
}

static void ptk_wlwindow_set_min_width(ptk_window_t *wnd, int min_width)
{
        wnd->min_width = min_width;
        ptk_wlwindow_apply_size_hints(wnd);
}

static void ptk_wlwindow_set_min_height(ptk_window_t *wnd, int min_height)
{
        wnd->min_height = min_height;
        ptk_wlwindow_apply_size_hints(wnd);
}

static void ptk_wlwindow_set_max_width(ptk_window_t *wnd, int max_width)
{
        wnd->max_width = max_width;
        ptk_wlwindow_apply_size_hints(wnd);
}

static void ptk_wlwindow_set_max_height(ptk_window_t *wnd, int max_height)
{
        wnd->max_height = max_height;
        ptk_wlwindow_apply_size_hints(wnd);
}

static void ptk_wlwindow_set_maximized(ptk_window_t *wnd, bool maximized)
{
#ifdef PTK_HAS_LIBDECOR
        if (wnd->decor_frame) {
                if (maximized) {
                        libdecor_frame_set_maximized(wnd->decor_frame);
                } else {
                        libdecor_frame_unset_maximized(wnd->decor_frame);
                }
                return;
        }
#endif
        if (wnd->xdg_toplevel) {
                if (maximized) {
                        xdg_toplevel_set_maximized(wnd->xdg_toplevel);
                } else {
                        xdg_toplevel_unset_maximized(wnd->xdg_toplevel);
                }
        }
}

static bool ptk_wlwindow_is_maximized(ptk_window_t *wnd)
{
        return wnd->maximized;
}

static int ptk_wlwindow_get_width(ptk_window_t *wnd)
{
        return wnd->width;
}

static int ptk_wlwindow_get_height(ptk_window_t *wnd)
{
        return wnd->height;
}

static void *ptk_wlwindow_get_handle(ptk_window_t *wnd)
{
        return wnd->surface;
}

static void ptk_wlwindow_show(ptk_window_t *wnd)
{
        wnd->visible = true;
#ifdef PTK_HAS_LIBDECOR
        if (wnd->decor_frame) {
                /* libdecor sends the configure after mapping, no buffer-less commit first */
                libdecor_frame_map(wnd->decor_frame);
                return;
        }
#endif
        if (!wnd->configured) {
                /* The first commit carries no buffer; it is what triggers configure */
                wl_surface_commit(wnd->surface);
                return;
        }
        ptk_wlwindow_damage_all(wnd);
        ptk_wlwindow_present(wnd);
}

static void ptk_wlwindow_hide(ptk_window_t *wnd)
{
        wnd->visible = false;
        wl_surface_attach(wnd->surface, NULL, 0, 0);
        wl_surface_commit(wnd->surface);
}

static void ptk_wlwindow_activate(ptk_window_t *wnd)
{
        wnd->activated = true;
        ptk_wlwindow_show(wnd);
}

static void ptk_wlwindow_close(ptk_window_t *wnd)
{
        ptk_wlwindow_post_close(wnd);
}

static ptk_window_paint_t *ptk_wlwindow_begin_paint(ptk_window_t *wnd,
                                                    pd_rect_t *rect)
{
        pd_context_t *paint;

        paint = malloc(sizeof(pd_context_t));
        if (!paint) {
                return NULL;
        }
        paint->rect = *rect;
        paint->with_alpha = false;
        pd_canvas_init(&paint->canvas);
        pd_rect_correct(&paint->rect, wnd->width, wnd->height);
        pd_canvas_quote(&paint->canvas, &wnd->fb, &paint->rect);
        pd_canvas_fill(&paint->canvas, pd_rgb(255, 255, 255));
        return paint;
}

static void ptk_wlwindow_end_paint(ptk_window_t *wnd,
                                   ptk_window_paint_t *paint)
{
        pd_rect_t *rect;

        rect = malloc(sizeof(pd_rect_t));
        if (!rect) {
                free(paint);
                return;
        }
        *rect = paint->rect;
        list_append(&wnd->rects, rect);
        pd_canvas_init(&paint->canvas);
        free(paint);
}

static void ptk_wlwindow_copy_rect(ptk_window_t *wnd, ptk_wl_buffer_t *buf,
                                   const pd_rect_t *rect)
{
        pd_rect_t r = *rect;
        unsigned char *src, *dst;
        int y, stride;

        pd_rect_correct(&r, wnd->width, wnd->height);
        if (r.width < 1 || r.height < 1) {
                return;
        }
        stride = wnd->width * 4;
        src = wnd->fb.bytes + (size_t)r.y * wnd->fb.bytes_per_row +
              (size_t)r.x * 4;
        dst = buf->data + (size_t)r.y * stride + (size_t)r.x * 4;
        for (y = 0; y < r.height; ++y) {
                memcpy(dst, src, (size_t)r.width * 4);
                src += wnd->fb.bytes_per_row;
                dst += stride;
        }
}

static void ptk_wlwindow_present(ptk_window_t *wnd)
{
        list_node_t *node;
        ptk_wl_buffer_t *buf;
        pd_rect_t *rect;

        if (!wnd->configured || !wnd->visible || !wnd->surface) {
                return;
        }
        if (wnd->rects.length > 0) {
                list_concat(&wnd->pending, &wnd->rects);
        }
        if (wnd->pending.length < 1) {
                return;
        }
        buf = ptk_wlwindow_get_free_buffer(wnd);
        if (!buf) {
                /* Every buffer is still held by the compositor: keep the rectangles for
           the next attempt */
                return;
        }
        for (list_each(node, &wnd->pending)) {
                rect = node->data;
                ptk_wlwindow_copy_rect(wnd, buf, rect);
        }
        wl_surface_attach(wnd->surface, buf->handle, 0, 0);
        for (list_each(node, &wnd->pending)) {
                rect = node->data;
                wl_surface_damage(wnd->surface, rect->x, rect->y,
                                  rect->width, rect->height);
        }
        wl_surface_commit(wnd->surface);
        buf->busy = true;
        list_destroy(&wnd->pending, free);
}

void ptk_wlwindow_present_all(void)
{
        ptk_window_t *wnd;
        list_node_t *node;

        for (list_each(node, &wlapp.windows)) {
                wnd = node->data;
                ptk_wlwindow_present(wnd);
        }
}

void ptk_wlwindow_driver_init(ptk_window_driver_t *driver)
{
        driver->show = ptk_wlwindow_show;
        driver->hide = ptk_wlwindow_hide;
        driver->close = ptk_wlwindow_close;
        driver->destroy = ptk_wlwindow_destroy;
        driver->activate = ptk_wlwindow_activate;
        driver->set_title = ptk_wlwindow_set_title;
        driver->set_position = ptk_wlwindow_set_position;
        driver->get_position = ptk_wlwindow_get_position;
        driver->set_size = ptk_wlwindow_set_size;
        driver->set_maximized = ptk_wlwindow_set_maximized;
        driver->get_width = ptk_wlwindow_get_width;
        driver->get_height = ptk_wlwindow_get_height;
        driver->is_maximized = ptk_wlwindow_is_maximized;
        driver->get_handle = ptk_wlwindow_get_handle;
        driver->set_max_width = ptk_wlwindow_set_max_width;
        driver->set_max_height = ptk_wlwindow_set_max_height;
        driver->set_min_width = ptk_wlwindow_set_min_width;
        driver->set_min_height = ptk_wlwindow_set_min_height;
        driver->begin_paint = ptk_wlwindow_begin_paint;
        driver->end_paint = ptk_wlwindow_end_paint;
        driver->present = ptk_wlwindow_present;
}

#endif
