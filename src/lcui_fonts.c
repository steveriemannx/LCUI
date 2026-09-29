/*
 * src/lcui_fonts.c
 *
 * Copyright (c) 2024-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include <ui.h>
#include <ptk.h>
#include <pandagl.h>

bool lcui_fonts_set_default(const char *family_name)
{
        int id = pd_font_get_id(family_name, PD_FONT_STYLE_NORMAL,
                                PD_FONT_WEIGHT_NORMAL);
        if (id < 0) {
                return false;
        }
        pd_font_set_default(id);
        pd_font_family_set_alias("system-ui", family_name);
        return true;
}

#ifdef PTK_WIN32
static void lcui_windows_fonts_init(void)
{
        size_t i;
        const char *fonts[] = { "C:/Windows/Fonts/consola.ttf",
                                "C:/Windows/Fonts/consolai.ttf",
                                "C:/Windows/Fonts/consolab.ttf",
                                "C:/Windows/Fonts/consolaz.ttf",
                                "C:/Windows/Fonts/simsun.ttc",
                                "C:/Windows/Fonts/msyh.ttf",
                                "C:/Windows/Fonts/msyh.ttc",
                                "C:/Windows/Fonts/msyhbd.ttc",
                                "C:/Windows/Fonts/msyhl.ttc" };

        for (i = 0; i < sizeof(fonts) / sizeof(char *); ++i) {
                pd_font_load_file(fonts[i]);
        }
        lcui_fonts_set_default("Microsoft YaHei");
        pd_font_family_set_alias("serif", "Simsun");
        pd_font_family_set_alias("sans-serif", "Microsoft YaHei");
        pd_font_family_set_alias("monospace", "Consolas");
}

#elif defined(PTK_MACOS)

static void lcui_macos_fonts_init(void)
{
        bool has_default = false;
        const char *family_name = "inconsolata";
#ifdef PANDAGL_HAS_FREETYPE
        static const struct {
                const char *path;
                const char *family_name;
        } fonts[] = { { "/System/Library/Fonts/SFNS.ttf", "System Font" },
                      { "/System/Library/Fonts/Supplemental/Arial.ttf",
                        "Arial" } };
        size_t i;

        for (i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i) {
                if (pd_font_load_file(fonts[i].path) == 0 &&
                    lcui_fonts_set_default(fonts[i].family_name)) {
                        family_name = fonts[i].family_name;
                        has_default = true;
                        break;
                }
        }
#endif
        if (!has_default) {
                lcui_fonts_set_default(family_name);
        }
        pd_font_family_set_alias("serif", family_name);
        pd_font_family_set_alias("sans-serif", family_name);
        pd_font_family_set_alias("system-ui", family_name);
        pd_font_family_set_alias("monospace", family_name);
}

#else

#ifdef HAVE_FONTCONFIG

static void lcui_fc_fonts_init(void)
{
        size_t i;
        bool has_default = false;
        char *path, *family;
        const char *fonts[] = { "Ubuntu", "Noto Sans CJK SC",
                                "WenQuanYi Micro Hei" };

        for (i = 0; i < sizeof(fonts) / sizeof(char *); ++i) {
                path = pd_font_find_path(fonts[i]);
                pd_font_load_file(path);
                free(path);
                if (has_default) {
                        continue;
                }
                // TODO: 使用系统已设置的默认字体
                // fontconfig 会把字族名匹配到系统上实际存在的字体，其字族名
                // 不一定和请求的名称相同，所以要用匹配结果的字族名来设置默认
                // 字体，否则默认字体一直是内置的位图字体。
                family = pd_font_find_family(fonts[i]);
                has_default =
                    lcui_fonts_set_default(family ? family : fonts[i]);
                free(family);
        }
}

#else

static void lcui_linux_fonts_init(void)
{
        size_t i;
        const char *fonts[] = {
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-BI.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-C.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-LI.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-L.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-MI.ttf",
                "/usr/share/fonts/truetype/ubuntu/UbuntuMono-BI.ttf",
                "/usr/share/fonts/truetype/ubuntu/UbuntuMono-B.ttf",
                "/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf",
                "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-M.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-RI.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-R.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-RI.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-B.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-BI.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-M.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-MI.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-L.ttf",
                "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-LI.ttf",
                "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                "/usr/share/fonts/opentype/noto/NotoSansCJK.ttc",
                "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc"
        };

        for (i = 0; i < sizeof(fonts) / sizeof(char *); ++i) {
                pd_font_load_file(fonts[i]);
        }
        // TODO: 使用系统已设置的默认字体
        if (!lcui_fonts_set_default("Noto Sans CJK SC")) {
                lcui_fonts_set_default("Ubuntu");
        }
        pd_font_family_set_alias("sans-serif", "Ubuntu");
        pd_font_family_set_alias("monospace", "Ubuntu Mono");
}
#endif

#endif

void lcui_fonts_init(void)
{
#ifdef PTK_WIN32
        lcui_windows_fonts_init();
#elif defined(PTK_MACOS)
        lcui_macos_fonts_init();
#elif defined(HAVE_FONTCONFIG)
        logger_debug("[font] fontconfig enabled\n");
        lcui_fc_fonts_init();
#else
        lcui_linux_fonts_init();
#endif
}
