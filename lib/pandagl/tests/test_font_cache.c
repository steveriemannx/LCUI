/*
 * lib/pandagl/tests/test_font_cache.c
 *
 * Copyright (c) 2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <stdio.h>
#include <string.h>
#include <pandagl.h>
#include <ctest.h>

/*
 * Functional tests for the glyph bitmap cache:
 * - a second request for (face, ch, size) hits the cache, same pointer;
 * - different sizes should yield different entries;
 * - a character that fails to render falls back to .notdef, without crashing.
 */

static void should_return_same_pointer_on_cache_hit(void)
{
        const pd_glyph_bitmap_t *bmp1 = NULL;
        const pd_glyph_bitmap_t *bmp2 = NULL;

        ctest_equal_int("first lookup should succeed",
                        pd_font_cache_get_bitmap('A', -1, 14, &bmp1), 0);
        ctest_equal_int("second lookup should succeed",
                        pd_font_cache_get_bitmap('A', -1, 14, &bmp2), 0);
        ctest_equal_bool("cached bitmaps should share the same address",
                         bmp1 == bmp2, true);
}

static void should_treat_different_sizes_as_different_entries(void)
{
        const pd_glyph_bitmap_t *bmp1 = NULL;
        const pd_glyph_bitmap_t *bmp2 = NULL;

        pd_font_cache_get_bitmap('B', -1, 12, &bmp1);
        pd_font_cache_get_bitmap('B', -1, 24, &bmp2);
        ctest_equal_bool("different sizes should yield different entries",
                         bmp1 != NULL && bmp2 != NULL && bmp1 != bmp2, true);
}

static void should_not_crash_on_unknown_codepoint(void)
{
        const pd_glyph_bitmap_t *bmp = NULL;
        const pd_glyph_bitmap_t *fallback = NULL;
        /* Chars beyond the BMP likely have no glyph, fall back, no crash */
        int ret = pd_font_cache_get_bitmap(0x10FFFD, -1, 14, &bmp);
        ctest_equal_bool("should return non-positive code", ret <= 0, true);
        ret = pd_font_cache_get_bitmap(0x10FFFC, -1, 14, &fallback);
        ctest_equal_bool("should reuse the fallback bitmap", ret <= 0 &&
                                                           fallback == bmp,
                         true);
}

static void should_evict_least_recently_used(void)
{
        const pd_glyph_bitmap_t *bmp = NULL;
        size_t before;

        /* Use a large limit first to warm up several entries */
        pd_font_cache_set_capacity(1024);
        pd_font_cache_get_bitmap('X', -1, 10, &bmp);
        pd_font_cache_get_bitmap('Y', -1, 10, &bmp);
        pd_font_cache_get_bitmap('Z', -1, 10, &bmp);
        before = pd_font_cache_count();
        ctest_equal_bool("should hold at least 3 entries", before >= 3, true);

        /* Shrink the limit to 2, eviction should happen at once */
        pd_font_cache_set_capacity(2);
        ctest_equal_uint("count should drop to capacity",
                         (unsigned)pd_font_cache_count(), 2u);

        /* Adding a new entry must stay within the limit */
        pd_font_cache_get_bitmap('W', -1, 10, &bmp);
        ctest_equal_uint("count should stay at capacity after new insert",
                         (unsigned)pd_font_cache_count(), 2u);

        /* Restore the unlimited capacity */
        pd_font_cache_set_capacity(0);
}

void test_pandagl_font_cache(void)
{
        pd_font_library_init();
        ctest_describe("glyph cache hit returns same pointer",
                       should_return_same_pointer_on_cache_hit);
        ctest_describe("glyph cache differentiates by size",
                       should_treat_different_sizes_as_different_entries);
        ctest_describe("glyph cache handles unknown codepoint",
                       should_not_crash_on_unknown_codepoint);
        ctest_describe("glyph cache lru eviction",
                       should_evict_least_recently_used);
        pd_font_library_destroy();
}
