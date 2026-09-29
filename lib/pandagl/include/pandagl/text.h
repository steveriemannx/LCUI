/*
 * lib/pandagl/include/pandagl/text.h: -- Text layout and rendering module.
 *
 * Copyright (c) 2018-2026, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef LIB_PANDAGL_INCLUDE_PANDAGL_TEXT_H
#define LIB_PANDAGL_INCLUDE_PANDAGL_TEXT_H

#include <yutil.h>
#include "common.h"
#include "types.h"
#include "font.h"

PD_BEGIN_DECLS

typedef struct pd_text_style {
        bool has_family : 1;
        bool has_style : 1;
        bool has_weight : 1;
        bool has_back_color : 1;
        bool has_fore_color : 1;
        bool has_pixel_size : 1;

        int style;
        int weight;
        int *font_ids;

        pd_color_t fore_color;
        pd_color_t back_color;

        int pixel_size;
} pd_text_style_t;

typedef enum pd_text_align_t {
        PD_TEXT_ALIGN_LEFT,
        PD_TEXT_ALIGN_RIGHT,
        PD_TEXT_ALIGN_CENTER
} pd_text_align_t;

typedef struct pd_char {
        wchar_t code;                    /**< character code */
        pd_text_style_t *style;          /**< style used by this character */
        const pd_glyph_bitmap_t *bitmap; /**< font bitmap data (read-only) */
} pd_char_t;

/** end of line character */
typedef enum pd_text_eol_t {
        /** no line break */
        PD_TEXT_EOL_NONE,

        /** mac os style line break, cf = carriage-return, character: \r */
        PD_TEXT_EOL_CR,

        /** unix/linux style line break, lf = line-feed, character: \r */
        PD_TEXT_EOL_LF,

        /** windows style line break: \r\n */
        PD_TEXT_EOL_CR_LF
} pd_text_eol_t;

/* text line */
typedef struct pd_text_line {
        int width;          /**< width */
        int height;         /**< height */
        int length;         /**< text length of this line */
        int baseline_y;     /**< baseline y offset relative to the line top */
        pd_char_t **string; /**< text data of this line */
        pd_text_eol_t eol;  /**< end of line type */
} pd_text_line_t;

/**
 * word-break mode
 * the word-break mode specifies whether or not the textlayer should
 * insert line breaks wherever the text would otherwise overflow its
 * content box.
 */
typedef enum pd_word_break_t {
        PD_WORD_BREAK_NORMAL,
        PD_WORD_BREAK_BREAK_ALL
} pd_word_break_t;

typedef struct pd_text_caret {
        int x; /**< column index of the caret */
        int y; /**< row index of the caret */
} pd_text_caret_t;

typedef struct pd_text {
        int offset_x;          /**< x-axis coordinate offset */
        int offset_y;          /**< y-axis coordinate offset */
        int new_offset_x;      /**< new x-axis coordinate offset */
        int new_offset_y;      /**< new y-axis coordinate offset */
        pd_text_caret_t caret; /**< caret position */
        int width;             /**< actual text width */

        /**
         * Fixed width and height
         * When they are 0, the text width and height adapt to the text content
         * When they are greater than 0, text alignment uses these values
         */
        int fixed_width, fixed_height;
        /**
         * Maximum text width and height
         * When no fixed width is set, text is typeset by the maximum width
         */
        int max_width, max_height;

        int length;
        int line_height;
        pd_text_align_t text_align;
        pd_word_break_t word_break;
        bool multiline_enabled;
        bool autowrap_enabled;
        bool style_tag_enabled;
        list_t dirty_rects;
        list_t styles;
        pd_text_style_t default_style;
        pd_text_line_t **lines;
        int lines_length;
        struct {
                bool update_bitmap;
                bool update_typeset;
                int typeset_start_line;
                bool redraw_all;
        } task;
} pd_text_t;

/** Get the total number of text lines */
PD_PUBLIC int pd_text_get_lines_length(pd_text_t *layer);

/** Get the height of the given text line */
PD_PUBLIC int pd_text_get_line_height(pd_text_t *layer, int line_num);

/** Get the text length of the given text line */
PD_PUBLIC int pd_text_get_line_length(pd_text_t *layer, int line_num);

/** Add a task to update the text typesetting */
PD_PUBLIC void pd_text_request_typeset(pd_text_t *layer, int start_row);

/** Set the text alignment */
PD_PUBLIC void pd_text_set_align(pd_text_t *layer, int align);

/** Set the coordinate offset */
PD_PUBLIC bool pd_text_set_offset(pd_text_t *layer, int offset_x, int offset_y);

PD_PUBLIC pd_text_t *pd_text_create(void);

PD_PUBLIC void pd_text_destroy(pd_text_t *layer);

/** Mark the rectangles of the text lines in the given range as invalid */
PD_PUBLIC void pd_text_mark_dirty(pd_text_t *layer, int start_row, int end_row);

/** Set the row and column coordinates of the caret */
PD_PUBLIC void pd_text_set_caret(pd_text_t *layer, int line_num, int col);

/** Set the caret row and column from pixel coordinates */
PD_PUBLIC int pd_text_set_caret_pixel(pd_text_t *layer, int x, int y);

/** Get the pixel coordinates of the character at a row and column */
PD_PUBLIC int pd_text_get_char_pixel(pd_text_t *layer, int line_num, int col,
                                     pd_pos_t *pixel_pos);

/** Get the pixel coordinates of the text caret */
PD_PUBLIC int pd_text_get_caret_pixel(pd_text_t *layer, pd_pos_t *pixel_pos);

