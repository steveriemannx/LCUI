/*
 * lib/pandagl/src/text/text_edit.c
 *
 * Copyright (c) 2018-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <pandagl.h>
#include "text_internal.h"

typedef enum { PD_TEXT_ACTION_INSERT, PD_TEXT_ACTION_APPEND } pd_text_action_t;

void pd_text_set_caret(pd_text_t *text, int line_num, int col)
{
        if (line_num < 0) {
                line_num = 0;
        } else if (line_num >= text->lines_length) {
                if (text->lines_length < 0) {
                        line_num = 0;
                } else {
                        line_num = text->lines_length - 1;
                }
        }
        if (col < 0) {
                col = 0;
        } else if (text->lines_length > 0) {
                if (col >= text->lines[line_num]->length) {
                        col = text->lines[line_num]->length;
                }
        } else {
                col = 0;
        }
        text->caret.x = col;
        text->caret.y = line_num;
}

int pd_text_set_caret_pixel(pd_text_t *text, int x, int y)
{
        pd_text_line_t *line;
        int i, pixel_pos, ins_x, ins_y = 0;

        pixel_pos = text->offset_y;
        for (i = 0; i < text->lines_length; ++i) {
                pixel_pos += text->lines[i]->height;
                if (pixel_pos >= y) {
                        ins_y = i;
                        break;
                }
        }
        if (i >= text->lines_length) {
                if (text->lines_length > 0) {
                        ins_y = text->lines_length - 1;
                } else {
                        text->caret.x = 0;
                        text->caret.y = 0;
                        return -1;
                }
        }
        line = text->lines[ins_y];
        ins_x = line->length;
        pixel_pos = text->offset_x;
        pixel_pos += pd_text_get_line_start_x(text, line);
        for (i = 0; i < line->length; ++i) {
                pd_char_t *txtchar;
                txtchar = line->string[i];
                if (!txtchar->bitmap) {
                        continue;
                }
                pixel_pos += txtchar->bitmap->metrics.hori_advance;
                /* If it is before the center of the current character */
                if (x <=
                    pixel_pos - txtchar->bitmap->metrics.hori_advance / 2) {
                        ins_x = i;
                        break;
                }
        }
        pd_text_set_caret(text, ins_y, ins_x);
        return 0;
}

/** Get the pixel coordinates of the character at a row and column */
int pd_text_get_char_pixel(pd_text_t *text, int line_num, int col,
                           pd_pos_t *pixel_pos)
{
        pd_text_line_t *line;
        int i, pixel_x = 0, pixel_y = 0;

        if (line_num < 0 || line_num >= text->lines_length) {
                return -1;
        }
        if (col < 0) {
                return -2;
        } else if (col > text->lines[line_num]->length) {
                return -3;
        }
        /* Accumulate the heights of the previous lines */
        for (i = 0; i < line_num; ++i) {
                pixel_y += text->lines[i]->height;
        }
        line = text->lines[line_num];
        pixel_x = pd_text_get_line_start_x(text, line);
        for (i = 0; i < col; ++i) {
                pd_char_t *txtchar = line->string[i];
                if (!txtchar || !txtchar->bitmap) {
                        continue;
                }
                pixel_x += txtchar->bitmap->metrics.hori_advance;
        }
        pixel_pos->x = pixel_x;
        pixel_pos->y = pixel_y;
        return 0;
}

int pd_text_get_caret_pixel(pd_text_t *text, pd_pos_t *pixel_pos)
{
        return pd_text_get_char_pixel(text, text->caret.y, text->caret.x,
                                      pixel_pos);
}

static const wchar_t *pd_text_process_style_tags(pd_text_t *text,
                                                 const wchar_t *p, list_t *tags,
                                                 pd_text_style_t **style)
{
        pd_text_style_t *s;
        const wchar_t *pp;

        pp = pd_style_tags_next_close_tag(tags, p);
        if (pp) {
                s = pd_style_tags_get_text_style(tags);
                if (s) {
                        pd_text_style_merge(s, &text->default_style);
                        list_append(&text->styles, s);
                }
                *style = s;
                return pp;
        }
        pp = pd_style_tags_next_open_tag(tags, p);
        if (pp) {
                s = pd_style_tags_get_text_style(tags);
                if (s) {
                        pd_text_style_merge(s, &text->default_style);
                        list_append(&text->styles, s);
                }
                *style = s;
                return pp;
        }
        return NULL;
}

