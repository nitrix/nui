#ifndef LAYOUT_TESTING_H
#define LAYOUT_TESTING_H

#include "nui.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLUE 0x0a6d89ff
#define PINK 0xf1928fff
#define YELLOW 0xfed84dff
#define LIGHT_BLUE 0x5ecbe4ff

struct layout_test_rect {
    int x, y, w, h;
    uint32_t color;
};

static struct {
    struct layout_test_rect rects[32];
    size_t rects_count;
    char text[1024];
    int word_join_extra_width;
} layout_test;

static struct nui_font layout_test_font = { .handle = (void *)1 };

static void layout_test_init(void) {
}

static void layout_test_fini(void) {
}

static void layout_test_before_render(int width, int height) {
    (void)width;
    (void)height;
    layout_test.rects_count = 0;
    layout_test.text[0] = '\0';
}

static void layout_test_after_render(void) {
}

static void layout_test_draw_rect(int x, int y, int w, int h, uint32_t color) {
    assert(layout_test.rects_count < sizeof layout_test.rects / sizeof layout_test.rects[0]);
    layout_test.rects[layout_test.rects_count++] = (struct layout_test_rect) {
        .x = x,
        .y = y,
        .w = w,
        .h = h,
        .color = color,
    };
}

static void layout_test_draw_image(int x, int y, int w, int h, const struct nui_image *image, enum nui_image_mode mode) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)image;
    (void)mode;
}

static void layout_test_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color) {
    (void)font;
    (void)x;
    (void)y;
    (void)color;
    snprintf(layout_test.text, sizeof layout_test.text, "%s", text);
}

static void layout_test_measure_text(const struct nui_font *font, const char *text, int *width, int *height) {
    (void)font;

    int line_width = 0;
    int max_width = 0;
    int lines = 1;

    for (const char *at = text; at && *at; at++) {
        if (*at == '\n') {
            if (line_width > max_width) {
                max_width = line_width;
            }
            line_width = 0;
            lines++;
        } else {
            if (layout_test.word_join_extra_width > 0 && at > text && *at != ' ' && *(at - 1) == ' ') {
                line_width += layout_test.word_join_extra_width;
            }
            line_width += 10;
        }
    }

    if (line_width > max_width) {
        max_width = line_width;
    }

    *width = max_width;
    *height = lines * 10;
}

static struct nui_backend layout_test_backend = {
    .init = layout_test_init,
    .fini = layout_test_fini,
    .before_render = layout_test_before_render,
    .after_render = layout_test_after_render,
    .draw_rect = layout_test_draw_rect,
    .draw_image = layout_test_draw_image,
    .draw_text = layout_test_draw_text,
    .measure_text = layout_test_measure_text,
};

static const struct layout_test_rect *layout_test_find_rect(uint32_t color) {
    for (size_t i = 0; i < layout_test.rects_count; i++) {
        if (layout_test.rects[i].color == color) {
            return &layout_test.rects[i];
        }
    }

    return NULL;
}

#endif
