/*
 * lib/ptk/src/macos/app.c
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <string.h>
#include "ptk.h"
#include "app.h"

static struct macos_app {
        bool active;
        ptk_window_driver_t window;
        ptk_app_driver_t app;
} macos_app;

int ptk_process_native_events(ptk_process_events_option_t option)
{
        return macos_app.app.process_events(option);
}

int ptk_add_native_event_listener(int type, ptk_native_event_handler_t handler,
                                  void *data)
{
        return macos_app.app.on_event(type, handler, data);
}

int ptk_remove_native_event_listener(int type,
                                     ptk_native_event_handler_t handler)
{
        return macos_app.app.off_event(type, handler);
}

PTK_PUBLIC int ptk_screen_get_width(void)
{
        return macos_app.app.get_screen_width();
}

PTK_PUBLIC int ptk_screen_get_height(void)
{
        return macos_app.app.get_screen_height();
}

ptk_window_t *ptk_find_window(void *handle)
{
        return macos_app.app.get_window(handle);
}

void ptk_window_close(ptk_window_t *wnd)
{
        macos_app.window.close(wnd);
}

void ptk_window_destroy(ptk_window_t *wnd)
{
        macos_app.window.destroy(wnd);
}

void ptk_window_set_fullscreen(ptk_window_t *wnd, bool fullscreen)
{
        ptk_macoswindow_set_fullscreen(wnd, fullscreen);
}

void ptk_window_activate(ptk_window_t *wnd)
{
        macos_app.window.activate(wnd);
}

void ptk_window_set_title(ptk_window_t *wnd, const wchar_t *title)
{
        macos_app.window.set_title(wnd, title);
}

void ptk_window_set_size(ptk_window_t *wnd, int width, int height)
{
        macos_app.window.set_size(wnd, width, height);
}

void ptk_window_set_position(ptk_window_t *wnd, int x, int y)
{
        macos_app.window.set_position(wnd, x, y);
}

void ptk_window_get_position(ptk_window_t *wnd, int *x, int *y)
{
        macos_app.window.get_position(wnd, x, y);
}

void ptk_window_set_maximized(ptk_window_t *wnd, bool maximized)
{
        macos_app.window.set_maximized(wnd, maximized);
}

void *ptk_window_get_handle(ptk_window_t *wnd)
{
        return macos_app.window.get_handle(wnd);
}

unsigned ptk_window_get_dpi(ptk_window_t *wnd)
{
        return ptk_macoswindow_get_dpi(wnd);
}

int ptk_window_get_width(ptk_window_t *wnd)
{
        return macos_app.window.get_width(wnd);
}

int ptk_window_get_height(ptk_window_t *wnd)
{
        return macos_app.window.get_height(wnd);
}

bool ptk_window_is_maximized(ptk_window_t *wnd)
{
        return macos_app.window.is_maximized(wnd);
}

void ptk_window_set_min_width(ptk_window_t *wnd, int min_width)
{
        macos_app.window.set_min_width(wnd, min_width);
}

void ptk_window_set_min_height(ptk_window_t *wnd, int min_height)
{
        macos_app.window.set_min_height(wnd, min_height);
}

void ptk_window_set_max_width(ptk_window_t *wnd, int max_width)
{
        macos_app.window.set_max_width(wnd, max_width);
}

void ptk_window_set_max_height(ptk_window_t *wnd, int max_height)
{
        macos_app.window.set_max_height(wnd, max_height);
}

void ptk_window_show(ptk_window_t *wnd)
{
        macos_app.window.show(wnd);
}

void ptk_window_hide(ptk_window_t *wnd)
{
        macos_app.window.hide(wnd);
}

ptk_window_paint_t *ptk_window_begin_paint(ptk_window_t *wnd, pd_rect_t *rect)
{
        return macos_app.window.begin_paint(wnd, rect);
}

void ptk_window_end_paint(ptk_window_t *wnd, ptk_window_paint_t *paint)
{
        macos_app.window.end_paint(wnd, paint);
}

void ptk_window_present(ptk_window_t *wnd)
{
        macos_app.window.present(wnd);
}

ptk_window_t *ptk_window_create(const wchar_t *title, int x, int y, int width,
                                int height, ptk_window_t *parent)
{
        return macos_app.app.create_window(title, x, y, width, height, parent);
}

void ptk_app_present(void)
{
        macos_app.app.present();
}

void ptk_app_exit(int exit_code)
{
        macos_app.app.exit(exit_code);
}

ptk_app_id_t ptk_get_app_id(void)
{
        return PTK_APP_ID_MACOS;
}

int ptk_app_init(const wchar_t *name)
{
        ptk_macosapp_driver_init(&macos_app.app);
        ptk_macoswindow_driver_init(&macos_app.window);
        if (macos_app.app.init(name) != 0) {
                return -1;
        }
        macos_app.active = true;
        return 0;
}

int ptk_app_destroy(void)
{
        if (!macos_app.active) {
                return -1;
        }
        macos_app.active = false;
        return macos_app.app.destroy();
}
