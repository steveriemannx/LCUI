/*
 * lib/pandagl/src/text/text_typeset.c
 *
 * Copyright (c) 2018-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <pandagl.h>
#include "text_internal.h"

#define isalpha_(ch) ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))

void pd_text_break_line(pd_text_t *text, int line_num, int col,
                        pd_text_eol_t eol)
{
        int n;
        pd_text_line_t *line, *next;

        line = pd_text_get_line(text, line_num);
        next = pd_text_insert_line(text, line_num + 1);
        /* Move the original end of line of this line to the next line */
        next->eol = line->eol;
        line->eol = eol;
        for (n = line->length - 1; n >= col; --n) {
                pd_text_line_insert(next, 0, line->string[n]);
                line->string[n] = NULL;
        }
        line->length = col;
        pd_text_update_line_size(text, line);
        pd_text_update_line_size(text, next);
}

static void pd_text_merge_line(pd_text_t *text, int line_num)
{
        int old_len;
        pd_text_line_t *line = pd_text_get_line(text, line_num);
        pd_text_line_t *next = pd_text_get_line(text, line_num + 1);

        if (!line || !next) {
                return;
        }
        if (text->caret.y > line_num) {
                --text->caret.y;
                if (text->caret.y == line_num) {
                        text->caret.x += line->length;
                }
        }
        old_len = line->length;
        pd_text_line_set_length(line, line->length + next->length);
        pd_text_line_move(line, old_len, next, 0, next->length);
        line->eol = next->eol;
        pd_text_update_line_size(text, line);
        pd_text_delete_line(text, line_num + 1);
}

static void pd_text_typeset_line(pd_text_t *text, int line_num)
{
        int col, line_width = 0, word_col = 0;
        int max_width =
            text->fixed_width > 0 ? text->fixed_width : text->max_width;

        pd_char_t *txtchar;
        pd_text_line_t *line = text->lines[line_num];
        bool autowrap =
            max_width > 0 && text->autowrap_enabled && text->multiline_enabled;

        for (col = 0; col < line->length; ++col) {
                txtchar = line->string[col];
                if (!txtchar->bitmap) {
                        continue;
                }
                /* Accumulate the line width */
                line_width += txtchar->bitmap->metrics.hori_advance;
                /* If it is the first char, or the width fits the limit */
                if (!autowrap || col < 1 || line_width <= max_width) {
                        if (isalpha_(txtchar->code)) {
                        } else {
                                word_col = col + 1;
                        }
                        continue;
                }
                if (text->word_break == PD_WORD_BREAK_NORMAL) {
                        if (word_col < 1) {
                                continue;
                        }
                        col = word_col;
                }
                pd_text_break_line(text, line_num, col, PD_TEXT_EOL_NONE);
                return;
        }
        pd_text_update_line_size(text, line);
        /* If this line has a line break, or it is the last line */
        if (line->eol != PD_TEXT_EOL_NONE ||
            line_num == text->lines_length - 1) {
                return;
        }
        line_width = line->width;
        /* This line is under the limit, so pull text from the next line */
        if (line->eol == PD_TEXT_EOL_NONE) {
                pd_text_mark_line_dirty(text, line_num, 0, -1);
                pd_text_mark_line_dirty(text, line_num + 1, 0, -1);
                pd_text_merge_line(text, line_num);
                pd_text_typeset_line(text, line_num);
        }
}

/** Typeset the text starting from the given line */
void pd_text_typeset(pd_text_t *text, int start_line)
{
        int line_num;
        /* Record the rect of each text line before typesetting */
        pd_text_mark_dirty(text, start_line, -1);
        for (line_num = start_line; line_num < text->lines_length; ++line_num) {
                pd_text_typeset_line(text, line_num);
        }
        /* Record the rect of each text line after typesetting */
        pd_text_mark_dirty(text, start_line, -1);
}

/** Enable or disable multiline text mode */
void pd_text_set_multiline(pd_text_t *text, bool enabled)
{
        if (text->multiline_enabled != enabled) {
                text->multiline_enabled = enabled;
                pd_text_request_typeset(text, 0);
        }
}

void pd_text_set_autowrap(pd_text_t *text, bool autowrap)
{
        if (text->autowrap_enabled != autowrap) {
                text->autowrap_enabled = autowrap;
                pd_text_request_typeset(text, 0);
        }
}

void pd_text_set_word_break(pd_text_t *text, pd_word_break_t word_break)
{
        if (text->word_break != word_break) {
                text->word_break = word_break;
                pd_text_request_typeset(text, 0);
        }
}

void pd_text_set_align(pd_text_t *text, int align)
{
        text->text_align = align;
        text->task.update_typeset = true;
        text->task.typeset_start_line = 0;
}

void pd_text_set_line_height(pd_text_t *text, int height)
{
        text->line_height = height;
        text->task.update_typeset = true;
        text->task.typeset_start_line = 0;
}
