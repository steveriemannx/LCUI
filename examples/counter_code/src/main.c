/*
 * examples/counter_code/src/main.c: -- a counter that fills its window.
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

/*
 * Same counter as the one in examples/counter, also built entirely in code,
 * but laid out differently: there is no backdrop and no floating card. A
 * single panel paints the whole window white and centres its content, so the
 * window has no margin or border of its own.
 */

#include <stdio.h>
#include <wchar.h>
#include <LCUI.h>

#define COUNTER_STEP 1
#define COUNTER_MAX 99
#define COUNTER_MIN (-99)

static ui_widget_t *counter_display;
static int counter_value;

/**
 * Apply one CSS declaration to a widget.
 * ui_widget_set_style_string() reports unknown properties and values that it
 * cannot parse, so the result is worth checking while building an interface
 * by hand.
 */
static void set_style(ui_widget_t *w, const char *property, const char *value)
{
        if (ui_widget_set_style_string(w, property, value) != 0) {
                logger_error("[counter_code] unsupported style %s: %s\n", property,
                             value);
        }
}

static ui_widget_t *create_text(const wchar_t *content, const char *font_size,
                                const char *color)
{
        ui_widget_t *text = ui_create_widget("text");

        ui_text_set_content_w(text, content);
        set_style(text, "font-size", font_size);
        set_style(text, "color", color);
        return text;
}

static void counter_refresh(void)
{
        wchar_t text[16];

        swprintf(text, 16, L"%d", counter_value);
        ui_text_set_content_w(counter_display, text);
}

static void counter_shift(int delta)
{
        counter_value += delta;
        if (counter_value > COUNTER_MAX) {
                counter_value = COUNTER_MAX;
        } else if (counter_value < COUNTER_MIN) {
                counter_value = COUNTER_MIN;
        }
        counter_refresh();
}

static void on_decrement(ui_widget_t *w, ui_event_t *e, void *arg)
{
        counter_shift(-COUNTER_STEP);
}

static void on_increment(ui_widget_t *w, ui_event_t *e, void *arg)
{
        counter_shift(COUNTER_STEP);
}

static ui_widget_t *create_button(const wchar_t *content, const char *color,
                                  ui_event_handler_t on_click)
{
        ui_widget_t *button = ui_create_widget(NULL);

        ui_widget_append(button, create_text(content, "26px", "#ffffff"));
        set_style(button, "width", "52px");
        set_style(button, "height", "52px");
        set_style(button, "border-radius", "26px");
        set_style(button, "background-color", color);
        set_style(button, "display", "flex");
        set_style(button, "align-items", "center");
        set_style(button, "justify-content", "center");
        ui_widget_on(button, "click", on_click, NULL);
        return button;
}

int main(void)
{
        ui_widget_t *panel;
        ui_widget_t *buttons;
        ui_widget_t *root;

        /* The root widget only exists once LCUI is initialised. */
        lcui_init();
        root = ui_root();
        ui_widget_set_title(root, L"Counter");
        /* Nothing sets the window size but the root widget's box. */
        set_style(root, "width", "640px");
        set_style(root, "height", "420px");
        /* No backdrop: the panel below covers the window edge to edge. */
        set_style(root, "background-color", "#ffffff");

        panel = ui_create_widget(NULL);
        set_style(panel, "width", "100%");
        set_style(panel, "height", "100%");
        set_style(panel, "background-color", "#ffffff");
        set_style(panel, "display", "flex");
        set_style(panel, "flex-direction", "column");
        set_style(panel, "align-items", "center");
        set_style(panel, "justify-content", "center");
        ui_root_append(panel);

        ui_widget_append(panel, create_text(L"Counter", "18px", "#5a7a96"));

        counter_display = create_text(L"0", "72px", "#2f4a63");
        set_style(counter_display, "font-weight", "600");
        set_style(counter_display, "margin", "12px 0 28px 0");
        ui_widget_append(panel, counter_display);

        buttons = ui_create_widget(NULL);
        set_style(buttons, "display", "flex");
        set_style(buttons, "column-gap", "18px");
        ui_widget_append(panel, buttons);
        ui_widget_append(buttons, create_button(L"-", "#7f9bb5", on_decrement));
        ui_widget_append(buttons, create_button(L"+", "#4acea3", on_increment));

        ui_widget_append(
            panel, create_text(L"Click the buttons to count", "13px", "#96a8b8"));

        counter_refresh();
        return lcui_main();
}
