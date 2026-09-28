/*
 * lib/ptk/src/macos/app.m
 *
 * Copyright (c) 2023-2026, Liu Chao <hello@lcui.dev> All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * This file is part of LCUI, distributed under the MIT License found
 * in the LICENSE.TXT file in the root directory of this source tree.
 */

#include "app.h"
#import <AppKit/AppKit.h>
#include <limits.h>
#include <math.h>
#include <pandagl.h>
#include <stdlib.h>
#include <string.h>
#include <yutil.h>
#include "ptk.h"

struct ptk_window {
        NSWindow *handle;
        id delegate;
        int x, y;
        int width, height;
        int logical_width, logical_height;
        CGFloat scale;
        int min_width, min_height;
        int max_width, max_height;
        pd_canvas_t canvas;
        list_t rects;
        list_node_t node;
};

static struct ptk_macos_app {
        bool active;
        bool should_quit;
        int exit_code;
        list_t windows;
        list_t native_listeners;
} macos_app;

static NSMutableArray *macos_windows;
static NSMutableArray *macos_window_delegates;
static id macos_delegate;

static void ptk_macoswindow_on_size(ptk_window_t *wnd, CGFloat width,
                                    CGFloat height);

@interface LCUIAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation LCUIAppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
        return YES;
}
@end

@interface LCUIWindowDelegate : NSObject <NSWindowDelegate> {
       @public
        ptk_window_t *window;
}
@end

@implementation LCUIWindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
        ptk_event_t event = {0};
        event.type = PTK_EVENT_CLOSE;
        event.window = window;
        ptk_process_event(&event);
        return YES;
}

- (void)windowDidMove:(NSNotification *)notification
{
        NSRect frame = [window->handle frame];
        NSScreen *screen = [window->handle screen] ?: [NSScreen mainScreen];
        NSRect screen_frame = [screen frame];
        window->x = (int)frame.origin.x;
        window->y = (int)(screen_frame.size.height - NSMaxY(frame));
        ptk_event_t event = {0};
        event.type = PTK_EVENT_MOVE;
        event.window = window;
        ptk_process_event(&event);
}

- (void)windowDidResize:(NSNotification *)notification
{
        NSView *content_view = [window->handle contentView];
        NSSize size = [content_view frame].size;

        [content_view setBoundsSize:size];
        ptk_macoswindow_on_size(window, size.width, size.height);
}
@end

@interface LCUIView : NSView {
       @public
        ptk_window_t *window;
}
@end

@implementation LCUIView
- (BOOL)isFlipped
{
        return YES;
}

- (BOOL)acceptsFirstResponder
{
        return YES;
}

- (NSPoint)lcuiPointForEvent:(NSEvent *)event
{
        NSPoint point = [self convertPoint:[event locationInWindow]
                                  fromView:nil];
        CGFloat scale = [self.window backingScaleFactor];
        point.x *= scale;
        point.y *= scale;
        return point;
}

- (void)drawRect:(NSRect)dirtyRect
{
        if (!window || !window->canvas.bytes || window->width <= 0 ||
            window->height <= 0) {
                return;
        }
        NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:NULL
                          pixelsWide:window->width
                          pixelsHigh:window->height
                       bitsPerSample:8
                     samplesPerPixel:4
                            hasAlpha:YES
                            isPlanar:NO
                      colorSpaceName:NSDeviceRGBColorSpace
                        bitmapFormat:NSBitmapFormatAlphaFirst
                         bytesPerRow:window->width * 4
                        bitsPerPixel:32];
        if (!bitmap) {
                return;
        }
        const uint8_t *source = window->canvas.bytes;
        uint8_t *pixels = [bitmap bitmapData];
        for (size_t y = 0; y < (size_t)window->height; ++y) {
                size_t src_row = y * window->width * 4;
                size_t dst_row =
                    ((size_t)window->height - y - 1) * window->width * 4;
                for (size_t x = 0; x < (size_t)window->width; ++x) {
                        size_t src = src_row + x * 4;
                        size_t dst = dst_row + x * 4;
                        pixels[dst] = source[src + 3];
                        pixels[dst + 1] = source[src + 2];
                        pixels[dst + 2] = source[src + 1];
                        pixels[dst + 3] = source[src];
                }
        }
        [bitmap drawInRect:NSMakeRect(0, 0, window->logical_width,
                                      window->logical_height)];
}

