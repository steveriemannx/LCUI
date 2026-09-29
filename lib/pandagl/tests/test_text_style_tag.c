/*
 * lib/pandagl/tests/test_text_style_tag.c
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
#include <wchar.h>
#include <pandagl.h>
#include <ctest.h>

/*
 * These cases cover how pd_text handles nested [color] / [bgcolor] tags
 * in style_tag mode. The bug was that pd_style_tags_get_text_style used
 * list_each_reverse to walk the tag stack backwards, so nesting picked the
 * oldest (outermost) color instead of the newest (innermost) one. E.g.
 *
 *   [color=#116329]<[color=#0550ae]Field[/color]>[/color]
 *
 * The fore_color of "Field" was #116329 (the outer green) before the
 * fix and should be #0550ae (the inner blue) after it. This is the root
 * cause of the field-basic.tsx rendering issue ("all chars are #116329").
 *
 * The cases assert on the character buffer of pd_text_t
 * (text->lines[0]->string[col]): each char's style->has_fore_color and
 * fore_color.r/g/b. No font rendering, no pd_font_library_init call.
 */

#define GREEN pd_color(255, 0x11, 0x63, 0x29)
#define BLUE pd_color(255, 0x05, 0x50, 0xae)
#define DBLUE pd_color(255, 0x0a, 0x30, 0x69)
#define RED pd_color(255, 0xcf, 0x22, 0x2e)

static void assert_char_fore(pd_text_t *text, int col, pd_color_t expect,
                             const char *scope)
{
        pd_char_t *ch = text->lines[0]->string[col];
        char prefix[64];

        if (!ch->style) {
                snprintf(prefix, sizeof(prefix), "%s[%d] should have style",
                         scope, col);
                ctest_equal_bool(prefix, false, true);
                return;
        }
        snprintf(prefix, sizeof(prefix), "%s[%d] has_fore_color", scope, col);
        ctest_equal_bool(prefix, ch->style->has_fore_color, true);
        snprintf(prefix, sizeof(prefix), "%s[%d] fore.r", scope, col);
        ctest_equal_int(prefix, ch->style->fore_color.r, expect.r);
        snprintf(prefix, sizeof(prefix), "%s[%d] fore.g", scope, col);
        ctest_equal_int(prefix, ch->style->fore_color.g, expect.g);
        snprintf(prefix, sizeof(prefix), "%s[%d] fore.b", scope, col);
        ctest_equal_int(prefix, ch->style->fore_color.b, expect.b);
}

static void assert_char_back(pd_text_t *text, int col, pd_color_t expect,
                             const char *scope)
{
        pd_char_t *ch = text->lines[0]->string[col];
        char prefix[64];

        if (!ch->style) {
                snprintf(prefix, sizeof(prefix), "%s[%d] should have style",
                         scope, col);
                ctest_equal_bool(prefix, false, true);
                return;
        }
        snprintf(prefix, sizeof(prefix), "%s[%d] has_back_color", scope, col);
        ctest_equal_bool(prefix, ch->style->has_back_color, true);
        snprintf(prefix, sizeof(prefix), "%s[%d] back.r", scope, col);
        ctest_equal_int(prefix, ch->style->back_color.r, expect.r);
        snprintf(prefix, sizeof(prefix), "%s[%d] back.g", scope, col);
        ctest_equal_int(prefix, ch->style->back_color.g, expect.g);
        snprintf(prefix, sizeof(prefix), "%s[%d] back.b", scope, col);
        ctest_equal_int(prefix, ch->style->back_color.b, expect.b);
}

static pd_text_t *make_text_with_tag(const wchar_t *wstr)
{
        pd_text_t *text = pd_text_create();
        pd_text_set_style_tag(text, true);
        pd_text_write(text, wstr, NULL);
        return text;
}

static void should_apply_single_color_tag(void)
{
        pd_text_t *text = make_text_with_tag(L"[color=#116329]hello[/color]");

        ctest_equal_int("single color line length", text->lines[0]->length, 5);
        for (int i = 0; i < 5; ++i) {
                assert_char_fore(text, i, GREEN, "single");
        }
        pd_text_destroy(text);
}

