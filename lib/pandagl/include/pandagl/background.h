/*
 * lib/pandagl/include/pandagl/background.h
 *
 * Copyright (c) 2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

/*
 * lib/pandagl/include/pandagl/background.h: -- Background image draw
 * support.
 * Copyright (c) 2018-2026, Liu chao <lc-soft@live.cn> All rights
 * reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI,
 * distributed under the MIT License found in the
 * LICENSE.TXT file in the
 * root directory of this source tree.
 */

#ifndef LIB_PANDAGL_INCLUDE_PANDAGL_BACKGROUND_H
#define LIB_PANDAGL_INCLUDE_PANDAGL_BACKGROUND_H

#include "common.h"
#include "types.h"

PD_BEGIN_DECLS

/**
 * Draw the background
 * @param ctx The context handle of the renderer
 * @param[in] box The background area
 * @param[in] bg The background style parameters
 */
PD_PUBLIC void pd_paint_background(pd_context_t *ctx, const pd_background_t *bg,
                                   const pd_rect_t *box);

PD_END_DECLS

#endif