- (void)mouseMoved:(NSEvent *)event
{
        NSPoint point = [self lcuiPointForEvent:event];
        ptk_event_t e = {0};
        e.type = PTK_EVENT_MOUSEMOVE;
        e.window = window;
        e.mouse.x = (int)point.x;
        e.mouse.y = (int)point.y;
        ptk_process_event(&e);
}

- (void)mouseDown:(NSEvent *)event
{
        [[self window] makeFirstResponder:self];
        NSPoint point = [self lcuiPointForEvent:event];
        ptk_event_t e = {0};
        e.type = PTK_EVENT_MOUSEDOWN;
        e.window = window;
        e.mouse.x = (int)point.x;
        e.mouse.y = (int)point.y;
        e.mouse.button = MOUSE_BUTTON_LEFT;
        ptk_process_event(&e);
}

- (void)mouseUp:(NSEvent *)event
{
        NSPoint point = [self lcuiPointForEvent:event];
        ptk_event_t e = {0};
        e.type = PTK_EVENT_MOUSEUP;
        e.window = window;
        e.mouse.x = (int)point.x;
        e.mouse.y = (int)point.y;
        e.mouse.button = MOUSE_BUTTON_LEFT;
        ptk_process_event(&e);
}

- (void)rightMouseDown:(NSEvent *)event
{
        NSPoint point = [self lcuiPointForEvent:event];
        ptk_event_t e = {0};
        e.type = PTK_EVENT_MOUSEDOWN;
        e.window = window;
        e.mouse.x = (int)point.x;
        e.mouse.y = (int)point.y;
        e.mouse.button = MOUSE_BUTTON_RIGHT;
        ptk_process_event(&e);
}

- (void)rightMouseUp:(NSEvent *)event
{
        NSPoint point = [self lcuiPointForEvent:event];
        ptk_event_t e = {0};
        e.type = PTK_EVENT_MOUSEUP;
        e.window = window;
        e.mouse.x = (int)point.x;
        e.mouse.y = (int)point.y;
        e.mouse.button = MOUSE_BUTTON_RIGHT;
        ptk_process_event(&e);
}

- (void)scrollWheel:(NSEvent *)event
{
        ptk_event_t e = {0};
        e.type = PTK_EVENT_WHEEL;
        e.window = window;
        e.wheel.delta_x = (int)([event scrollingDeltaX] * 20);
        e.wheel.delta_y = (int)([event scrollingDeltaY] * 20);
        e.wheel.delta_mode = APP_WHEEL_DELTA_PIXEL;
        ptk_process_event(&e);
}