/** Clear the text */
PD_PUBLIC void pd_text_clear(pd_text_t *layer);

PD_PUBLIC int pd_text_insert(pd_text_t *layer, const wchar_t *wstr,
                             list_t *tag_stack);

PD_PUBLIC int pd_text_append(pd_text_t *layer, const wchar_t *wstr,
                             list_t *tag_stack);

PD_PUBLIC int pd_text_write(pd_text_t *layer, const wchar_t *wstr,
                            list_t *tag_stack);

/** Get the text in the text layer (wide character version) */
PD_PUBLIC size_t pd_text_read(pd_text_t *text, size_t start_pos, size_t max_len,
                              wchar_t *wstr_buff);

/**
 * Get the complete content of the text layer, including the line breaks
 * restored from line.eol. The caller frees the result; NULL on failure.
 */
PD_PUBLIC wchar_t *pd_text_to_wcs(pd_text_t *text);

/** Compute and get the width of the text */
PD_PUBLIC int pd_text_get_width(pd_text_t *layer);

/** Compute and get the height of the text */
PD_PUBLIC int pd_text_get_height(pd_text_t *layer);

/** Set the fixed size */
PD_PUBLIC int pd_text_set_fixed_size(pd_text_t *layer, int width, int height);

/** Set the maximum size */
PD_PUBLIC int pd_text_set_max_size(pd_text_t *layer, int width, int height);

/** Enable or disable multiline text mode */
PD_PUBLIC void pd_text_set_multiline(pd_text_t *layer, bool enabled);

/** Delete the text to the right of the caret position */
PD_PUBLIC int pd_text_delete(pd_text_t *layer, int n_char);

/** Backspace delete, removes the text to the left of the caret position */
PD_PUBLIC int pd_text_backspace(pd_text_t *layer, int n_char);

/** Enable or disable the autowrap mode */
PD_PUBLIC void pd_text_set_autowrap(pd_text_t *layer, bool autowrap);

/** Set the word-break mode */
PD_PUBLIC void pd_text_set_word_break(pd_text_t *layer, pd_word_break_t mode);

/** Enable or disable style tags */
PD_PUBLIC void pd_text_set_style_tag(pd_text_t *layer, bool enabled);

/** Reload the font bitmap of every character */
PD_PUBLIC void pd_text_reload_bitmap(pd_text_t *layer);

/** Update the data */
PD_PUBLIC void pd_text_update(pd_text_t *layer, list_t *rects);

/**
 * Draw the content of the given area in the text layer to the target image
 * @param layer The text layer to use
 * @param area The area of the text layer to draw
 * @param layer_pos The position of the text layer in the target image
 * @param cavans The target canvas
 */
PD_PUBLIC int pd_text_render_to(pd_text_t *layer, pd_rect_t area,
                                pd_pos_t layer_pos, pd_canvas_t *canvas);

/** Set the global text style */
PD_PUBLIC void pd_text_set_style(pd_text_t *layer, pd_text_style_t *style);

/** Set the height of the text lines */
PD_PUBLIC void pd_text_set_line_height(pd_text_t *layer, int height);

/** Initialize the font style data */
PD_PUBLIC void pd_text_style_init(pd_text_style_t *data);

PD_PUBLIC int pd_text_style_copy_family(pd_text_style_t *dst,
                                        pd_text_style_t *src);

PD_PUBLIC int pd_text_style_copy(pd_text_style_t *dst, pd_text_style_t *src);

PD_PUBLIC void pd_text_style_destroy(pd_text_style_t *data);

PD_PUBLIC void pd_text_style_merge(pd_text_style_t *base,
                                   pd_text_style_t *target);

/* Set the font weight */
PD_PUBLIC int pd_text_style_set_weight(pd_text_style_t *ts,
                                       pd_font_weight_t weight);

PD_PUBLIC int pd_text_style_set_style(pd_text_style_t *ts,
                                      pd_font_style_t style);

/**
 * Set the font
 * @param[in][out] ts The font style data
 * @param[in] str Font names, separated by commas if there are several
 */
PD_PUBLIC int pd_text_style_set_font(pd_text_style_t *ts,
                                     const char *const *names);

PD_PUBLIC void pd_text_style_set_size(pd_text_style_t *ts, int pixel_size);

PD_PUBLIC void pd_text_style_set_fore_color(pd_text_style_t *ts,
                                            pd_color_t color);

PD_PUBLIC void pd_text_style_set_back_color(pd_text_style_t *ts,
                                            pd_color_t color);

/** Set the style to use the default font */
PD_PUBLIC int pd_text_style_set_default_font(pd_text_style_t *ts);

/** Get the name and attributes of a style open tag from a string */
PD_PUBLIC const wchar_t *pd_scan_style_open_tag(const wchar_t *wstr,
                                                wchar_t *name, int max_name_len,
                                                wchar_t *data);

/** Get a style close tag from a string, the tag name is output */
PD_PUBLIC const wchar_t *pd_scan_style_close_tag(const wchar_t *wstr,
                                                 wchar_t *name);

PD_PUBLIC void pd_style_tags_clear(list_t *tags);

PD_PUBLIC pd_text_style_t *pd_style_tags_get_text_style(list_t *tags);

/** Handle a style open tag */
PD_PUBLIC const wchar_t *pd_style_tags_next_open_tag(list_t *tags,
                                                     const wchar_t *str);

/** Handle a style close tag */
PD_PUBLIC const wchar_t *pd_style_tags_next_close_tag(list_t *tags,
                                                      const wchar_t *str);

PD_END_DECLS

#endif
