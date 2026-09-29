/*
 * lib/pandagl/src/background.c: -- canvas background image draw support.
 *
 * Copyright (c) 2018-2026, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <math.h>
#include <yutil.h>
#include <pandagl.h>

void pd_paint_background(pd_context_t *ctx, const pd_background_t *bg,
                         const pd_rect_t *box)
{
        double scale;
        pd_canvas_t canvas, buffer;
        pd_rect_t rect, read_rect;
        int x, y, width, height;

        /* Get the overlap of the draw area and the background content box */
        if (!pd_rect_overlap(box, &ctx->rect, &rect)) {
                return;
        }
        rect.x -= ctx->rect.x;
        rect.y -= ctx->rect.y;
        pd_canvas_init(&buffer);
        pd_canvas_quote(&canvas, &ctx->canvas, &rect);
        pd_canvas_fill(&canvas, bg->color);
        if (!bg->image) {
                return;
        }
        /* Convert coordinates relative to the background content box */
        rect.x += ctx->rect.x - box->x;
        rect.y += ctx->rect.y - box->y;
        /* Save the background image area */
        read_rect.x = x = bg->x;
        read_rect.y = y = bg->y;
        read_rect.width = width = bg->width;
        read_rect.height = height = bg->height;
        /* Get the overlap of the draw area and the background image */
        if (!pd_rect_overlap(&read_rect, &rect, &read_rect)) {
                return;
        }
        /* Convert to coordinates relative to the image */
        read_rect.x -= bg->x;
        read_rect.y -= bg->y;
        /* Quote it directly if the size is unchanged */
        if (bg->width == bg->image->width && bg->height == bg->image->height) {
                pd_canvas_quote(&canvas, bg->image, &read_rect);
        } else {
                rect = read_rect;
                /* Compute the area to quote from the width/height scale */
                if (width != bg->image->width) {
                        scale = 1.0 * bg->image->width / width;
                        rect.x = (int)round(rect.x * scale);
                        rect.width = (int)round(rect.width * scale);
                }
                if (height != bg->image->height) {
                        scale = 1.0 * bg->image->height / height;
                        rect.y = (int)round(rect.y * scale);
                        rect.height = (int)round(rect.height * scale);
                }
                /* Quote an area of the source background image */
                pd_canvas_quote(&canvas, bg->image, &rect);
                width = read_rect.width;
                height = read_rect.height;
                /* Scale it proportionally */
                pd_canvas_zoom(&canvas, &buffer, false, width, height);
                pd_canvas_quote(&canvas, &buffer, NULL);
        }
        /* Compute the coordinates relative to the draw area */
        x += read_rect.x + box->x - ctx->rect.x;
        y += read_rect.y + box->y - ctx->rect.y;
        pd_canvas_mix(&ctx->canvas, &canvas, x, y, bg->color.a < 255);
        pd_canvas_destroy(&buffer);
}