- (int)lcuiKeyCode:(unsigned short)keyCode
{
        switch (keyCode) {
                case 0:
                        return KEY_A;
                case 1:
                        return KEY_S;
                case 2:
                        return KEY_D;
                case 3:
                        return KEY_F;
                case 4:
                        return KEY_H;
                case 5:
                        return KEY_G;
                case 6:
                        return KEY_Z;
                case 7:
                        return KEY_X;
                case 8:
                        return KEY_C;
                case 9:
                        return KEY_V;
                case 11:
                        return KEY_B;
                case 12:
                        return KEY_Q;
                case 13:
                        return KEY_W;
                case 14:
                        return KEY_E;
                case 15:
                        return KEY_R;
                case 16:
                        return KEY_Y;
                case 17:
                        return KEY_T;
                case 18:
                        return KEY_1;
                case 19:
                        return KEY_2;
                case 20:
                        return KEY_3;
                case 21:
                        return KEY_4;
                case 22:
                        return KEY_6;
                case 23:
                        return KEY_5;
                case 24:
                        return KEY_EQUAL;
                case 25:
                        return KEY_9;
                case 26:
                        return KEY_7;
                case 27:
                        return KEY_MINUS;
                case 28:
                        return KEY_8;
                case 29:
                        return KEY_0;
                case 30:
                        return KEY_BRACKETRIGHT;
                case 31:
                        return KEY_O;
                case 32:
                        return KEY_U;
                case 33:
                        return KEY_BRACKETLEFT;
                case 34:
                        return KEY_I;
                case 35:
                        return KEY_P;
                case 36:
                        return KEY_ENTER;
                case 37:
                        return KEY_L;
                case 38:
                        return KEY_J;
                case 39:
                        return KEY_APOSTROPHE;
                case 40:
                        return KEY_K;
                case 41:
                        return KEY_SEMICOLON;
                case 42:
                        return KEY_BACKSLASH;
                case 43:
                        return KEY_COMMA;
                case 44:
                        return KEY_SLASH;
                case 45:
                        return KEY_N;
                case 46:
                        return KEY_M;
                case 47:
                        return KEY_PERIOD;
                case 48:
                        return KEY_TAB;
                case 49:
                        return KEY_SPACE;
                case 50:
                        return KEY_GRAVE;
                case 51:
                        return KEY_BACKSPACE;
                case 53:
                        return KEY_ESCAPE;
                case 55:
                case 54:
                case 59:
                case 62:
                        return KEY_CONTROL;
                case 56:
                case 60:
                        return KEY_SHIFT;
                case 57:
                        return KEY_CAPITAL;
                case 58:
                case 61:
                        return KEY_ALT;
                case 123:
                        return KEY_LEFT;
                case 124:
                        return KEY_RIGHT;
                case 125:
                        return KEY_DOWN;
                case 126:
                        return KEY_UP;
                case 115:
                        return KEY_HOME;
                case 119:
                        return KEY_END;
                case 116:
                        return KEY_PAGEUP;
                case 121:
                        return KEY_PAGEDOWN;
                case 117:
                        return KEY_DELETE;
                case 114:
                        return KEY_INSERT;
                default:
                        return (int)keyCode;
        }
}

- (void)keyDown:(NSEvent *)event
{
        NSUInteger flags = [event modifierFlags];
        ptk_event_t e = {0};
        e.type = PTK_EVENT_KEYDOWN;
        e.window = window;
        e.key.code = [self lcuiKeyCode:[event keyCode]];
        e.key.shift_key = (flags & NSEventModifierFlagShift) != 0;
        e.key.ctrl_key = (flags & NSEventModifierFlagControl) != 0;
        e.key.alt_key = (flags & NSEventModifierFlagOption) != 0;
        e.key.meta_key = (flags & NSEventModifierFlagCommand) != 0;
        ptk_process_event(&e);
        NSString *characters = [event characters];
        if ([characters length] > 0 && !e.key.ctrl_key && !e.key.meta_key) {
                NSUInteger length = [characters length];
                NSUInteger i;
                size_t text_length = 0;
                wchar_t *text = calloc(length + 1, sizeof(wchar_t));

                e.type = PTK_EVENT_KEYPRESS;
                e.key.code = (int)[characters characterAtIndex:0];
                ptk_process_event(&e);
                if (!text) {
                        return;
                }
                for (i = 0; i < length; ++i) {
                        unichar ch = [characters characterAtIndex:i];
                        if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < length) {
                                unichar low =
                                    [characters characterAtIndex:i + 1];
                                if (low >= 0xDC00 && low <= 0xDFFF) {
                                        ch = 0x10000 + ((ch - 0xD800) << 10) +
                                             (low - 0xDC00);
                                        ++i;
                                }
                        }
                        text[text_length++] = (wchar_t)ch;
                }
                ptk_ime_commit(text, text_length);
                free(text);
        }
}

- (void)keyUp:(NSEvent *)event
{
        NSUInteger flags = [event modifierFlags];
        ptk_event_t e = {0};
        e.type = PTK_EVENT_KEYUP;
        e.window = window;
        e.key.code = [self lcuiKeyCode:[event keyCode]];
        e.key.shift_key = (flags & NSEventModifierFlagShift) != 0;
        e.key.ctrl_key = (flags & NSEventModifierFlagControl) != 0;
        e.key.alt_key = (flags & NSEventModifierFlagOption) != 0;
        e.key.meta_key = (flags & NSEventModifierFlagCommand) != 0;
        ptk_process_event(&e);
}
@end

