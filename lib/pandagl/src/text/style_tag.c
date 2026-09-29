/*
 * lib/pandagl/src/text/style_tag.c
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <pandagl.h>
#include <stdio.h>

typedef enum pd_text_style_type_t {
        PD_TEXT_STYLE_TYPE_STYLE,
        PD_TEXT_STYLE_TYPE_BOLD,
        PD_TEXT_STYLE_TYPE_ITALIC,
        PD_TEXT_STYLE_TYPE_SIZE,
        PD_TEXT_STYLE_TYPE_COLOR,
        PD_TEXT_STYLE_TYPE_BG_COLOR,
        PD_TEXT_STYLE_TYPE_TOTAL_NUM
} pd_text_style_type_t;

typedef struct pd_style_tag {
        pd_text_style_type_t id;
        union {
                pd_color_t color;
                int number;
        } data;
} pd_style_tag_t;

void pd_style_tags_clear(list_t *tags)
{
        list_destroy(tags, free);
}

/** Get the current text style */
pd_text_style_t *pd_style_tags_get_text_style(list_t *tags)
{
        int count = 0;
        list_node_t *node;
        pd_style_tag_t *tag;
        pd_text_style_t *style;
        bool found_tags[PD_TEXT_STYLE_TYPE_TOTAL_NUM] = { 0 };

        if (tags->length <= 0) {
                return NULL;
        }
        style = malloc(sizeof(pd_text_style_t));
        pd_text_style_init(style);
        /* Build the current text style from the recorded styles */
        for (list_each(node, tags)) {
                tag = node->data;
                switch (tag->id) {
                case PD_TEXT_STYLE_TYPE_COLOR:
                        if (found_tags[tag->id]) {
                                break;
                        }
                        style->has_fore_color = true;
                        style->fore_color = tag->data.color;
                        found_tags[tag->id] = true;
                        ++count;
                        break;
                case PD_TEXT_STYLE_TYPE_BG_COLOR:
                        if (found_tags[tag->id]) {
                                break;
                        }
                        style->has_back_color = true;
                        style->back_color = tag->data.color;
                        found_tags[tag->id] = true;
                        ++count;
                        break;
                case PD_TEXT_STYLE_TYPE_BOLD:
                        if (found_tags[tag->id]) {
                                break;
                        }
                        found_tags[tag->id] = true;
                        pd_text_style_set_weight(style, PD_FONT_WEIGHT_BOLD);
                        ++count;
                        break;
                case PD_TEXT_STYLE_TYPE_ITALIC:
                        if (found_tags[tag->id]) {
                                break;
                        }
                        found_tags[tag->id] = true;
                        pd_text_style_set_style(style, PD_FONT_STYLE_ITALIC);
                        ++count;
                        break;
                case PD_TEXT_STYLE_TYPE_SIZE:
                        if (found_tags[tag->id]) {
                                break;
                        }
                        style->has_pixel_size = true;
                        style->pixel_size = tag->data.number;
                        found_tags[tag->id] = true;
                        ++count;
                        break;
                default:
                        break;
                }
                if (count == 4) {
                        break;
                }
        }
        if (count == 0) {
                free(style);
                return NULL;
        }
        return style;
}

/** Remove a tag's style data from the queue, only the first match */
static void pd_style_tags_remove(list_t *tags, int id)
{
        pd_style_tag_t *tag;
        list_node_t *node;

        if (tags->length <= 0) {
                return;
        }
        for (list_each(node, tags)) {
                tag = node->data;
                if (tag->id == id) {
                        free(tag);
                        list_delete_node(tags, node);
                        break;
                }
        }
}

/** Get a style close tag from a string, the tag name is output */
const wchar_t *pd_scan_style_close_tag(const wchar_t *wstr, wchar_t *name)
{
        size_t i, j, len;

        len = wcslen(wstr);
        if (wstr[0] != '[' || wstr[1] != '/') {
                return NULL;
        }
        /* Match the tag and get the tag name */
        for (j = 0, i = 2; i < len; ++i) {
                switch (wstr[i]) {
                case ' ':
                        break;
                case ']':
                        ++i;
                        goto end_tag_search;
                default:
                        if (name) {
                                name[j] = wstr[i];
                        }
                        ++j;
                        break;
                }
        }

end_tag_search:;
        if (name) {
                name[j] = 0;
        }
        if (j < 1) {
                return NULL;
        }
        return wstr + i;
}

/** Get the name and attributes of a style open tag from a string */
const wchar_t *pd_scan_style_open_tag(const wchar_t *wstr, wchar_t *name,
                                      int max_name_len, wchar_t *data)
{
        size_t i, j, len;
        bool end_name = false;

        len = wcslen(wstr);
        if (wstr[0] != '<') {
                return NULL;
        }
        /* Match the first half of the tag */
        for (j = 0, i = 1; i < len; ++i) {
                if (wstr[i] == ' ') {
                        /* Name ends if the previous char is not a space */
                        if (i > 0 && wstr[i - 1] != ' ') {
                                end_name = true;
                        }
                        /* The tag name may have spaces at head and tail */
                        if (j == 0 || max_name_len == 0 ||
                            (max_name_len > 0 && end_name)) {
                                continue;
                        }
                        /* The tag name cannot contain spaces in the middle */
                        return NULL;
                }
                /* If the tag name part has been matched completely */
                if (wstr[i] == '=') {
                        ++i;
                        break;
                }
                /* If the tag name has already ended */
                if (end_name) {
                        return NULL;
                }
                if (max_name_len > 0 && data) {
                        name[j] = wstr[i];
                }
                ++j;
        }

        if (data) {
                name[j] = 0;
        }
        /* Get the second half of the tag */
        for (j = 0; i < len; ++i) {
                if (wstr[i] == ' ') {
                        continue;
                }
                /* End of the tag, exit */
                if (wstr[i] == '>') {
                        ++i;
                        break;
                }
                if (data) {
                        /* Save the data inside the tag */
                        data[j] = wstr[i];
                }
                ++j;
        }
        if (data) {
                data[j] = 0;
        }
        if (i >= len) {
                return NULL;
        }
        return wstr + i;
}