static int pd_text_process(pd_text_t *text, const wchar_t *wstr,
                           pd_text_action_t action, list_t *tags)
{
        pd_text_eol_t eol;
        pd_text_line_t *line;
        pd_char_t txtchar;
        list_t tmp_tags;
        const wchar_t *p;
        int cur_col, cur_line, start_line, ins_x, ins_y;
        bool need_typeset, rect_has_added;
        pd_text_style_t *style = NULL;

        if (!wstr) {
                return -1;
        }
        need_typeset = false;
        rect_has_added = false;
        list_create(&tmp_tags);
        if (!tags) {
                tags = &tmp_tags;
        }
        /* If the text is appended to the end of the text */
        if (action == PD_TEXT_ACTION_APPEND) {
                if (text->lines_length > 0) {
                        cur_line = text->lines_length - 1;
                } else {
                        cur_line = 0;
                }
                line = pd_text_get_line(text, cur_line);
                if (!line) {
                        line = pd_text_append_line(text);
                }
                cur_col = line->length;
        } else { /* Otherwise the text is inserted at the current caret */
                cur_line = text->caret.y;
                cur_col = text->caret.x;
                line = pd_text_get_line(text, cur_line);
                if (!line) {
                        line = pd_text_append_line(text);
                }
        }
        start_line = cur_line;
        ins_x = cur_col;
        ins_y = cur_line;
        for (p = wstr; *p; ++p) {
                if (text->style_tag_enabled && *p != L'\\') {
                        const wchar_t *pp;
                        pp = pd_text_process_style_tags(text, p, tags, &style);
                        if (pp) {
                                p = pp - 1;
                                continue;
                        }
                }
                if (*p == L'\\' &&
                    (p[1] == L'[' || p[1] == L']' || p[1] == L'\\')) {
                        ++p;
                } else if (*p == '\r' || *p == '\n') {
                        /* Determine which line break mode it is */
                        if (*p == '\r') {
                                if (*(p + 1) == '\n') {
                                        eol = PD_TEXT_EOL_CR_LF;
                                } else {
                                        eol = PD_TEXT_EOL_CR;
                                }
                        } else {
                                eol = PD_TEXT_EOL_LF;
                        }
                        /* If the text line rect has not been recorded */
                        if (!rect_has_added) {
                                pd_text_mark_dirty(text, ins_y, -1);
                                rect_has_added = true;
                                start_line = ins_y;
                        }
                        /* Break the line at the caret of the current line */
                        pd_text_break_line(text, ins_y, ins_x, eol);
                        text->width = y_max(text->width, line->width);
                        need_typeset = true;
                        ++text->length;
                        ins_x = 0;
                        ++ins_y;
                        line = pd_text_get_line(text, ins_y);
                        continue;
                }
                txtchar.style = style;
                txtchar.code = *p;
                pd_char_update_bitmap(&txtchar, &text->default_style);
                pd_text_line_insert_copy(line, ins_x, &txtchar);
                ++text->length;
                ++ins_x;
        }
        /* Update the size of the current line */
        pd_text_update_line_size(text, line);
        text->width = y_max(text->width, line->width);
        if (action == PD_TEXT_ACTION_INSERT) {
                text->caret.x = ins_x;
                text->caret.y = ins_y;
        }
        /* Mark the text for retypesetting when autowrap is enabled */
        if (text->autowrap_enabled || need_typeset) {
                pd_text_request_typeset(text, cur_line);
        } else {
                pd_text_mark_line_dirty(text, cur_line, 0, -1);
        }
        /* If the text line rect has already been recorded */
        if (rect_has_added) {
                pd_text_mark_dirty(text, start_line, -1);
                rect_has_added = true;
        }
        pd_style_tags_clear(&tmp_tags);
        return 0;
}

int pd_text_insert(pd_text_t *text, const wchar_t *wstr, list_t *tags)
{
        return pd_text_process(text, wstr, PD_TEXT_ACTION_INSERT, tags);
}

int pd_text_append(pd_text_t *text, const wchar_t *wstr, list_t *tag_stack)
{
        return pd_text_process(text, wstr, PD_TEXT_ACTION_APPEND, tag_stack);
}

int pd_text_write(pd_text_t *text, const wchar_t *wstr, list_t *tag_stack)
{
        pd_text_clear(text);
        return pd_text_append(text, wstr, tag_stack);
}