static void ptk_macoswindow_on_size(ptk_window_t *wnd, CGFloat width,
                                    CGFloat height)
{
        ptk_event_t event = {0};
        CGFloat scale = [wnd->handle backingScaleFactor];
        int pixel_width, pixel_height;

        if (scale <= 0) {
                scale = 1;
        }
        pixel_width = (int)lrint(width * scale);
        pixel_height = (int)lrint(height * scale);
        if (pixel_width <= 0 || pixel_height <= 0 ||
            (pixel_width == wnd->width && pixel_height == wnd->height &&
             width == wnd->logical_width && height == wnd->logical_height)) {
                return;
        }
        pd_canvas_destroy(&wnd->canvas);
        pd_canvas_init(&wnd->canvas);
        wnd->canvas.color_type = PD_COLOR_TYPE_ARGB;
        if (pd_canvas_create(&wnd->canvas, pixel_width, pixel_height) != 0) {
                return;
        }
        wnd->width = pixel_width;
        wnd->height = pixel_height;
        wnd->logical_width = (int)lrint(width);
        wnd->logical_height = (int)lrint(height);
        wnd->scale = scale;
        event.type = PTK_EVENT_SIZE;
        event.window = wnd;
        event.size.width = pixel_width;
        event.size.height = pixel_height;
        ptk_process_event(&event);
        [(NSView *)[wnd->handle contentView] setNeedsDisplay:YES];
}

static ptk_window_t *ptk_macosapp_get_window(void *handle)
{
        list_node_t *node;

        for (list_each(node, &macos_app.windows)) {
                ptk_window_t *wnd = node->data;
                if (wnd->handle == handle) {
                        return wnd;
                }
        }
        return NULL;
}