/** Get the data of a given style tag from a string */
static const wchar_t *pd_scan_style_tag_by_name(const wchar_t *wstr,
                                                const wchar_t *name, char *data)
{
        size_t i, j, len, tag_len;

        len = wcslen(wstr);
        tag_len = wcslen(name);
        if (wstr[0] != '[') {
                return NULL;
        }
        /* Match the first half of the tag */
        for (j = 0, i = 1; i < len; ++i) {
                if (wstr[i] == ' ') {
                        if (j == 0 || j >= tag_len) {
                                continue;
                        }
                        return NULL;
                }
                if (j < tag_len) {
                        if (wstr[i] == name[j]) {
                                ++j;
                                continue;
                        }
                } else if (wstr[i] == '=') {
                        ++i;
                        break;
                } else if (wstr[i] == ']') {
                        break;
                }
                return NULL;
        }
        /* Get the second half of the tag */
        for (j = 0; i < len; ++i) {
                if (wstr[i] == ' ') {
                        continue;
                }
                /* End of the tag, exit */
                if (wstr[i] == ']') {
                        ++i;
                        break;
                }
                /* Save the data inside the tag */
                data[j] = (char)wstr[i];
                ++j;
        }
        data[j] = 0;
        if (i >= len) {
                return NULL;
        }
        return wstr + i;
}

static bool pd_parse_color(const char *str, pd_color_t *value)
{
        size_t len = strlen(str);
        int r, g, b;

        if (str[0] != '#') {
                return false;
        }
        if (len == 4) {
                len = sscanf(str, "#%1X%1X%1X", &r, &g, &b);
                r *= 255 / 0xf;
                g *= 255 / 0xf;
                b *= 255 / 0xf;
        } else if (len == 7) {
                len = sscanf(str, "#%2X%2X%2X", &r, &g, &b);
        } else {
                return false;
        }
        if (len == 3) {
                value->r = r;
                value->g = g;
                value->b = b;
                value->a = 255;
                return true;
        }
        return false;
}

/** Get the style data from a tag in a string, return a pointer past it */
static const wchar_t *pd_scan_style_tag_by_name_data(const wchar_t *wstr,
                                                     pd_style_tag_t *tag)
{
        const wchar_t *p, *q;
        char tag_data[256];

        p = wstr;
        if ((q = pd_scan_style_tag_by_name(p, L"color", tag_data))) {
                if (!pd_parse_color(tag_data, &tag->data.color)) {
                        return NULL;
                }
                tag->id = PD_TEXT_STYLE_TYPE_COLOR;
                return q;
        }
        if ((q = pd_scan_style_tag_by_name(p, L"bgcolor", tag_data))) {
                if (pd_parse_color(tag_data, &tag->data.color) != 1) {
                        return NULL;
                }
                tag->id = PD_TEXT_STYLE_TYPE_BG_COLOR;
                return q;
        }
        if ((q = pd_scan_style_tag_by_name(p, L"size", tag_data))) {
                if (sscanf(tag_data, "%d", &tag->data.number) != 1) {
                        return NULL;
                }
                tag->id = PD_TEXT_STYLE_TYPE_SIZE;
                return q;
        }
        if ((q = pd_scan_style_tag_by_name(p, L"b", tag_data))) {
                tag->id = PD_TEXT_STYLE_TYPE_BOLD;
                return q;
        }
        if ((q = pd_scan_style_tag_by_name(p, L"i", tag_data))) {
                tag->id = PD_TEXT_STYLE_TYPE_ITALIC;
                return q;
        }
        return NULL;
}

const wchar_t *pd_style_tags_next_open_tag(list_t *tags, const wchar_t *str)
{
        const wchar_t *q;
        pd_style_tag_t *tag;

        tag = malloc(sizeof(pd_style_tag_t));
        q = pd_scan_style_tag_by_name_data(str, tag);
        if (q) {
                /* Add the tag style data to the queue */
                list_insert(tags, 0, tag);
        } else {
                free(tag);
        }
        return q;
}

const wchar_t *pd_style_tags_next_close_tag(list_t *tags, const wchar_t *str)
{
        const wchar_t *p;
        wchar_t tagname[256];
        /* Get the tag name */
        p = pd_scan_style_close_tag(str, tagname);
        if (!p) {
                return NULL;
        }
        /* Remove the corresponding style tag */
        if (wcscmp(tagname, L"color") == 0) {
                pd_style_tags_remove(tags, PD_TEXT_STYLE_TYPE_COLOR);
        } else if (wcscmp(tagname, L"bgcolor") == 0) {
                pd_style_tags_remove(tags, PD_TEXT_STYLE_TYPE_BG_COLOR);
        } else if (wcscmp(tagname, L"size") == 0) {
                pd_style_tags_remove(tags, PD_TEXT_STYLE_TYPE_SIZE);
        } else if (wcscmp(tagname, L"b") == 0) {
                pd_style_tags_remove(tags, PD_TEXT_STYLE_TYPE_BOLD);
        } else if (wcscmp(tagname, L"i") == 0) {
                pd_style_tags_remove(tags, PD_TEXT_STYLE_TYPE_ITALIC);
        } else {
                return NULL;
        }
        return p;
}
