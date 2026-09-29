/*
 * string.h -- The string operation set.
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

#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <wchar.h>
#include <string.h>

Y_BEGIN_DECLS

YUTIL_API char *y_strdup(const char *str);
YUTIL_API wchar_t *y_wcsdup(const wchar_t *str);

YUTIL_API size_t y_strlower(char *outstr, const char *instr);

/**
 * Remove the characters at both ends of a string
 * @param[out] outstr The resulting string
 * @param[in] instr The string to process
 * @param[in] charlist Characters to remove; NULL means blanks are removed
 * @return The length of the resulting string
 */
YUTIL_API size_t y_strtrim(char *outstr, const char *instr,
                           const char *charlist);
YUTIL_API size_t y_wcstrim(wchar_t *outstr, const wchar_t *instr,
                           const wchar_t *charlist);

YUTIL_API unsigned y_strhash(unsigned hash, const char *str);

/**
 * Replace a substring in a string (in place, only the first match)
 * @param[in][out] str The string to process, modified in place on success
 * @param[in] max_len The maximum length of the output string
 * @param[in] substr The substring in the string to be replaced
 * @param[in] newstr The new replacement string
 * @returns The length of the resulting string, or, if nothing was replaced, 0
 */
YUTIL_API size_t y_strreplace(char *str, size_t max_len, const char *substr,
                              const char *newstr);

/**
 * Split a string
 * @param[in] instr The string to split
 * @param[in] sep The delimiter string
 * @param[out] outstrs The resulting string list
 */
YUTIL_API int y_strsplit(const char *instr, const char *sep, char ***outstrs);

Y_END_DECLS

#endif
