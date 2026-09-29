/*
 * lib/ptk/src/freebsd/app.c
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <string.h>
#include <yutil.h>
#include "ptk.h"
#include "x11app.h"
#include "wlapp.h"

#ifdef PTK_FREEBSD

/** The backend currently in use */
typedef enum {
        BACKEND_NONE,
        BACKEND_WAYLAND,
        BACKEND_X11
} backend_id_t;

static struct app {
        bool active;
        ptk_window_driver_t window;
        ptk_app_driver_t app;
        ptk_app_id_t id;
        backend_id_t backend;
} freebsd_app;

int ptk_process_native_events(ptk_process_events_option_t option)
{
        return freebsd_app.app.process_events(option);
}

int ptk_add_native_event_listener(int type, ptk_native_event_handler_t handler,
                                  void *data)
{
        return freebsd_app.app.on_event(type, handler, data);
}

int ptk_remove_native_event_listener(int type,
                                     ptk_native_event_handler_t handler)
{
        return freebsd_app.app.off_event(type, handler);
}

PTK_PUBLIC int ptk_screen_get_width(void)
{
        return freebsd_app.app.get_screen_width();
}

PTK_PUBLIC int ptk_screen_get_height(void)
{
        return freebsd_app.app.get_screen_height();
}

ptk_window_t *ptk_find_window(void *handle)
{
        return freebsd_app.app.get_window(handle);
}

void ptk_window_close(ptk_window_t *wnd)
{
        return freebsd_app.window.close(wnd);
}

void ptk_window_destroy(ptk_window_t *wnd)
{
        return freebsd_app.window.destroy(wnd);
}

void ptk_window_set_fullscreen(ptk_window_t *wnd, bool fullscreen)
{
        // TODO:
}

void ptk_window_activate(ptk_window_t *wnd)
{
        return freebsd_app.window.activate(wnd);
}

void ptk_window_set_title(ptk_window_t *wnd, const wchar_t *title)
{
        return freebsd_app.window.set_title(wnd, title);
}

void ptk_window_set_size(ptk_window_t *wnd, int width, int height)
{
        return freebsd_app.window.set_size(wnd, width, height);
}

void ptk_window_set_position(ptk_window_t *wnd, int x, int y)
{
        return freebsd_app.window.set_position(wnd, x, y);
}

void ptk_window_get_position(ptk_window_t *wnd, int *x, int *y)
{
        freebsd_app.window.get_position(wnd, x, y);
}

void ptk_window_set_maximized(ptk_window_t *wnd, bool maximized)
{
        freebsd_app.window.set_maximized(wnd, maximized);
}

void *ptk_window_get_handle(ptk_window_t *wnd)
{
        return freebsd_app.window.get_handle(wnd);
}

unsigned ptk_window_get_dpi(ptk_window_t *wnd)
{
        // TODO: get dpi from x11 window ?
        return 96;
}

int ptk_window_get_width(ptk_window_t *wnd)
{
        return freebsd_app.window.get_width(wnd);
}

int ptk_window_get_height(ptk_window_t *wnd)
{
        return freebsd_app.window.get_height(wnd);
}

bool ptk_window_is_maximized(ptk_window_t *wnd)
{
        return freebsd_app.window.is_maximized(wnd);
}

void ptk_window_set_min_width(ptk_window_t *wnd, int min_width)
{
        freebsd_app.window.set_min_width(wnd, min_width);
}

void ptk_window_set_min_height(ptk_window_t *wnd, int min_height)
{
        freebsd_app.window.set_min_height(wnd, min_height);
}

void ptk_window_set_max_width(ptk_window_t *wnd, int max_width)
{
        freebsd_app.window.set_max_width(wnd, max_width);
}

void ptk_window_set_max_height(ptk_window_t *wnd, int max_height)
{
        freebsd_app.window.set_max_height(wnd, max_height);
}

void ptk_window_show(ptk_window_t *wnd)
{
        freebsd_app.window.show(wnd);
}

void ptk_window_hide(ptk_window_t *wnd)
{
        freebsd_app.window.hide(wnd);
}

ptk_window_paint_t *ptk_window_begin_paint(ptk_window_t *wnd, pd_rect_t *rect)
{
        return freebsd_app.window.begin_paint(wnd, rect);
}

void ptk_window_end_paint(ptk_window_t *wnd, ptk_window_paint_t *paint)
{
        freebsd_app.window.end_paint(wnd, paint);
}

void ptk_window_present(ptk_window_t *wnd)
{
        freebsd_app.window.present(wnd);
}

ptk_window_t *ptk_window_create(const wchar_t *title, int x, int y, int width,
                                int height, ptk_window_t *parent)
{
        return freebsd_app.app.create_window(title, x, y, width, height, parent);
}

void ptk_app_present(void)
{
        freebsd_app.app.present();
}

void ptk_app_exit(int exit_code)
{
        freebsd_app.app.exit(exit_code);
}

ptk_app_id_t ptk_get_app_id(void)
{
        return freebsd_app.id;
}

/**
 * Try the backends in the order wayland -> x11.  LCUI_BACKEND may be
 * set to "wayland" or "x11" to force one, which is handy when
 * comparing them.
 */
static bool ptk_app_backend_allowed(const char *backend, const char *name)
{
        return !backend || strcmp(backend, name) == 0;
}

int ptk_app_init(const wchar_t *name)
{
        const char *backend = getenv("LCUI_BACKEND");

        freebsd_app.id = PTK_APP_ID_FREEBSD;
        freebsd_app.backend = BACKEND_NONE;
        (void)backend;
        (void)ptk_app_backend_allowed;
#ifdef PTK_HAS_LIBWAYLAND
        if (ptk_app_backend_allowed(backend, "wayland")) {
                ptk_wlapp_driver_init(&freebsd_app.app);
                ptk_wlwindow_driver_init(&freebsd_app.window);
                if (freebsd_app.app.init(name) == 0) {
                        logger_debug("[app] use engine: wayland\n");
                        freebsd_app.id = PTK_APP_ID_WAYLAND;
                        freebsd_app.backend = BACKEND_WAYLAND;
                        freebsd_app.active = true;
                        return 0;
                }
                memset(&freebsd_app.app, 0, sizeof(freebsd_app.app));
                memset(&freebsd_app.window, 0, sizeof(freebsd_app.window));
        }
#endif
#ifdef PTK_HAS_LIBX11
        if (ptk_app_backend_allowed(backend, "x11")) {
                ptk_x11app_driver_init(&freebsd_app.app);
                ptk_x11window_driver_init(&freebsd_app.window);
                if (freebsd_app.app.init(name) == 0) {
                        logger_debug("[app] use engine: x11app\n");
                        freebsd_app.id = PTK_APP_ID_FREEBSD_X11;
                        freebsd_app.backend = BACKEND_X11;
                        freebsd_app.active = true;
                        return 0;
                }
                memset(&freebsd_app.app, 0, sizeof(freebsd_app.app));
                memset(&freebsd_app.window, 0, sizeof(freebsd_app.window));
        }
#endif
        return -1;
}

int ptk_app_destroy(void)
{
        if (!freebsd_app.active) {
                return -1;
        }
        freebsd_app.active = false;
        return freebsd_app.app.destroy();
}

#endif
