/*
 * lib/ui/src/ui_events.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

void ui_widget_destroy_listeners(ui_widget_t *w);

/** Initialize the event system of LCUI widgets */
void ui_init_events(void);

/** Destroy (release) the resources of the LCUI widget event system */
void ui_destroy_events(void);
