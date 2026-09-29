/*
 * lib/ui/include/ui/events.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef LIB_UI_INCLUDE_UI_EVENTS_H
#define LIB_UI_INCLUDE_UI_EVENTS_H

#include "common.h"
#include "types.h"

LIBUI_BEGIN_DECLS

/** Set the event blocking of a widget and its children */
LIBUI_INLINE void ui_widget_block_event(ui_widget_t *w, bool block)
{
        w->event_blocked = block;
}

/** Trigger an event, letting handlers be called in the main loop */
LIBUI_PUBLIC int ui_widget_post_event(ui_widget_t *w, const ui_event_t *e,
                                      void *arg,
                                      ui_event_arg_destructor_t destroy_arg);

/** Trigger an event, calling the handlers directly */
LIBUI_PUBLIC int ui_widget_emit_event(ui_widget_t *w, ui_event_t e, void *arg);

/** Automatically allocate an available event id */
LIBUI_PUBLIC int ui_alloc_event_id(void);

/** Set the name corresponding to an event id */
LIBUI_PUBLIC int ui_set_event_id(int event_id, const char *event_name);

/** Get the name corresponding to an event id */
LIBUI_PUBLIC const char *ui_get_event_name(int event_id);

/** Get the id corresponding to an event name */
LIBUI_PUBLIC int ui_get_event_id(const char *event_name);

LIBUI_PUBLIC void ui_event_init(ui_event_t *e, const char *name);

LIBUI_PUBLIC void ui_event_destroy(ui_event_t *e);

/**
 * Add a widget event binding
 * @param[in] widget target widget
 * @param[in] event_id event id
 * @param[in] handler event handler function
 * @param[in] listener_data listener context data, available as e->data
 * @param[in] destroy_arg destructor for the listener context data
 * @return 0 on success, a negative value on failure
 */
LIBUI_PUBLIC ui_event_listener_t *ui_widget_add_event_listener(
    ui_widget_t *widget, int event_id, ui_event_handler_t handler,
    void *listener_data, void (*destroy_arg)(void *));

/**
 * Remove a widget event binding
 * @param[in] widget target widget
 * @param[in] event_name event name
 * @param[in] handler function bound to the event
 * @return 0 on success, a negative value on failure
 */
LIBUI_PUBLIC int ui_widget_remove_event_listener(ui_widget_t *w, int event_id,
                                                 ui_event_handler_t handler,
                                                 void *listener_data);

/**
 * Add a widget event binding
 * @param[in] widget target widget
 * @param[in] event_name event name
 * @param[in] handler event handler function
 * @param[in] listener_data listener context data, available as e->data
 * @return the number of removed event listeners
 */
LIBUI_PUBLIC ui_event_listener_t *ui_widget_on(ui_widget_t *widget,
                                               const char *event_name,
                                               ui_event_handler_t handler,
                                               void *listener_data);

/**
 * Remove a widget event binding
 * @param[in] widget target widget
 * @param[in] event_name event name
 * @param[in] handler function bound to the event
 * @return 0 on success, a negative value on failure
 */
LIBUI_PUBLIC int ui_widget_off(ui_widget_t *widget, const char *event_name,
                               ui_event_handler_t handler, void *listener_data);

LIBUI_PUBLIC int ui_emit_event(ui_event_t e, void *emit_arg);

LIBUI_PUBLIC int ui_post_event(const ui_event_t *e, void *emit_arg,
                               ui_event_arg_destructor_t destroy_arg);

LIBUI_PUBLIC ui_event_listener_t *ui_add_event_listener(
    ui_widget_t *widget, int event_id, ui_event_handler_t handler,
    void *emit_arg, ui_event_arg_destructor_t destroy_arg);

LIBUI_PUBLIC int ui_remove_event_listener(ui_widget_t *w, int event_id,
                                          ui_event_handler_t handler,
                                          void *listener_data);

LIBUI_PUBLIC ui_event_listener_t *ui_on_event(const char *event_name,
                                              ui_event_handler_t handler,
                                              void *listener_data);
LIBUI_PUBLIC int ui_off_event(const char *event_name,
                              ui_event_handler_t handler, void *listener_data);

/** Clear the event object, usually called when the widget is destroyed
 * to avoid events still being sent to it after it is destroyed
 */
LIBUI_PUBLIC void ui_clear_event_target(ui_widget_t *widget);

/** get current focused widget */
LIBUI_PUBLIC ui_widget_t *ui_get_focus(void);

/** Set a widget as the focus */
LIBUI_PUBLIC int ui_set_focus(ui_widget_t *widget);

/** Stop the event propagation of a widget */
LIBUI_PUBLIC int ui_widget_stop_event_propagation(ui_widget_t *widget);

/** Set mouse capture for a widget to capture global mouse events */
LIBUI_PUBLIC void ui_widget_set_mouse_capture(ui_widget_t *w);

/** Release the mouse capture of a widget */
LIBUI_PUBLIC void ui_widget_release_mouse_capture(ui_widget_t *w);

/**
 * Set touch point capture for a widget to capture global touch events
 * @param[in] w widget
 * @param[in] point_id touch point id, -1 captures all touch points
 * @returns 0 on success, or -1 if the touch point is already captured
 */
LIBUI_PUBLIC int ui_widget_set_touch_capture(ui_widget_t *w, int point_id);

/**
 * Release the touch point capture of a widget
 * @param[in] w widget
 * @param[in] point_id touch point id, -1 releases all touch points
 */
LIBUI_PUBLIC int ui_widget_release_touch_capture(ui_widget_t *w, int point_id);

LIBUI_END_DECLS

#endif
