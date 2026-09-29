/*
 * lib/ptk/include/ptk/ime.h
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#ifndef PTK_INCLUDE_PLATFORM_IME_H
#define PTK_INCLUDE_PLATFORM_IME_H

#include "types.h"
#include "common.h"

PTK_BEGIN_DECLS

typedef struct ptk_ime_handler {
        bool (*prockey)(int, bool);
        void (*totext)(int);
        bool (*open)(void);
        bool (*close)(void);
        void (*setcaret)(int, int);
} ptk_ime_handler_t;

PTK_PUBLIC int ptk_ime_add(const char *ime_name, ptk_ime_handler_t *handler);

/** Select an input method engine */
PTK_PUBLIC bool ptk_ime_select(int ptk_ime_id);

PTK_PUBLIC bool ptk_ime_select_by_name(const char *name);

/** Tell whether a key code is a character key */
PTK_PUBLIC bool ptk_ime_check_char_key(int key);

/** Switch to the next input method engine */
PTK_PUBLIC void ptk_ime_switch(void);

/** Ask the engine whether it wants to handle a key event */
PTK_PUBLIC bool ptk_ime_process_key(ptk_event_t *e);

/** Commit the composed text to the target */
PTK_PUBLIC int ptk_ime_commit(const wchar_t *str, size_t len);

PTK_PUBLIC void ptk_ime_set_caret(int x, int y);

PTK_END_DECLS

#endif
