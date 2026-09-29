/*
 * strlist.h -- string list
 *
 * Copyright (c) 2018, Liu chao <lc-soft@live.cn>
 * Copyright (c) 2021, Li Zihao <yidianyiko@foxmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of LCUI nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UTIL_STRLIST_H
#define UTIL_STRLIST_H

Y_BEGIN_DECLS

typedef char **strlist_t;

YUTIL_API int strlist_sorted_add(strlist_t *strlist, const char *str);

YUTIL_API int strlist_add_one(strlist_t *strlist, const char *str);

YUTIL_API int strlist_remove_one(strlist_t *strlist, const char *str);

/**
 * Add a string to a string list
 * @param[in][out] strlist The string list
 * @param[in] str The string
 */
YUTIL_API int strlist_add(strlist_t *strlist, const char *str);

/**
 * Check whether a string list contains the specified string
 * @param[in][out] strlist The string list
 * @param[in] str The string
 * @returns Returns 1 if it contains the string, otherwise 0
 */
YUTIL_API int strlist_has(strlist_t strlist, const char *str);

/**
 * Remove the specified string from a string list
 * @param[in][out] strlist The string list
 * @param[in] str The string
 * @returns Returns 1 if the removal succeeds, otherwise 0
 */
YUTIL_API int strlist_remove(strlist_t *strlist, const char *str);

/** Free a string list */
YUTIL_API void strlist_free(strlist_t strs);

Y_END_DECLS

#endif
