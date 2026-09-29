/*
 * lib/ui-xml/include/ui_xml.h
 *
 * Copyright (c) 2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

/*
 * lib/ui-xml/include/ui_xml.h: -- The GUI build module, parse UI config code
 * and build UI.
 * Copyright (c) 2018-2026, Liu chao <lc-soft@live.cn> All
 * rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part
 * of LCUI, distributed under the MIT License found in the
 * LICENSE.TXT file
 * in the root directory of this source tree.
 */

#ifndef LIB_UI_XML_INCLUDE_XML_H
#define LIB_UI_XML_INCLUDE_XML_H

#include "ui_xml/common.h"
#include <ui.h>

LIBUI_XML_BEGIN_DECLS

/**
 * Load UI configuration code from a string, parse it and create the
 * corresponding UI elements
 * @param[in] str string containing the UI configuration code
 * @return a widget on success, or NULL on error
 */
LIBUI_XML_PUBLIC ui_widget_t *ui_load_xml_string(const char *str, int size);

/**
 * Load UI configuration code from a file, parse it and create the
 * corresponding UI elements
 * @param[in] filepath file path
 * @return a widget on success, or NULL on error
 */
LIBUI_XML_PUBLIC ui_widget_t *ui_load_xml_file(const char *filepath);

LIBUI_XML_END_DECLS

#endif
