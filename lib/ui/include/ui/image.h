/*
 * lib/ui/include/ui/image.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef LIB_UI_INCLUDE_UI_IMAGE_H
#define LIB_UI_INCLUDE_UI_IMAGE_H

#include "common.h"
#include "types.h"

#define ui_image_on ui_image_add_event_listener
#define ui_image_off ui_image_remove_event_listener

typedef enum ui_image_state {
        UI_IMAGE_STATE_PENDING,
        UI_IMAGE_STATE_LOADING,
        UI_IMAGE_STATE_COMPLETE
} ui_image_state_t;

typedef enum ui_image_event_type {
        UI_IMAGE_EVENT_LOAD,
        UI_IMAGE_EVENT_PROGRESS,
        UI_IMAGE_EVENT_ERROR,
} ui_image_event_type_t;

typedef struct ui_image {
        pd_canvas_t data;
        pd_error_t error;
        ui_image_state_t state;
        char *path;
        float progress;
} ui_image_t;

typedef struct ui_image_event {
        ui_image_event_type_t type;
        ui_image_t *image;
        void *data;
} ui_image_event_t;

typedef void (*ui_image_event_handler_t)(ui_image_event_t *);

LIBUI_BEGIN_DECLS

LIBUI_PUBLIC ui_image_t *ui_get_image(const char *path);

/**
 * Create a UIImage object and load data from the specified path
 * Note: the data of a UIImage object is not loaded immediately
 **/
LIBUI_PUBLIC ui_image_t *ui_image_create(const char *path);

LIBUI_INLINE bool ui_image_valid(ui_image_t *image)
{
        return image != NULL && image->error == PD_OK &&
               image->state == UI_IMAGE_STATE_COMPLETE;
}

LIBUI_PUBLIC void ui_image_destroy(ui_image_t *image);
LIBUI_PUBLIC bool ui_image_add_event_listener(ui_image_t *image,
                                              ui_image_event_type_t type,
                                              ui_image_event_handler_t handler,
                                              void *data);
LIBUI_PUBLIC bool ui_image_remove_event_listener(
    ui_image_t *image, ui_image_event_type_t type,
    ui_image_event_handler_t handler, void *data);

LIBUI_INLINE bool ui_image_on_load(ui_image_t *image,
                                   ui_image_event_handler_t handler, void *data)
{
        return ui_image_add_event_listener(image, UI_IMAGE_EVENT_LOAD, handler,
                                           data);
}

LIBUI_INLINE bool ui_image_on_error(ui_image_t *image,
                                    ui_image_event_handler_t handler,
                                    void *data)
{
        return ui_image_add_event_listener(image, UI_IMAGE_EVENT_ERROR, handler,
                                           data);
}

LIBUI_INLINE bool ui_image_on_progress(ui_image_t *image,
                                       ui_image_event_handler_t handler,
                                       void *data)
{
        return ui_image_add_event_listener(image, UI_IMAGE_EVENT_PROGRESS,
                                           handler, data);
}

LIBUI_INLINE bool ui_image_off_load(ui_image_t *image,
                                    ui_image_event_handler_t handler,
                                    void *data)
{
        return ui_image_remove_event_listener(image, UI_IMAGE_EVENT_LOAD,
                                              handler, data);
}

LIBUI_INLINE bool ui_image_off_error(ui_image_t *image,
                                     ui_image_event_handler_t handler,
                                     void *data)
{
        return ui_image_remove_event_listener(image, UI_IMAGE_EVENT_ERROR,
                                              handler, data);
}

LIBUI_INLINE bool ui_image_off_progress(ui_image_t *image,
                                        ui_image_event_handler_t handler,
                                        void *data)
{
        return ui_image_remove_event_listener(image, UI_IMAGE_EVENT_PROGRESS,
                                              handler, data);
}

/**
 * Load data for all UIImage objects
 * Note: this function should be called outside the UI thread
 **/
LIBUI_PUBLIC void ui_load_images(void);

/**
 * Process the events of UIImage
 * Note: this function should be called in the UI thread
 */
LIBUI_PUBLIC void ui_process_image_events(void);

/**
 * Clean up unused UIImage objects and free memory resources
 * Note: this function should be called in the UI thread
 */
LIBUI_PUBLIC void ui_clear_images(void);

LIBUI_PUBLIC void ui_set_image_loader_callback(void (*callback)(ui_image_t *));

LIBUI_END_DECLS

#endif
