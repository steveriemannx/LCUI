/*
 * lib/ptk/src/wlinput.c: -- Wayland input (pointer and keyboard) support.
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

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <yutil.h>

#define MOUSE_WHEEL_DELTA 20

/* evdev button codes; the input layer is the same on Linux and FreeBSD */
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#define BTN_MIDDLE 0x112

static struct {
        struct wl_pointer *pointer;
        struct wl_keyboard *keyboard;

        struct xkb_context *xkb_context;
        struct xkb_keymap *xkb_keymap;
        struct xkb_state *xkb_state;

        /** Last pointer position; button events carry no coordinates of their own */
        int pointer_x;
        int pointer_y;
} wlinput;

static void ptk_wlinput_post_mouse(ptk_event_type_t type, ptk_window_t *wnd,
                                   int x, int y, int button)
{
        ptk_event_t e = { 0 };

        if (!wnd) {
                return;
        }
        e.type = type;
        e.window = wnd;
        e.mouse.x = x;
        e.mouse.y = y;
        e.mouse.button = button;
        ptk_post_event(&e);
}

static void wl_pointer_enter(void *data, struct wl_pointer *pointer,
                             uint32_t serial, struct wl_surface *surface,
                             wl_fixed_t sx, wl_fixed_t sy)
{
        ptk_window_t *wnd = ptk_wlapp_find_window(surface);

        (void)data;
        wlapp.pointer_window = wnd;
        wlinput.pointer_x = wl_fixed_to_int(sx);
        wlinput.pointer_y = wl_fixed_to_int(sy);
        /*
         * Passing NULL lets the compositor pick the cursor, which is why
         * LCUI does not draw one of its own - the same thing the X11
         * backend gets by calling ui_cursor_hide().
         */
        wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
        ptk_wlinput_post_mouse(PTK_EVENT_MOUSEMOVE, wnd, wlinput.pointer_x,
                               wlinput.pointer_y, 0);
}

static void wl_pointer_leave(void *data, struct wl_pointer *pointer,
                             uint32_t serial, struct wl_surface *surface)
{
        (void)data;
        (void)pointer;
        (void)serial;
        (void)surface;
        wlapp.pointer_window = NULL;
}

static void wl_pointer_motion(void *data, struct wl_pointer *pointer,
                              uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
        ptk_window_t *wnd = wlapp.pointer_window;

        (void)data;
        (void)pointer;
        (void)time;
        wlinput.pointer_x = wl_fixed_to_int(sx);
        wlinput.pointer_y = wl_fixed_to_int(sy);
        ptk_wlinput_post_mouse(PTK_EVENT_MOUSEMOVE, wnd, wlinput.pointer_x,
                               wlinput.pointer_y, 0);
}

static void wl_pointer_button(void *data, struct wl_pointer *pointer,
                              uint32_t serial, uint32_t time, uint32_t button,
                              uint32_t state)
{
        ptk_window_t *wnd = wlapp.pointer_window;
        int code;

        (void)data;
        (void)pointer;
        (void)serial;
        (void)time;
        switch (button) {
        case BTN_RIGHT:
                code = 3;
                break;
        case BTN_MIDDLE:
                code = 2;
                break;
        case BTN_LEFT:
        default:
                code = 1;
                break;
        }
        ptk_wlinput_post_mouse(
            state == WL_POINTER_BUTTON_STATE_PRESSED
                ? PTK_EVENT_MOUSEDOWN
                : PTK_EVENT_MOUSEUP,
            wnd, wlinput.pointer_x, wlinput.pointer_y, code);
}

static void wl_pointer_axis(void *data, struct wl_pointer *pointer,
                            uint32_t time, uint32_t axis, wl_fixed_t value)
{
        ptk_event_t e = { 0 };
        double v = wl_fixed_to_double(value);

        (void)data;
        (void)pointer;
        (void)time;
        if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL || v == 0.0) {
                return;
        }
        e.type = PTK_EVENT_WHEEL;
        e.window = wlapp.pointer_window;
        /* The compositor reports scrolling down as positive, LCUI follows the
           X11 convention of positive being up */
        e.wheel.delta_y = v > 0 ? -MOUSE_WHEEL_DELTA : MOUSE_WHEEL_DELTA;
        ptk_post_event(&e);
}

