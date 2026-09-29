/*
 * lib/pandagl/include/pandagl/font.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef LIB_PANDAGL_INCLUDE_FONT_H
#define LIB_PANDAGL_INCLUDE_FONT_H

#include "common.h"

typedef enum pd_font_style_t {
        PD_FONT_STYLE_NORMAL,
        PD_FONT_STYLE_ITALIC,
        PD_FONT_STYLE_OBLIQUE,
        PD_FONT_STYLE_TOTAL_NUM
} pd_font_style_t;

typedef enum pd_font_weight_t {
        PD_FONT_WEIGHT_NONE,
        PD_FONT_WEIGHT_THIN,
        PD_FONT_WEIGHT_EXTRA_LIGHT,
        PD_FONT_WEIGHT_LIGHT,
        PD_FONT_WEIGHT_NORMAL,
        PD_FONT_WEIGHT_MEDIUM,
        PD_FONT_WEIGHT_SEMI_BOLD,
        PD_FONT_WEIGHT_BOLD,
        PD_FONT_WEIGHT_EXTRA_BOLD,
        PD_FONT_WEIGHT_BLACK,
        PD_FONT_WEIGHT_TOTAL_NUM
} pd_font_weight_t;

typedef struct pd_glyph_bitmap {
        int top;   /**< distance from the top border */
        int left;  /**< distance from the left border */
        int width; /**< bitmap width */
        int rows;  /**< bitmap rows */
        int pitch;
        uint8_t *buffer; /**< font bitmap data */
        struct {
                int hori_advance, vert_advance;
                int bbox_width, bbox_height;
                int ascender;
        } metrics;
} pd_glyph_bitmap_t;

typedef struct pd_font_engine pd_font_engine_t;

typedef struct pd_font_face {
        int id;                   /**< font face ID */
        char *style_name;         /**< style name */
        char *family_name;        /**< family name */
        void *data;               /**< related data */
        pd_font_style_t style;    /**< style */
        pd_font_weight_t weight;  /**< weight */
        pd_font_engine_t *engine; /**< the font engine it belongs to */
} pd_font_face_t;

struct pd_font_engine {
        char name[64];
        int (*open)(const char *, pd_font_face_t ***);
        unsigned (*get_glyph_index)(pd_font_face_t *, unsigned);
        int (*render)(pd_glyph_bitmap_t *, unsigned, int, pd_font_face_t *);
        void (*close)(void *);
};

PD_BEGIN_DECLS

/** Draw a font bitmap onto the target image */
PD_PUBLIC int pd_canvas_mix_glyph_bitmap(pd_canvas_t *graph, pd_pos_t pos,
                                         const pd_glyph_bitmap_t *bmp,
                                         pd_color_t color);

PD_PUBLIC char *pd_font_find_path(const char *name);

/**
 * Look up the family name of the font that matches a name.
 * fontconfig may resolve the requested name to a different family, and the
 * default font has to be set with the family that was actually matched,
 * otherwise looking the requested name up finds nothing.
 * Returns NULL when nothing matches; the caller owns the returned string.
 */
PD_PUBLIC char *pd_font_find_family(const char *name);

/**
 * Guess the font weight from the string content
 * Documentation: https://developer.mozilla.org/en-US/docs/Web/CSS/font-weight
 */
PD_PUBLIC pd_font_weight_t pd_font_parse_weight(const char *str);

/**
 * Guess the font style from the string content
 * Documentation: https://developer.mozilla.org/en-US/docs/Web/CSS/font-style
 */
PD_PUBLIC pd_font_style_t pd_font_parse_style(const char *str);

/** Load a font bitmap */
PD_PUBLIC int pd_font_render_glyph(pd_glyph_bitmap_t *buff, unsigned ch,
                                   int font_id, int pixel_size);

/** Release the bitmap buffer returned by pd_font_render_glyph(). */
PD_PUBLIC void pd_glyph_bitmap_destroy(pd_glyph_bitmap_t *bitmap);

/** Register a font family and return its ID */
PD_PUBLIC int pd_font_register(pd_font_face_t *font);

/**
 * Get the ID of a font
 * @param[in] family_name The family name
 * @param[in] style The font style
 * @param[in] weight The font weight, defaults to PD_FONT_WEIGHT_NORMAL if 0
 */
PD_PUBLIC int pd_font_get_id(const char *family_name, pd_font_style_t style,
                             pd_font_weight_t weight);

/**
 * Update the weight of the current fonts
 * @param[in] font_ids The current font ID list
 * @params[in] weight The font weight
 * @params[out] new_font_ids The font ID list after the update
 */
PD_PUBLIC size_t pd_font_replace_weight(const int *font_ids,
                                        pd_font_weight_t weight,
                                        int **new_font_ids);

/**
 * Update the style of the current fonts
 * @param[in] font_ids The current font ID list
 * @params[in] style The font style
 * @params[out] new_font_ids The font ID list after the update
 */
PD_PUBLIC size_t pd_font_replace_style(const int *font_ids,
                                       pd_font_style_t style,
                                       int **new_font_ids);

/**
 * Get the list of font IDs for a family name
 * @param[out] ids The output font ID list
 * @param[in] style The style
 * @param[in] weight The font weight, defaults to PD_FONT_WEIGHT_NORMAL if 0
 * @param[in] names Family names, separated by commas
 * @return The number of font IDs found
 */
PD_PUBLIC unsigned pd_font_query(int **font_ids, pd_font_style_t style,
                                 pd_font_weight_t weight,
                                 const char *const *names);

/** Get the font face for the given font ID */
PD_PUBLIC pd_font_face_t *pd_font_get(int id);

/** Get the default font ID */
PD_PUBLIC int pd_font_get_default(void);

/** Set the default font */
PD_PUBLIC void pd_font_set_default(int id);

/** Set an alias for a font family */
PD_PUBLIC bool pd_font_family_set_alias(const char *alias,
                                        const char *family_name);

/**
 * Add a font bitmap to the font cache
 * @param[in] ch The character code
 * @param[in] font_id The font ID to use
 * @param[in] size The font size (in pixels)
 * @param[out] bmp The font bitmap to add
 * @warning This function only copies bmp into the cache, it does not allocate
 * space to store the bitmap data, so do not free bmp manually afterwards.
 */
PD_PUBLIC pd_glyph_bitmap_t *pd_font_cache_add_bitmap(
    wchar_t ch, int font_id, int size, const pd_glyph_bitmap_t *bmp);

/**
 * Get a font bitmap from the cache
 * @param[in] ch The character code
 * @param[in] font_id The font ID to use
 * @param[in] size The font size (in pixels)
 * @param[out] bmp The output reference to the font bitmap
 * @warning Do not free bmp, bmp is only a reference to the font bitmap in the
 * cache, no new space is allocated to store a copy of the bitmap.
 */
PD_PUBLIC int pd_font_cache_get_bitmap(unsigned ch, int font_id, int size,
                                       const pd_glyph_bitmap_t **bmp);

/**
 * Set the glyph bitmap cache limit (by entry count). 0 means no limit.
 * When the cache exceeds the new limit, evict the extra entries in LRU order.
 */
PD_PUBLIC void pd_font_cache_set_capacity(size_t capacity);

/** The number of entries currently held in the cache */
PD_PUBLIC size_t pd_font_cache_count(void);

/** Load a font into the database */
PD_PUBLIC int pd_font_load_file(const char *filepath);

/** Initialize the font processing module */
PD_PUBLIC void pd_font_library_init(void);

/** Dispose the font processing module */
PD_PUBLIC void pd_font_library_destroy(void);

PD_END_DECLS

#endif