size_t pd_text_read(pd_text_t *text, size_t start_pos, size_t max_len,
                    wchar_t *wstr_buff)
{
        size_t i, line_num, col;
        pd_text_line_t *line;

        if (max_len == 0) {
                wstr_buff[0] = 0;
                return 0;
        }
        /* First compute the row and column from the 1D coordinate */
        for (i = 0, line_num = 0, col = 0; line_num < text->lines_length;
             ++line_num) {
                if (i >= start_pos) {
                        col = start_pos - i;
                        break;
                }
                i += text->lines[line_num]->length;
        }
        for (i = 0; line_num < text->lines_length && i < max_len; ++line_num) {
                line = text->lines[line_num];
                for (; col < line->length && i < max_len; ++col, ++i) {
                        wstr_buff[i] = line->string[col]->code;
                }
                /* Restore the line break char from the eol before next line */
                if (i >= max_len) {
                        break;
                }
                switch (line->eol) {
                case PD_TEXT_EOL_CR:
                        wstr_buff[i++] = L'\r';
                        break;
                case PD_TEXT_EOL_LF:
                        wstr_buff[i++] = L'\n';
                        break;
                case PD_TEXT_EOL_CR_LF:
                        wstr_buff[i++] = L'\r';
                        if (i >= max_len) {
                                break;
                        }
                        wstr_buff[i++] = L'\n';
                        break;
                case PD_TEXT_EOL_NONE:
                default:
                        break;
                }
                /* The column must be reset when moving to the next line */
                col = 0;
        }
        wstr_buff[i] = 0;
        return i;
}

wchar_t *pd_text_to_wcs(pd_text_t *text)
{
        size_t total = 0;
        int line_num;
        pd_text_line_t *line;
        wchar_t *buf;

        for (line_num = 0; line_num < text->lines_length; ++line_num) {
                line = text->lines[line_num];
                total += (size_t)line->length;
                switch (line->eol) {
                case PD_TEXT_EOL_CR:
                case PD_TEXT_EOL_LF:
                        total += 1;
                        break;
                case PD_TEXT_EOL_CR_LF:
                        total += 2;
                        break;
                default:
                        break;
                }
        }
        buf = malloc(sizeof(wchar_t) * (total + 1));
        if (!buf) {
                return NULL;
        }
        pd_text_read(text, 0, total, buf);
        buf[total] = 0;
        return buf;
}