static void wl_pointer_frame(void *data, struct wl_pointer *pointer)
{
        (void)data;
        (void)pointer;
}

static void wl_pointer_axis_source(void *data, struct wl_pointer *pointer,
                                   uint32_t source)
{
        (void)data;
        (void)pointer;
        (void)source;
}

static void wl_pointer_axis_stop(void *data, struct wl_pointer *pointer,
                                 uint32_t time, uint32_t axis)
{
        (void)data;
        (void)pointer;
        (void)time;
        (void)axis;
}

static void wl_pointer_axis_discrete(void *data, struct wl_pointer *pointer,
                                     uint32_t axis, int32_t discrete)
{
        (void)data;
        (void)pointer;
        (void)axis;
        (void)discrete;
}

static const struct wl_pointer_listener pointer_listener = {
        .enter = wl_pointer_enter,
        .leave = wl_pointer_leave,
        .motion = wl_pointer_motion,
        .button = wl_pointer_button,
        .axis = wl_pointer_axis,
        .frame = wl_pointer_frame,
        .axis_source = wl_pointer_axis_source,
        .axis_stop = wl_pointer_axis_stop,
        .axis_discrete = wl_pointer_axis_discrete,
};

/**
 * Turn a keysym into one of LCUI's key codes.
 * This mirrors convert_keycode() in the X11 backend; xkbcommon names the
 * constants XKB_KEY_* rather than XK_*, but the values are identical.
 */
static int convert_keycode(xkb_keysym_t keysym)
{
        switch (keysym) {
        case XKB_KEY_Tab:
                return KEY_TAB;
        case XKB_KEY_Escape:
                return KEY_ESCAPE;
        case XKB_KEY_Return:
        case XKB_KEY_KP_Enter:
                return KEY_ENTER;
        case XKB_KEY_Delete:
                return KEY_DELETE;
        case XKB_KEY_BackSpace:
                return KEY_BACKSPACE;
        case XKB_KEY_Home:
                return KEY_HOME;
        case XKB_KEY_Left:
                return KEY_LEFT;
        case XKB_KEY_Up:
                return KEY_UP;
        case XKB_KEY_Right:
                return KEY_RIGHT;
        case XKB_KEY_Down:
                return KEY_DOWN;
        case XKB_KEY_Page_Up:
                return KEY_PAGEUP;
        case XKB_KEY_Page_Down:
                return KEY_PAGEDOWN;
        case XKB_KEY_End:
                return KEY_END;
        case XKB_KEY_Insert:
                return KEY_INSERT;
        case XKB_KEY_Control_L:
        case XKB_KEY_Control_R:
                return KEY_CONTROL;
        case XKB_KEY_Shift_L:
        case XKB_KEY_Shift_R:
                return KEY_SHIFT;
        case XKB_KEY_Alt_L:
        case XKB_KEY_Alt_R:
                return KEY_ALT;
        case XKB_KEY_Caps_Lock:
                return KEY_CAPITAL;
        case XKB_KEY_comma:
                return KEY_COMMA;
        case XKB_KEY_period:
                return KEY_PERIOD;
        case XKB_KEY_minus:
                return KEY_MINUS;
        case XKB_KEY_slash:
                return KEY_SLASH;
        case XKB_KEY_semicolon:
                return KEY_SEMICOLON;
        case XKB_KEY_equal:
                return KEY_EQUAL;
        case XKB_KEY_bracketleft:
                return KEY_BRACKETLEFT;
        case XKB_KEY_bracketright:
                return KEY_BRACKETRIGHT;
        case XKB_KEY_backslash:
                return KEY_BACKSLASH;
        case XKB_KEY_apostrophe:
                return KEY_APOSTROPHE;
        case XKB_KEY_grave:
                return KEY_GRAVE;
        default:
                break;
        }
        /* Letters are upper-cased, digits and space keep their value, as in the
           X11 backend */
        if (keysym >= XKB_KEY_a && keysym <= XKB_KEY_z) {
                return (int)(keysym - XKB_KEY_a + 'A');
        }
        return (int)keysym;
}