static ptk_window_t *ptk_macoswindow_create(const wchar_t *title, int x, int y,
                                            int width, int height,
                                            ptk_window_t *parent)
{
        ptk_window_t *wnd = calloc(1, sizeof(ptk_window_t));
        LCUIView *view;
        NSString *window_title;
        size_t title_len;
        char *utf8_title;
        NSRect rect;
        NSScreen *screen = [NSScreen mainScreen];

        if (!wnd) {
                return NULL;
        }
        if (width <= 0) {
                width = PTK_WINDOW_DEFAULT_WIDTH;
        }
        if (height <= 0) {
                height = PTK_WINDOW_DEFAULT_HEIGHT;
        }
        if (x == PTK_WINDOW_CENTER_X || x == PTK_WINDOW_DEFAULT_X) {
                x = (int)(([screen frame].size.width - width) / 2);
        }
        if (y == PTK_WINDOW_CENTER_Y || y == PTK_WINDOW_DEFAULT_Y) {
                y = (int)(([screen frame].size.height - height) / 2);
        }
        rect = NSMakeRect(x, [screen frame].size.height - y - height, width,
                          height);
        wnd->handle = [[NSWindow alloc]
            initWithContentRect:rect
                      styleMask:NSWindowStyleMaskTitled |
                                NSWindowStyleMaskClosable |
                                NSWindowStyleMaskMiniaturizable |
                                NSWindowStyleMaskResizable
                        backing:NSBackingStoreBuffered
                          defer:NO];
        if (!wnd->handle) {
                free(wnd);
                return NULL;
        }
        title_len = title ? encode_utf8(NULL, title, 0) + 1 : 1;
        utf8_title = malloc(title_len);
        if (utf8_title) {
                if (title) {
                        encode_utf8(utf8_title, title, title_len);
                } else {
                        utf8_title[0] = 0;
                }
                window_title = [NSString stringWithUTF8String:utf8_title];
                if (window_title) {
                        [wnd->handle setTitle:window_title];
                }
                free(utf8_title);
        }
        view = [[LCUIView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
        [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        view->window = wnd;
        [wnd->handle setContentView:view];
        wnd->delegate = [[LCUIWindowDelegate alloc] init];
        ((LCUIWindowDelegate *)wnd->delegate)->window = wnd;
        [wnd->handle setDelegate:wnd->delegate];
        [wnd->handle setAcceptsMouseMovedEvents:YES];
        wnd->logical_width = width;
        wnd->logical_height = height;
        wnd->scale = [wnd->handle backingScaleFactor];
        if (wnd->scale <= 0) {
                wnd->scale = 1;
        }
        wnd->width = (int)lrint(width * wnd->scale);
        wnd->height = (int)lrint(height * wnd->scale);
        wnd->x = x;
        wnd->y = y;
        wnd->min_width = (int)lrint(100 * wnd->scale);
        wnd->min_height = (int)lrint(40 * wnd->scale);
        wnd->max_width = INT_MAX;
        wnd->max_height = INT_MAX;
        [wnd->handle
            setContentMinSize:NSMakeSize(wnd->min_width / wnd->scale,
                                         wnd->min_height / wnd->scale)];
        [wnd->handle
            setContentMaxSize:NSMakeSize(wnd->max_width / wnd->scale,
                                         wnd->max_height / wnd->scale)];
        pd_canvas_init(&wnd->canvas);
        wnd->canvas.color_type = PD_COLOR_TYPE_ARGB;
        if (pd_canvas_create(&wnd->canvas, wnd->width, wnd->height) != 0) {
                wnd->handle = nil;
                wnd->delegate = nil;
                free(wnd);
                return NULL;
        }
        [macos_windows addObject:wnd->handle];
        [macos_window_delegates addObject:wnd->delegate];
        list_create(&wnd->rects);
        wnd->node.data = wnd;
        list_append_node(&macos_app.windows, &wnd->node);
        if (parent) {
                [parent->handle addChildWindow:wnd->handle
                                       ordered:NSWindowAbove];
        }
        return wnd;
}

static void ptk_macoswindow_destroy(ptk_window_t *wnd)
{
        if (!wnd) {
                return;
        }
        list_unlink(&macos_app.windows, &wnd->node);
        [macos_windows removeObject:wnd->handle];
        [macos_window_delegates removeObject:wnd->delegate];
        [wnd->handle setDelegate:nil];
        [wnd->handle close];
        wnd->handle = nil;
        wnd->delegate = nil;
        list_destroy(&wnd->rects, free);
        pd_canvas_destroy(&wnd->canvas);
        free(wnd);
}

static void ptk_macoswindow_show(ptk_window_t *wnd)
{
        [wnd->handle makeKeyAndOrderFront:nil];
}

static void ptk_macoswindow_hide(ptk_window_t *wnd)
{
        [wnd->handle orderOut:nil];
}

static void ptk_macoswindow_activate(ptk_window_t *wnd)
{
        [NSApp activateIgnoringOtherApps:YES];
        [wnd->handle makeKeyAndOrderFront:nil];
}

static void ptk_macoswindow_close(ptk_window_t *wnd)
{
        [wnd->handle performClose:nil];
}

static void ptk_macoswindow_set_title(ptk_window_t *wnd, const wchar_t *title)
{
        size_t len = title ? encode_utf8(NULL, title, 0) + 1 : 1;
        char *text = malloc(len);

        if (!text) {
                return;
        }
        if (title) {
                encode_utf8(text, title, len);
        } else {
                text[0] = 0;
        }
        NSString *string = [NSString stringWithUTF8String:text];
        if (string) {
                [wnd->handle setTitle:string];
        }
        free(text);
}

static void ptk_macoswindow_set_size(ptk_window_t *wnd, int width, int height)
{
        [wnd->handle
            setContentSize:NSMakeSize(width / wnd->scale, height / wnd->scale)];
}

static void ptk_macoswindow_set_position(ptk_window_t *wnd, int x, int y)
{
        NSScreen *screen = [wnd->handle screen] ?: [NSScreen mainScreen];
        NSRect frame = [wnd->handle frame];
        CGFloat scale = wnd->scale > 0 ? wnd->scale : 1;

        frame.origin.x = x / scale;
        frame.origin.y =
            [screen frame].size.height - y / scale - frame.size.height;
        [wnd->handle setFrameOrigin:frame.origin];
}

static void ptk_macoswindow_get_position(ptk_window_t *wnd, int *x, int *y)
{
        NSRect frame = [wnd->handle frame];
        NSScreen *screen = [wnd->handle screen] ?: [NSScreen mainScreen];
        CGFloat scale = wnd->scale > 0 ? wnd->scale : 1;

        if (x) {
                *x = (int)lrint(frame.origin.x * scale);
        }
        if (y) {
                *y = (int)lrint(([screen frame].size.height - NSMaxY(frame)) *
                                scale);
        }
}

static void ptk_macoswindow_set_maximized(ptk_window_t *wnd, bool maximized)
{
        bool is_maximized = [wnd->handle isZoomed];
        if (maximized != is_maximized) {
                [wnd->handle zoom:nil];
        }
}

void ptk_macoswindow_set_fullscreen(ptk_window_t *wnd, bool fullscreen)
{
        bool is_fullscreen =
            ([wnd->handle styleMask] & NSWindowStyleMaskFullScreen) != 0;
        if (fullscreen != is_fullscreen) {
                [wnd->handle toggleFullScreen:nil];
        }
}

unsigned ptk_macoswindow_get_dpi(ptk_window_t *wnd)
{
        return (unsigned)lrint(96 * wnd->scale);
}

static void *ptk_macoswindow_get_handle(ptk_window_t *wnd)
{
        return (__bridge void *)wnd->handle;
}

static int ptk_macoswindow_get_width(ptk_window_t *wnd) { return wnd->width; }

static int ptk_macoswindow_get_height(ptk_window_t *wnd) { return wnd->height; }

static bool ptk_macoswindow_is_maximized(ptk_window_t *wnd)
{
        return [wnd->handle isZoomed];
}

static void ptk_macoswindow_set_min_width(ptk_window_t *wnd, int width)
{
        wnd->min_width = width;
        [wnd->handle setContentMinSize:NSMakeSize(width / wnd->scale,
                                                 wnd->min_height / wnd->scale)];
}

static void ptk_macoswindow_set_min_height(ptk_window_t *wnd, int height)
{
        wnd->min_height = height;
        [wnd->handle setContentMinSize:NSMakeSize(wnd->min_width / wnd->scale,
                                                 height / wnd->scale)];
}

static void ptk_macoswindow_set_max_width(ptk_window_t *wnd, int width)
{
        wnd->max_width = width;
        [wnd->handle setContentMaxSize:NSMakeSize(width / wnd->scale,
                                                 wnd->max_height / wnd->scale)];
}

static void ptk_macoswindow_set_max_height(ptk_window_t *wnd, int height)
{
        wnd->max_height = height;
        [wnd->handle setContentMaxSize:NSMakeSize(wnd->max_width / wnd->scale,
                                                 height / wnd->scale)];
}

static ptk_window_paint_t *ptk_macoswindow_begin_paint(ptk_window_t *wnd,
                                                       pd_rect_t *rect)
{
        pd_context_t *paint = calloc(1, sizeof(pd_context_t));

        if (!paint) {
                return NULL;
        }
        paint->rect = *rect;
        paint->with_alpha = false;
        pd_canvas_init(&paint->canvas);
        pd_rect_correct(&paint->rect, wnd->width, wnd->height);
        pd_canvas_quote(&paint->canvas, &wnd->canvas, &paint->rect);
        pd_canvas_fill(&paint->canvas, pd_rgb(255, 255, 255));
        return paint;
}

static void ptk_macoswindow_end_paint(ptk_window_t *wnd,
                                      ptk_window_paint_t *paint)
{
        pd_rect_t *rect = malloc(sizeof(pd_rect_t));
        if (rect) {
                *rect = paint->rect;
                list_append(&wnd->rects, rect);
        }
        pd_canvas_init(&paint->canvas);
        free(paint);
}

static void ptk_macoswindow_present(ptk_window_t *wnd)
{
        [(NSView *)[wnd->handle contentView] setNeedsDisplay:YES];
        list_destroy(&wnd->rects, free);
        list_create(&wnd->rects);
}

static void ptk_macosapp_present(void)
{
        list_node_t *node;
        for (list_each(node, &macos_app.windows)) {
                ptk_macoswindow_present(node->data);
        }
}

static void ptk_macosapp_exit(int exit_code)
{
        macos_app.exit_code = exit_code;
        macos_app.should_quit = true;
}

static void ptk_macosapp_dispatch_event(NSEvent *native_event)
{
        list_node_t *node;
        ptk_native_event_listener_t *listener;
        int type = (int)[native_event type];

        for (list_each(node, &macos_app.native_listeners)) {
                listener = node->data;
                if (listener->type == type) {
                        listener->handler((__bridge void *)native_event,
                                          listener->data);
                }
        }
}

static int ptk_macosapp_process_events(ptk_process_events_option_t option)
{
        NSDate *deadline;
        NSEvent *event;
        bool wait = option == PTK_PROCESS_EVENTS_UNTIL_QUIT;
        bool process_all = option == PTK_PROCESS_EVENTS_ALL_IF_PRESENT || wait;

        macos_app.should_quit = false;
        do {
                ptk_tick();
                deadline =
                    [NSDate dateWithTimeIntervalSinceNow:wait ? 0.01 : 0];
                event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:deadline
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES];
                if (event) {
                        [NSApp sendEvent:event];
                        [NSApp updateWindows];
                        ptk_macosapp_dispatch_event(event);
                }
                ptk_process_events();
                if (!process_all || macos_app.should_quit) {
                        break;
                }
        } while (
            wait ||
            [NSApp nextEventMatchingMask:NSEventMaskAny
                               untilDate:[NSDate dateWithTimeIntervalSinceNow:0]
                                  inMode:NSDefaultRunLoopMode
                                 dequeue:NO] != nil);
        return macos_app.exit_code;
}

static int ptk_macosapp_add_native_event_listener(
    int type, ptk_native_event_handler_t handler, void *data)
{
        ptk_native_event_listener_t *listener = malloc(sizeof(*listener));
        if (!listener) {
                return -1;
        }
        listener->type = type;
        listener->handler = handler;
        listener->data = data;
        list_append(&macos_app.native_listeners, listener);
        return 0;
}

static int ptk_macosapp_remove_native_event_listener(
    int type, ptk_native_event_handler_t handler)
{
        list_node_t *node;
        for (list_each(node, &macos_app.native_listeners)) {
                ptk_native_event_listener_t *listener = node->data;
                if (listener->type == type && listener->handler == handler) {
                        list_delete_node(&macos_app.native_listeners, node);
                        free(listener);
                        return 0;
                }
        }
        return -1;
}

static int ptk_macosapp_get_screen_width(void)
{
        NSScreen *screen = [NSScreen mainScreen];
        CGFloat scale = [screen backingScaleFactor];
        return (int)lrint([screen frame].size.width * (scale > 0 ? scale : 1));
}

static int ptk_macosapp_get_screen_height(void)
{
        NSScreen *screen = [NSScreen mainScreen];
        CGFloat scale = [screen backingScaleFactor];
        return (int)lrint([screen frame].size.height * (scale > 0 ? scale : 1));
}

static int ptk_macosapp_init(const wchar_t *name)
{
        NSApplication *app = [NSApplication sharedApplication];
        macos_delegate = [[LCUIAppDelegate alloc] init];
        app.delegate = macos_delegate;
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [app activateIgnoringOtherApps:YES];
        macos_windows = [NSMutableArray array];
        macos_window_delegates = [NSMutableArray array];
        list_create(&macos_app.windows);
        list_create(&macos_app.native_listeners);
        macos_app.active = true;
        macos_app.exit_code = 0;
        return 0;
}

static int ptk_macosapp_destroy(void)
{
        list_destroy_without_node(
            &macos_app.windows,
            (list_item_destructor_t)ptk_macoswindow_destroy);
        list_destroy(&macos_app.native_listeners, free);
        [NSApp setDelegate:nil];
        macos_delegate = nil;
        macos_windows = nil;
        macos_window_delegates = nil;
        macos_app.active = false;
        return 0;
}

int ptk_open_uri(const char *uri)
{
        NSString *text = [NSString stringWithUTF8String:uri];
        NSURL *url = text ? [NSURL URLWithString:text] : nil;
        return url && [[NSWorkspace sharedWorkspace] openURL:url] ? 0 : -1;
}

int ptk_macos_clipboard_request_text(ptk_clipboard_callback_t callback,
                                     void *arg)
{
        NSString *value = [[NSPasteboard generalPasteboard]
            stringForType:NSPasteboardTypeString];
        const char *utf8 = value ? [value UTF8String] : "";
        size_t length = decode_utf8(NULL, utf8, 0);
        wchar_t *text = calloc(length + 1, sizeof(wchar_t));
        ptk_clipboard_t clipboard = {0};

        if (!text) {
                return -1;
        }
        decode_utf8(text, utf8, length + 1);
        clipboard.text = text;
        clipboard.len = length;
        callback(&clipboard, arg);
        free(text);
        return 0;
}

int ptk_macos_clipboard_set_text(const wchar_t *text, size_t len)
{
        size_t bytes = encode_utf8(NULL, text, len);
        char *utf8 = malloc(bytes + 1);
        if (!utf8) {
                return -1;
        }
        encode_utf8(utf8, text, len);
        NSString *value = [NSString stringWithUTF8String:utf8];
        free(utf8);
        if (!value) {
                return -1;
        }
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        return [pasteboard setString:value forType:NSPasteboardTypeString] ? 0
                                                                           : -1;
}

void ptk_macosapp_driver_init(ptk_app_driver_t *driver)
{
        memset(driver, 0, sizeof(*driver));
        driver->init = ptk_macosapp_init;
        driver->destroy = ptk_macosapp_destroy;
        driver->process_events = ptk_macosapp_process_events;
        driver->on_event = ptk_macosapp_add_native_event_listener;
        driver->off_event = ptk_macosapp_remove_native_event_listener;
        driver->get_screen_width = ptk_macosapp_get_screen_width;
        driver->get_screen_height = ptk_macosapp_get_screen_height;
        driver->create_window = ptk_macoswindow_create;
        driver->get_window = ptk_macosapp_get_window;
        driver->present = ptk_macosapp_present;
        driver->exit = ptk_macosapp_exit;
}

void ptk_macoswindow_driver_init(ptk_window_driver_t *driver)
{
        memset(driver, 0, sizeof(*driver));
        driver->hide = ptk_macoswindow_hide;
        driver->show = ptk_macoswindow_show;
        driver->activate = ptk_macoswindow_activate;
        driver->close = ptk_macoswindow_close;
        driver->destroy = ptk_macoswindow_destroy;
        driver->set_title = ptk_macoswindow_set_title;
        driver->set_size = ptk_macoswindow_set_size;
        driver->set_position = ptk_macoswindow_set_position;
        driver->get_position = ptk_macoswindow_get_position;
        driver->set_maximized = ptk_macoswindow_set_maximized;
        driver->get_handle = ptk_macoswindow_get_handle;
        driver->get_width = ptk_macoswindow_get_width;
        driver->get_height = ptk_macoswindow_get_height;
        driver->is_maximized = ptk_macoswindow_is_maximized;
        driver->set_min_width = ptk_macoswindow_set_min_width;
        driver->set_min_height = ptk_macoswindow_set_min_height;
        driver->set_max_width = ptk_macoswindow_set_max_width;
        driver->set_max_height = ptk_macoswindow_set_max_height;
        driver->begin_paint = ptk_macoswindow_begin_paint;
        driver->end_paint = ptk_macoswindow_end_paint;
        driver->present = ptk_macoswindow_present;
}
