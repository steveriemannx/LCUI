/*
 * lib/ptk/src/wlapp.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef PTK_SRC_WLAPP_H
#define PTK_SRC_WLAPP_H

#include "ptk.h"

#ifdef PTK_HAS_LIBWAYLAND

#include <stdbool.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <pandagl.h>
#include "xdg-shell-client-protocol.h"

#ifdef PTK_HAS_LIBDECOR
#include <libdecor.h>
#endif

/**
 * Shared memory buffers per window.  At least 2 are needed so the next
 * frame can be drawn while one is still on screen.
 */
#define PTK_WL_MAX_BUFFERS 3

typedef struct ptk_wl_buffer {
        struct wl_buffer *handle;
        /** Where this buffer starts inside the shared memory pool */
        unsigned char *data;
        /** Set while the compositor holds the buffer; it must not be written to then */
        bool busy;
        ptk_window_t *window;
} ptk_wl_buffer_t;

struct ptk_window {
        struct wl_surface *surface;
        struct xdg_surface *xdg_surface;
        struct xdg_toplevel *xdg_toplevel;
#ifdef PTK_HAS_LIBDECOR
        struct libdecor_frame *decor_frame;
#endif
        /**
 * The window's persistent canvas: the UI paints here and present()
 * copies it into shared memory.
 */
        pd_canvas_t fb;

        /** list_t<pd_rect_t>, damaged rectangles to hand to the compositor */
        list_t rects;
        /** list_t<pd_rect_t>, damage deferred because every buffer was busy */
        list_t pending;

        struct wl_shm_pool *pool;
        unsigned char *pool_data;
        size_t pool_size;
        ptk_wl_buffer_t buffers[PTK_WL_MAX_BUFFERS];
        int buffer_count;

        int x, y;
        int width, height;
        int min_width, min_height;
        int max_width, max_height;
        /** Pending configure size; 0 means the client may choose */
        int configure_pending_width, configure_pending_height;
        bool maximized;
        bool configured;
        bool visible;
        bool activated;

        wchar_t *title;

        list_node_t node;
};

typedef struct ptk_wlapp {
        struct wl_display *display;
        struct wl_registry *registry;
        struct wl_compositor *compositor;
        struct wl_shm *shm;
        struct wl_seat *seat;
        struct wl_output *output;
        struct xdg_wm_base *wm_base;
#ifdef PTK_HAS_LIBDECOR
        struct libdecor *decor;
#endif

        int screen_width;
        int screen_height;
        int output_scale;

        int exit_code;
        bool should_quit;
        bool active;

        /** Window the pointer is currently over */
        ptk_window_t *pointer_window;
        /** Window holding the keyboard focus */
        ptk_window_t *focused_window;

        /** list_t<ptk_window_t *> */
        list_t windows;

        /** list_t<ptk_native_event_listener_t> */
        list_t listeners;
} ptk_wlapp_t;

extern ptk_wlapp_t wlapp;

void ptk_wlapp_driver_init(ptk_app_driver_t *driver);
void ptk_wlwindow_driver_init(ptk_window_driver_t *driver);
void ptk_wlwindow_present_all(void);
ptk_window_t *ptk_wlapp_find_window(struct wl_surface *surface);

ptk_window_t *ptk_wlwindow_create(const wchar_t *title, int x, int y,
                                  int width, int height, ptk_window_t *parent);
ptk_window_t *ptk_wlwindow_get_by_handle(void *handle);
void ptk_wlwindow_destroy(ptk_window_t *wnd);

void ptk_wlinput_init(void);
void ptk_wlinput_destroy(void);

/** Mark the whole window as damaged; call this after the canvas was recreated */
void ptk_wlwindow_damage_all(ptk_window_t *wnd);

#endif

#endif
