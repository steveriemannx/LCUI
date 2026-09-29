/*
 * lib/pandagl/include/pandagl/rect.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef LIB_PANDAGL_INCLUDE_PANDAGL_RECT_H
#define LIB_PANDAGL_INCLUDE_PANDAGL_RECT_H

#include <yutil.h>
#include "common.h"
#include "types.h"

PD_BEGIN_DECLS

PD_INLINE pd_rect_t pd_rect(int x, int y, int width, int height)
{
        pd_rect_t rect = { x, y, width, height };
        return rect;
}

PD_INLINE bool pd_rect_is_include(pd_rect_t *a, pd_rect_t *b)
{
        return (b->x >= a->x && b->x + b->width <= a->x + a->width &&
                b->y >= a->y && b->y + b->height <= a->y + a->height);
}

/** Get the area to crop within the given area from the container size */
PD_PUBLIC pd_rect_t pd_rect_crop(const pd_rect_t *rect, int container_width,
                                 int container_height);

/** Correct the rectangle to stay inside the valid container area */
PD_PUBLIC bool pd_rect_correct(pd_rect_t *rect, int container_width,
                               int container_height);

/** Check whether a rectangle covers another rectangle */
PD_PUBLIC bool pd_rect_is_cover(const pd_rect_t *a, const pd_rect_t *b);

/**
 * Get the overlapping rectangle of two rectangles
 * @param[in] a		Rectangle A
 * @param[in] b		Rectangle B
 * @param[out] out	The rectangle where A and B overlap
 * @returns true if the two rectangles overlap, otherwise false
 */
PD_PUBLIC bool pd_rect_overlap(const pd_rect_t *a, const pd_rect_t *b,
                               pd_rect_t *overlapping_rect);

PD_PUBLIC void pd_rect_split(pd_rect_t *base, pd_rect_t *target,
                             pd_rect_t rects[4]);

PD_INLINE bool pd_rect_is_equal(const pd_rect_t *a, const pd_rect_t *b)
{
        return a->x == b->x && a->y == b->y && a->width == b->width &&
               a->height == b->height;
}

/** Add a dirty rectangle record */
PD_PUBLIC int pd_rects_add(list_t *list, pd_rect_t *rect);

/** Remove a dirty rectangle */
PD_PUBLIC int pd_rects_remove(list_t *list, pd_rect_t *rect);

PD_PUBLIC void pd_rects_clear(list_t *list);

PD_PUBLIC int pd_rect_to_str(pd_rect_t *rect, char *str, unsigned max_len);

PD_END_DECLS

#endif
