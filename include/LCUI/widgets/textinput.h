/*
 * include/LCUI/widgets/textinput.h
 *
 * Copyright (c) 2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

/*
 * include/LCUI/widgets/textinput.h: -- textinput widget, used to allow user
 * edit text.
 * Copyright (c) 2018-2026, Liu chao <lc-soft@live.cn> All rights
 * reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI,
 * distributed under the MIT License found in the
 * LICENSE.TXT file in the
 * root directory of this source tree.
 */

#ifndef LCUI_INCLUDE_WIDGETS_TEXTINPUT_H
#define LCUI_INCLUDE_WIDGETS_TEXTINPUT_H

#include <LCUI/common.h>
#include <ui.h>

LCUI_BEGIN_HEADER

/** Enable style tag parser  */
LCUI_API void ui_textinput_enable_style_tag(ui_widget_t *widget, bool enable);

LCUI_API void ui_textinput_enable_multiline(ui_widget_t *widget, bool enable);

LCUI_API void ui_textinput_move_caret(ui_widget_t *widget, int row, int col);

/** Clear the text content */
LCUI_API void ui_textinput_clear_text(ui_widget_t *widget);

/** Get the text content */
LCUI_API size_t ui_textinput_get_text_w(ui_widget_t *w, size_t start,
                                        size_t max_len, wchar_t *buf);

/** Get the text length */
LCUI_API size_t ui_textinput_get_text_length(ui_widget_t *w);

/** Set the caret of the text edit box: whether it blinks, blink interval */
LCUI_API void ui_textinput_set_caret_blink(ui_widget_t *w, bool visible,
                                           int time);

/** Set the text for the text box (wide character version) */
LCUI_API int ui_textinput_set_text_w(ui_widget_t *widget, const wchar_t *wstr);

LCUI_API int ui_textinput_set_text(ui_widget_t *widget, const char *utf8_str);

/** Append text to the text box (wide character version) */
LCUI_API int ui_textinput_append_text_w(ui_widget_t *widget,
                                        const wchar_t *wstr);

/** Insert text into the text box (wide character version) */
LCUI_API int ui_textinput_insert_text_w(ui_widget_t *widget,
                                        const wchar_t *wstr);

/** Set the placeholder shown when the text edit box is empty */
LCUI_API int ui_textinput_set_placeholder_w(ui_widget_t *w,
                                            const wchar_t *wstr);

LCUI_API int ui_textinput_set_placeholder(ui_widget_t *w, const char *str);

/** Set the password masking character */
LCUI_API void ui_textinput_set_password_char(ui_widget_t *w, wchar_t ch);

LCUI_API void ui_register_textinput(void);

LCUI_END_HEADER

#endif
