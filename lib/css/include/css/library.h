/*
 * lib/css/include/css/library.h: -- CSS library operation module.
 *
 * Copyright (c) 2022-2026, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef LIBCSS_INCLUDE_CSS_LIBRARY_H
#define LIBCSS_INCLUDE_CSS_LIBRARY_H

#include "common.h"
#include "types.h"

LIBCSS_BEGIN_DECLS

LIBCSS_PUBLIC int css_add_style_decl(css_selector_t *selector,
                                     const css_style_decl_t *in_ss,
                                     const char *space);

/**
 * Find style sheets from the specified group
 * @param[in] group group number
 * @param[in] name selector node name, or NULL to generate one from the node
 * @param[in] s selector
 * @param[out] list found style sheets, sorted by weight in descending order
 */
LIBCSS_PUBLIC int css_query_selector_from_group(int group, const char *name,
                                                const css_selector_t *s,
                                                list_t *list);

LIBCSS_INLINE int css_query_selector(const css_selector_t *s, list_t *list)
{
        return css_query_selector_from_group(0, NULL, s, list);
}

LIBCSS_PUBLIC void css_each_style_rule(void (*callback)(css_style_rule_t *,
                                                        const char *, void *),
                                       void *data);

LIBCSS_PUBLIC css_style_decl_t *css_select_style(const css_selector_t *s);

LIBCSS_PUBLIC css_style_decl_t *css_select_style_with_cache(
    const css_selector_t *s);

LIBCSS_PUBLIC size_t css_get_groups_length(void);

LIBCSS_PUBLIC void css_init_library(void);

LIBCSS_PUBLIC void css_destroy_library(void);

LIBCSS_PUBLIC size_t css_print_style_rules(void);

LIBCSS_PUBLIC size_t css_style_rules_to_string(char *str, size_t max_len);

LIBCSS_END_DECLS

#endif