static void should_use_innermost_color_in_two_level_nesting(void)
{
        pd_text_t *text = make_text_with_tag(
            L"[color=#116329]<[color=#0550ae]Field[/color]>[/color]");

        ctest_equal_int("two-level line length", text->lines[0]->length, 7);
        assert_char_fore(text, 0, GREEN, "<");
        assert_char_fore(text, 1, BLUE, "Field[0]");
        assert_char_fore(text, 2, BLUE, "Field[1]");
        assert_char_fore(text, 3, BLUE, "Field[2]");
        assert_char_fore(text, 4, BLUE, "Field[3]");
        assert_char_fore(text, 5, BLUE, "Field[4]");
        assert_char_fore(text, 6, GREEN, ">");
        pd_text_destroy(text);
}

static void should_use_innermost_color_in_three_level_nesting(void)
{
        pd_text_t *text = make_text_with_tag(
            L"[color=#116329]A[color=#0550ae]B[color=#0a3069]C"
            L"[/color][/color][/color]");

        ctest_equal_int("three-level line length", text->lines[0]->length, 3);
        assert_char_fore(text, 0, GREEN, "A");
        assert_char_fore(text, 1, BLUE, "B");
        assert_char_fore(text, 2, DBLUE, "C");
        pd_text_destroy(text);
}

static void should_handle_field_basic_flex_1_line(void)
{
        pd_text_t *text = make_text_with_tag(
            L"[color=#116329]<[color=#0550ae]Field[/color] "
            L"[color=#0550ae]className[/color]=[color=#0a3069]"
            L"\"flex-1\"[/color]>[/color]");

        /* Expected character sequence and colors (0-indexed):
         *   '<'         (0)        green
         *   'Field'     (1-5)      blue
         *   ' '         (6)        green
         *   'className' (7-15)     blue
         *   '='         (16)       green
         *   '"'         (17)       dark blue
         *   'flex-1'    (18-23)    dark blue
         *   '"'         (24)       dark blue
         *   '>'         (25)       green
         */
        ctest_equal_int("field basic line length", text->lines[0]->length, 26);

        assert_char_fore(text, 0, GREEN, "<");
        for (int i = 1; i <= 5; ++i) {
                assert_char_fore(text, i, BLUE, "Field");
        }
        assert_char_fore(text, 6, GREEN, "space");
        for (int i = 7; i <= 15; ++i) {
                assert_char_fore(text, i, BLUE, "className");
        }
        assert_char_fore(text, 16, GREEN, "eq");
        for (int i = 17; i <= 24; ++i) {
                assert_char_fore(text, i, DBLUE, "string");
        }
        assert_char_fore(text, 25, GREEN, ">");

        pd_text_destroy(text);
}

static void should_reapply_color_after_sibling_close(void)
{
        pd_text_t *text = make_text_with_tag(
            L"[color=#0550ae]A[/color][color=#cf222e]B[/color]");

        ctest_equal_int("sibling line length", text->lines[0]->length, 2);
        assert_char_fore(text, 0, BLUE, "A");
        assert_char_fore(text, 1, RED, "B");
        pd_text_destroy(text);
}

static void should_use_innermost_bgcolor_in_two_level_nesting(void)
{
        pd_text_t *text = make_text_with_tag(
            L"[bgcolor=#116329]<[bgcolor=#0550ae]X[/bgcolor]>[/bgcolor]");

        ctest_equal_int("nested bgcolor line length", text->lines[0]->length,
                        3);
        assert_char_back(text, 1, BLUE, "X");
        pd_text_destroy(text);
}

static void should_keep_color_and_bgcolor_independent_when_nested(void)
{
        pd_text_t *text = make_text_with_tag(
            L"[bgcolor=#116329][color=#0550ae]X[/color][/bgcolor]");

        ctest_equal_int("bg+fg nested line length", text->lines[0]->length, 1);
        assert_char_fore(text, 0, BLUE, "X.fore");
        assert_char_back(text, 0, GREEN, "X.back");
        pd_text_destroy(text);
}

void test_pandagl_text_style_tag(void)
{
        ctest_describe("single color tag", should_apply_single_color_tag);
        ctest_describe("nested color two levels",
                       should_use_innermost_color_in_two_level_nesting);
        ctest_describe("nested color three levels",
                       should_use_innermost_color_in_three_level_nesting);
        ctest_describe("field basic flex-1 line",
                       should_handle_field_basic_flex_1_line);
        ctest_describe("sibling color reopen",
                       should_reapply_color_after_sibling_close);
        ctest_describe("nested bgcolor two levels",
                       should_use_innermost_bgcolor_in_two_level_nesting);
        ctest_describe("nested color and bgcolor sibling",
                       should_keep_color_and_bgcolor_independent_when_nested);
}
