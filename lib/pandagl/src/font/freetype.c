/*
 * lib/pandagl/src/font/freetype.c
 *
 * Copyright (c) 2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

/*
 * lib/pandagl/src/font/freetype.c: -- The FreeType font-engine support
 * module.
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

#include <pandagl.h>
#include "library.h"
#include "freetype.h"
#ifdef PANDAGL_HAS_FREETYPE
#include <stdlib.h>
#include <errno.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H

#define LCUI_FONT_RENDER_MODE FT_RENDER_MODE_NORMAL
#define LCUI_FONT_LOAD_FALGS (FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT)

static struct {
        FT_Library library;
} freetype;

static int pd_freetype_open(const char *filepath, pd_font_face_t ***outfonts)
{
        FT_Face face;
        pd_font_face_t *font, **fonts;
        int i, err, num_faces;

        err = FT_New_Face(freetype.library, filepath, -1, &face);
        if (err) {
                *outfonts = NULL;
                return -1;
        }
        num_faces = face->num_faces;
        FT_Done_Face(face);
        if (num_faces < 1) {
                return 0;
        }
        fonts = malloc(sizeof(pd_font_face_t *) * num_faces);
        if (!fonts) {
                return -ENOMEM;
        }
        for (i = 0; i < num_faces; ++i) {
                err = FT_New_Face(freetype.library, filepath, i, &face);
                if (err) {
                        fonts[i] = NULL;
                        continue;
                }
                FT_Select_Charmap(face, FT_ENCODING_UNICODE);
                font = pd_font_face_create(face->family_name, face->style_name);
                font->data = face;
                fonts[i] = font;
        }
        *outfonts = fonts;
        return num_faces;
}

static void pd_freetype_close(void *face)
{
        FT_Done_Face(face);
}

/** Convert FT_GlyphSlot data to pd_glyph_bitmap_t */
static size_t convert_glyph(pd_glyph_bitmap_t *bmp, FT_GlyphSlot slot, int mode)
{
        int error;
        size_t size;
        FT_BitmapGlyph bitmap_glyph;
        FT_Glyph glyph;

        /* Extract a glyph image from the glyph slot
         * The FT_Glyph object created must be freed with FT_Done_Glyph */
        error = FT_Get_Glyph(slot, &glyph);
        if (error) {
                return -1;
        }
        /*---------------------- print font info --------------------------
        LOG(" width= %ld,  met->height= %ld\n"
        "horiBearingX = %ld, horiBearingY = %ld, horiAdvance = %ld\n"
        "vertBearingX = %ld, vertBearingY = %ld,  vertAdvance = %ld\n",
        slot->metrics.width>>6, slot->metrics.height>>6,
        slot->metrics.horiBearingX>>6, slot->metrics.horiBearingY>>6,
        slot->metrics.horiAdvance>>6, slot->metrics.vertBearingX>>6,
        slot->metrics.vertBearingY>>6, slot->metrics.vertAdvance>>6 );
        ------------------------------------------------------------*/
        if (glyph->format != FT_GLYPH_FORMAT_BITMAP) {
                error = FT_Glyph_To_Bitmap(&glyph, mode, 0, 1);
                if (error) {
                        return -1;
                }
        }
        bitmap_glyph = (FT_BitmapGlyph)glyph;
        /*
         * Glyph metrics are stored in the FT_Glyph_Metrics struct, they are
         * read via face->glyph->metrics: width, height, bearings, advances.
         * Note: not all fonts contain vertical metrics, when FT_HAS_VERTICAL
         * is false, the values of vertBearingX, vertBearingY and vertAdvance
         * are unreliable, this case is not handled here for now.
         * */
        bmp->top = bitmap_glyph->top;
        bmp->left = bitmap_glyph->left;
        bmp->rows = bitmap_glyph->bitmap.rows;
        bmp->width = bitmap_glyph->bitmap.width;
        bmp->pitch = bmp->width * sizeof(uint8_t);
        bmp->metrics.ascender = slot->face->size->metrics.ascender >> 6;
        bmp->metrics.bbox_width = slot->face->size->metrics.max_advance >> 6;
        bmp->metrics.bbox_height = slot->face->size->metrics.height >> 6;
        bmp->metrics.hori_advance =
            slot->metrics.horiAdvance >> 6; /* horizontal advance */
        bmp->metrics.vert_advance =
            slot->metrics.vertAdvance >> 6; /* vertical advance */
        /* Allocate memory to store the font bitmap */
        size = bmp->rows * bmp->pitch;
        bmp->buffer = (uint8_t *)malloc(size);
        if (!bmp->buffer) {
                FT_Done_Glyph(glyph);
                return -1;
        }

        switch (bitmap_glyph->bitmap.pixel_mode) {
                /* 8-bit grayscale bitmap, copy it directly */
        case FT_PIXEL_MODE_GRAY:
                memcpy(bmp->buffer, bitmap_glyph->bitmap.buffer, size);
                break;
                /* Monochrome bitmap, needs conversion */
        case FT_PIXEL_MODE_MONO: {
                FT_Bitmap bitmap;
                FT_Int x, y;
                uint8_t *bit_ptr, *byte_ptr;

                FT_Bitmap_New(&bitmap);
                /* Convert bitmap_glyph->bitmap to bitmap, 1 byte per pixel */
                FT_Bitmap_Convert(freetype.library, &bitmap_glyph->bitmap,
                                  &bitmap, 1);
                bit_ptr = bitmap.buffer;
                byte_ptr = bmp->buffer;
                for (y = 0; y < bmp->rows; ++y) {
                        for (x = 0; x < bmp->width; ++x) {
                                *byte_ptr = *bit_ptr ? 255 : 0;
                                ++byte_ptr, ++bit_ptr;
                        }
                }
                FT_Bitmap_Done(freetype.library, &bitmap);
                break;
        }
        /* Other pixel modes just get 255 for now, refine when needed */
        default:
                memset(bmp->buffer, 255, size);
                break;
        }
        FT_Done_Glyph(glyph);
        return size;
}

static int pd_freetype_render(pd_glyph_bitmap_t *bmp, unsigned ch,
                              int pixel_size, pd_font_face_t *font)
{
        int ret = 0;
        FT_UInt index;
        FT_Face ft_face = (FT_Face)font->data;

        /* Set the font size */
        FT_Set_Pixel_Sizes(ft_face, 0, pixel_size);
        index = FT_Get_Char_Index(ft_face, ch);
        if (index == 0) {
                ret = -1;
        }
        /* Load the glyph data of this character */
        if (FT_Load_Glyph(ft_face, index, LCUI_FONT_LOAD_FALGS) != 0) {
                return -2;
        }
        convert_glyph(bmp, ft_face->glyph, LCUI_FONT_RENDER_MODE);
        return ret;
}

static unsigned pd_freetype_get_glyph_index(pd_font_face_t *font, unsigned ch)
{
        FT_Face ft_face = (FT_Face)font->data;

        return FT_Get_Char_Index(ft_face, ch);
}

int pd_freetype_engine_init(pd_font_engine_t *engine)
{
        if (FT_Init_FreeType(&freetype.library)) {
                return -1;
        }
        strcpy(engine->name, "FreeType");
        engine->render = pd_freetype_render;
        engine->open = pd_freetype_open;
        engine->close = pd_freetype_close;
        engine->get_glyph_index = pd_freetype_get_glyph_index;
        return 0;
}

int pd_freetype_engine_destroy(void)
{
        FT_Done_FreeType(freetype.library);
        return 0;
}

#endif