static bool wl_keyboard_mod_active(const char *name)
{
        if (!wlinput.xkb_state) {
                return false;
        }
        return xkb_state_mod_name_is_active(wlinput.xkb_state, name,
                                            XKB_STATE_MODS_EFFECTIVE) > 0;
}

static void wl_keyboard_post_text(uint32_t key)
{
        char utf8[32];
        wchar_t text[32];
        ptk_event_t e = { 0 };
        int n;

        if (wl_keyboard_mod_active(XKB_MOD_NAME_CTRL) ||
            wl_keyboard_mod_active(XKB_MOD_NAME_ALT)) {
                return;
        }
        n = xkb_state_key_get_utf8(wlinput.xkb_state, key + 8, utf8,
                                   sizeof(utf8));
        if (n < 1) {
                return;
        }
        n = decode_utf8(text, utf8, sizeof(text) / sizeof(wchar_t));
        if (n < 1) {
                return;
        }
        if (ptk_composition_event_init(&e, text, n) != 0) {
                return;
        }
        e.window = wlapp.focused_window;
        ptk_post_event(&e);
        ptk_event_destroy(&e);
}

static void wl_keyboard_key(void *data, struct wl_keyboard *keyboard,
                            uint32_t serial, uint32_t time, uint32_t key,
                            uint32_t state)
{
        ptk_event_t e = { 0 };
        xkb_keysym_t keysym;

        (void)data;
        (void)keyboard;
        (void)serial;
        (void)time;
        if (!wlinput.xkb_state) {
                return;
        }
        keysym = xkb_state_key_get_one_sym(wlinput.xkb_state, key + 8);
        e.type = state == WL_KEYBOARD_KEY_STATE_PRESSED ? PTK_EVENT_KEYDOWN
                                                        : PTK_EVENT_KEYUP;
        e.window = wlapp.focused_window;
        e.key.code = convert_keycode(keysym);
        e.key.shift_key = wl_keyboard_mod_active(XKB_MOD_NAME_SHIFT);
        e.key.ctrl_key = wl_keyboard_mod_active(XKB_MOD_NAME_CTRL);
        e.key.alt_key = wl_keyboard_mod_active(XKB_MOD_NAME_ALT);
        e.key.meta_key = wl_keyboard_mod_active(XKB_MOD_NAME_LOGO);
        ptk_post_event(&e);
        if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                /* No X11 input method to fall back on here, so text is produced directly */
                wl_keyboard_post_text(key);
        }
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *keyboard,
                              uint32_t serial, struct wl_surface *surface,
                              struct wl_array *keys)
{
        (void)data;
        (void)keyboard;
        (void)serial;
        (void)keys;
        wlapp.focused_window = ptk_wlapp_find_window(surface);
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *keyboard,
                              uint32_t serial, struct wl_surface *surface)
{
        (void)data;
        (void)keyboard;
        (void)serial;
        (void)surface;
        wlapp.focused_window = NULL;
}