/** Delete the character at a row and column and the text to its right */
static int pd_text_delete_ex(pd_text_t *text, int char_y, int char_x,
                             int n_char)
{
        int end_x, end_y, i, j, len;
        pd_text_line_t *line, *end_line, *prev_line;

        if (char_x < 0) {
                char_x = 0;
        }
        if (char_y < 0) {
                char_y = 0;
        }
        if (n_char <= 0) {
                return -1;
        }
        if (char_y >= text->lines_length) {
                return -2;
        }
        line = text->lines[char_y];
        if (char_x > line->length) {
                char_x = line->length;
        }
        i = n_char;
        end_x = char_x;
        end_y = char_y;
        /* Compute the position of the end point */
        for (; end_y < text->lines_length && n_char > 0; ++end_y) {
                line = text->lines[end_y];
                if (end_x + n_char <= line->length) {
                        end_x += n_char;
                        n_char = 0;
                        break;
                }
                n_char -= (line->length - end_x);
                if (line->eol == PD_TEXT_EOL_NONE) {
                        end_x = 0;
                } else {
                        n_char -= 1;
                        end_x = 0;
                }
        }
        if (n_char >= 0) {
                text->length -= i - n_char;
        } else {
                text->length -= n_char;
        }
        if (end_y >= text->lines_length) {
                end_y = text->lines_length - 1;
                end_line = text->lines[end_y];
                end_x = end_line->length;
        } else {
                end_line = text->lines[end_y];
        }
        if (end_x > end_line->length) {
                end_x = end_line->length;
        }
        if (end_x == char_x && end_y == char_y) {
                return 0;
        }
        /* Get the previous line (there is none for the first line) */
        prev_line = char_y > 0 ? text->lines[char_y - 1] : NULL;
        // Length after joining the start line and the end line
        // Start line: 0 1 2 3 4 5, start position: 2
        // End line: 0 1 2 3 4 5, end position: 4
        // Joined length: 2 + 6 - 4 = 4
        len = char_x + end_line->length - end_x;
        if (len < 0) {
                return -3;
        }
        /* If they are on the same line */
        if (line == end_line) {
                if (end_x > end_line->length) {
                        return -4;
                }
                pd_text_mark_line_dirty(text, char_y, char_x, -1);
                pd_text_request_typeset(text, char_y);
                /* Free the characters in the deleted range */
                pd_text_line_erase(line, char_x, end_x);
                /* Move the content after the end point forward */
                pd_text_line_move(line, char_x, line, end_x, line->length);
                /* If the line is empty and not the first one, and the previous
                 * line has an eol, remove this line; do not access line */
                if (len <= 0 && char_y > 0 && prev_line &&
                    prev_line->eol != PD_TEXT_EOL_NONE) {
                        pd_text_delete_line(text, char_y);
                        return 0;
                }
                /* Adjust the capacity of the start line */
                pd_text_line_set_length(line, len);
                /* Update the size of the text line */
                pd_text_update_line_size(text, line);
                return 0;
        }
        /* Free the deleted characters of the start line, [char_x, length) */
        pd_text_line_erase(line, char_x, line->length);
        /* Free the deleted characters of the end line, [0, end_x) */
        pd_text_line_erase(end_line, 0, end_x);
        /* If the end point is at the end of a line that is not the last one,
         * advance the end point to the start of the next line */
        if (end_x == end_line->length && end_y < text->lines_length - 1) {
                ++end_y;
                end_line = pd_text_get_line(text, end_y);
                end_x = 0;
                len = char_x + end_line->length;
        }
        pd_text_line_set_length(line, len);
        /* Mark the rects of all lines after the current one for refresh */
        pd_text_mark_dirty(text, char_y + 1, -1);
        /* Remove the text lines between the start line and the end line */
        for (i = char_y + 1, j = i; j < end_y; ++j) {
                pd_text_mark_line_dirty(text, i, 0, -1);
                pd_text_delete_line(text, i);
        }
        end_y = char_y + 1;
        /* Join the end line content to the start line (ownership moves) */
        pd_text_line_move(line, char_x, end_line, end_x, end_line->length);
        pd_text_update_line_size(text, line);
        pd_text_mark_line_dirty(text, end_y, 0, -1);
        /* Remove the end line */
        pd_text_delete_line(text, end_y);
        /* If the start line has no content and the previous line has an eol,
         * the start line needs to be removed */
        if (len <= 0 && char_y > 0 && prev_line &&
            prev_line->eol != PD_TEXT_EOL_NONE) {
                pd_text_mark_line_dirty(text, char_y, 0, -1);
                pd_text_delete_line(text, char_y);
        }
        pd_text_request_typeset(text, char_y);
        return 0;
}

/** Delete the text to the right of the caret position */
int pd_text_delete(pd_text_t *text, int n_char)
{
        return pd_text_delete_ex(text, text->caret.y, text->caret.x, n_char);
}

/** Backspace delete, removes the text to the left of the caret position */
int pd_text_backspace(pd_text_t *text, int n_char)
{
        int n_del;
        int char_x, char_y;
        pd_text_line_t *line;

        /* First get the position of the current character */
        char_x = text->caret.x;
        char_y = text->caret.y;
        /* Then compute the position after deleting n_char characters */
        for (n_del = n_char; char_y >= 0; --char_y) {
                line = text->lines[char_y];
                /* Move to the end of the line if it is not the current one */
                if (char_y < text->caret.y) {
                        char_x = line->length;
                        if (line->eol == PD_TEXT_EOL_NONE) {
                                --char_x;
                        }
                }
                if (char_x >= n_del) {
                        char_x = char_x - n_del;
                        n_del = 0;
                        break;
                }
                n_del = n_del - char_x - 1;
        }
        if (char_y < 0 || n_del == n_char) {
                return -1;
        }
        /* Adjust the count if fewer than n_char characters can be deleted */
        if (n_del > 0) {
                n_char -= n_del;
        }
        /* Start deleting the text */
        pd_text_delete_ex(text, char_y, char_x, n_char);
        /* If the last line was removed, move the caret to the line before */
        if (char_x == 0 && text->lines_length > 0 &&
            char_y >= text->lines_length) {
                char_y = text->lines_length - 1;
                char_x = text->lines[char_y]->length;
        }
        /* Update the caret position */
        pd_text_set_caret(text, char_y, char_x);
        return 0;
}

/** Enable or disable style tags */
void pd_text_set_style_tag(pd_text_t *text, bool enabled)
{
        text->style_tag_enabled = enabled;
}
