/*
 * examples/counter/src/main.c: -- a counter, built entirely in code.
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

/*
 * Unlike the other examples, this one ships no .xml or .css: every widget is
 * created, styled and wired up from C, which makes it the smallest reference
 * for building an interface without any resource files.
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
                logger_error("[counter] unsupported style %s: %s\n", property,
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
        ui_widget_t *card;
        ui_widget_t *buttons;
        ui_widget_t *root;

        /* The root widget only exists once LCUI is initialised. */
        lcui_init();
        root = ui_root();
        ui_widget_set_title(root, L"Counter");
        /* Nothing sets the window size but the root widget's box. */
        set_style(root, "width", "640px");
        set_style(root, "height", "420px");
        set_style(root, "background-color", "#adcdfc");
        set_style(root, "display", "flex");
        set_style(root, "align-items", "center");
        set_style(root, "justify-content", "center");

        card = ui_create_widget(NULL);
        set_style(card, "background-color", "#ffffff");
        set_style(card, "border-radius", "18px");
        set_style(card, "padding", "36px 64px");
        set_style(card, "box-shadow", "0 18px 60px rgba(0, 0, 0, 0.25)");
        set_style(card, "display", "flex");
        set_style(card, "flex-direction", "column");
        set_style(card, "align-items", "center");
        ui_root_append(card);

        ui_widget_append(card, create_text(L"Counter", "18px", "#5a7a96"));

        counter_display = create_text(L"0", "72px", "#2f4a63");
        set_style(counter_display, "font-weight", "600");
        set_style(counter_display, "margin", "12px 0 28px 0");
        ui_widget_append(card, counter_display);

        buttons = ui_create_widget(NULL);
        set_style(buttons, "display", "flex");
        set_style(buttons, "column-gap", "18px");
        ui_widget_append(card, buttons);
        ui_widget_append(buttons, create_button(L"-", "#7f9bb5", on_decrement));
        ui_widget_append(buttons, create_button(L"+", "#4acea3", on_increment));

        ui_widget_append(
            card, create_text(L"Click the buttons to count", "13px", "#96a8b8"));

        counter_refresh();
        return lcui_main();
}