static void wl_keyboard_keymap(void *data, struct wl_keyboard *keyboard,
                               uint32_t format, int32_t fd, uint32_t size)
{
        char *map;
        struct xkb_keymap *keymap;
        struct xkb_state *state;

        (void)data;
        (void)keyboard;
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
                close(fd);
                return;
        }
        map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (map == MAP_FAILED) {
                close(fd);
                return;
        }
        keymap = xkb_keymap_new_from_string(
            wlinput.xkb_context, map, XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(map, size);
        close(fd);
        if (!keymap) {
                logger_error("[wl] failed to compile the keymap\n");
                return;
        }
        state = xkb_state_new(keymap);
        if (!state) {
                xkb_keymap_unref(keymap);
                return;
        }
        /* The compositor may switch keyboard layout while running */
        if (wlinput.xkb_state) {
                xkb_state_unref(wlinput.xkb_state);
        }
        if (wlinput.xkb_keymap) {
                xkb_keymap_unref(wlinput.xkb_keymap);
        }
        wlinput.xkb_keymap = keymap;
        wlinput.xkb_state = state;
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
                                  uint32_t serial, uint32_t depressed,
                                  uint32_t latched, uint32_t locked,
                                  uint32_t group)
{
        (void)data;
        (void)keyboard;
        (void)serial;
        if (!wlinput.xkb_state) {
                return;
        }
        xkb_state_update_mask(wlinput.xkb_state, depressed, latched, locked,
                              0, 0, group);
}

static void wl_keyboard_repeat_info(void *data, struct wl_keyboard *keyboard,
                                    int32_t rate, int32_t delay)
{
        (void)data;
        (void)keyboard;
        (void)rate;
        (void)delay;
}

static const struct wl_keyboard_listener keyboard_listener = {
        .keymap = wl_keyboard_keymap,
        .enter = wl_keyboard_enter,
        .leave = wl_keyboard_leave,
        .key = wl_keyboard_key,
        .modifiers = wl_keyboard_modifiers,
        .repeat_info = wl_keyboard_repeat_info,
};

static void wl_seat_capabilities(void *data, struct wl_seat *seat,
                                 uint32_t capabilities)
{
        (void)data;
        if ((capabilities & WL_SEAT_CAPABILITY_POINTER) &&
            !wlinput.pointer) {
                wlinput.pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(wlinput.pointer, &pointer_listener,
                                        NULL);
        } else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) &&
                   wlinput.pointer) {
                wl_pointer_release(wlinput.pointer);
                wlinput.pointer = NULL;
        }
        if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) &&
            !wlinput.keyboard) {
                wlinput.keyboard = wl_seat_get_keyboard(seat);
                wl_keyboard_add_listener(wlinput.keyboard,
                                         &keyboard_listener, NULL);
        } else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) &&
                   wlinput.keyboard) {
                wl_keyboard_release(wlinput.keyboard);
                wlinput.keyboard = NULL;
        }
}

static void wl_seat_name(void *data, struct wl_seat *seat, const char *name)
{
        (void)data;
        (void)seat;
        logger_debug("[wl] seat: %s\n", name ? name : "");
}

static const struct wl_seat_listener seat_listener = {
        .capabilities = wl_seat_capabilities,
        .name = wl_seat_name,
};

void ptk_wlinput_init(void)
{
        if (!wlapp.seat) {
                return;
        }
        wlinput.xkb_context =
            xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!wlinput.xkb_context) {
                logger_error("[wl] failed to create the xkb context\n");
                return;
        }
        wl_seat_add_listener(wlapp.seat, &seat_listener, NULL);
}

void ptk_wlinput_destroy(void)
{
        if (wlinput.pointer) {
                wl_pointer_release(wlinput.pointer);
                wlinput.pointer = NULL;
        }
        if (wlinput.keyboard) {
                wl_keyboard_release(wlinput.keyboard);
                wlinput.keyboard = NULL;
        }
        if (wlinput.xkb_state) {
                xkb_state_unref(wlinput.xkb_state);
                wlinput.xkb_state = NULL;
        }
        if (wlinput.xkb_keymap) {
                xkb_keymap_unref(wlinput.xkb_keymap);
                wlinput.xkb_keymap = NULL;
        }
        if (wlinput.xkb_context) {
                xkb_context_unref(wlinput.xkb_context);
                wlinput.xkb_context = NULL;
        }
}

#endif
