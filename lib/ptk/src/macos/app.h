/*
 * lib/ptk/src/macos/app.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef PTK_MACOS_APP_H
#define PTK_MACOS_APP_H

#include "ptk/app.h"
#include "ptk/clipboard.h"

void ptk_macosapp_driver_init(ptk_app_driver_t *driver);
void ptk_macoswindow_driver_init(ptk_window_driver_t *driver);
void ptk_macoswindow_set_fullscreen(ptk_window_t *wnd, bool fullscreen);
unsigned ptk_macoswindow_get_dpi(ptk_window_t *wnd);
int ptk_macos_clipboard_request_text(ptk_clipboard_callback_t callback,
                                     void *arg);
int ptk_macos_clipboard_set_text(const wchar_t *text, size_t len);

#endif
