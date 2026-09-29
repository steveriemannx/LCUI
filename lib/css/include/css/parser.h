/*
 * lib/css/include/css/parser.h: -- CSS parser module
 *
 * Copyright (c) 2018-2026, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef LIBCSS_INCLUDE_CSS_PARSER_H
#define LIBCSS_INCLUDE_CSS_PARSER_H

#include "common.h"
#include "types.h"

#define CSS_PARSER_BUFFER_SIZE 512

LIBCSS_BEGIN_DECLS

typedef enum css_parser_target_t {
        CSS_PARSER_TARGET_NONE,      /**< None */
        CSS_PARSER_TARGET_RULE_NAME, /**< Rule name */
        CSS_PARSER_TARGET_RULE_DATA, /**< Rule data */
        CSS_PARSER_TARGET_SELECTOR,  /**< Selector */
        CSS_PARSER_TARGET_KEY,       /**< Property name */
        CSS_PARSER_TARGET_VALUE,     /**< Property value */
        CSS_PARSER_TARGET_COMMENT,   /**< Comment */
        CSS_PARSER_TARGET_TOTAL_NUM
} css_parser_target_t;

typedef enum css_rule_type_t {
        CSS_RULE_NONE,
        CSS_RULE_FONT_FACE, /**< @font-face */
        CSS_RULE_IMPORT,    /**< @import */
        CSS_RULE_MEDIA,     /**< @media */
        CSS_RULE_TOTAL_NUM
} css_rule_type_t;

typedef struct css_parser css_parser_t;
typedef struct css_style_parser css_style_parser_t;
typedef int (*css_parser_cb)(css_parser_t *);

typedef struct css_rule_parser {
        char name[32];
        void *data;
        css_parser_cb begin;
        css_parser_cb parse;
} css_rule_parser_t;

typedef struct css_style_parser {
        char *dirname; /**< Current directory */
        char *space;   /**< Space the style declarations belong to */
        char *property;

        void (*style_handler)(int, css_style_value_t *, void *);
        void *style_handler_arg;

        list_t selectors;        /**< Currently matched selector list */
        css_style_decl_t *style; /**< Currently cached style declarations */
} css_style_parser_t;

typedef struct css_comment_parser {
        /** Whether it is a single-line comment */
        bool is_line_comment;

        /** Previously saved target, restored after the comment is parsed */
        css_parser_target_t prev_target;
} css_comment_parser_t;

/** Environment parameters (context data) of the CSS parser */
struct css_parser {
        int pos;         /**< Index of the string position in the buffer */
        const char *cur; /**< Pointer used to traverse the string */
        char *space;     /**< Space the style declarations belong to */
        char *buffer;
        size_t buffer_size;

        css_parser_target_t target;
        css_comment_parser_t comment_parser;
        css_style_parser_t style_parser;
        css_rule_type_t rule;
        css_rule_parser_t rule_parsers[CSS_RULE_TOTAL_NUM];
};

// css parser

LIBCSS_INLINE void css_parser_get_char(css_parser_t *parser)
{
        parser->buffer[parser->pos++] = *(parser->cur);
}

LIBCSS_PUBLIC css_parser_t *css_parser_create(const char *space);

LIBCSS_PUBLIC void css_parser_destroy(css_parser_t *parser);

LIBCSS_PUBLIC void css_parser_end_parse_rule(css_parser_t *parser);

LIBCSS_PUBLIC void css_parser_commit(css_parser_t *parser);

LIBCSS_PUBLIC size_t css_parser_parse(css_parser_t *parser, const char *str);

LIBCSS_PUBLIC int css_parser_begin_parse_comment(css_parser_t *parser);

// css style parser

LIBCSS_PUBLIC void css_style_parser_init(css_style_parser_t *parser,
                                         const char *space);

LIBCSS_PUBLIC void css_style_parser_destroy(css_style_parser_t *parser);

LIBCSS_PUBLIC void css_style_parser_commit(css_style_parser_t *parser);

// css property parser

LIBCSS_PUBLIC void css_init_preset_property_parsers(void);

LIBCSS_PUBLIC void css_destroy_preset_property_parsers(void);

// css font face parser

LIBCSS_PUBLIC void css_font_face_parser_on_load(
    css_parser_t *ctx, void (*func)(const css_font_face_t *));

LIBCSS_PUBLIC int css_font_face_parser_init(css_parser_t *ctx);

LIBCSS_PUBLIC void css_font_face_parser_destroy(css_parser_t *ctx);

LIBCSS_END_DECLS

#endif
