#ifndef NUI_H
#define NUI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void nui_init(void);
void nui_fini(void);
void nui_frame(void);
void nui_viewport(int width, int height);

enum nui_style_flags {
    NUI_STYLE_FLAG_BACKGROUND_COLOR = 1,
};

struct nui_element {
    int x, y, w, h;
    struct { int top, right, bottom, left; } padding;
    struct nui_element *parent;
    struct nui_element **children;
    size_t children_count;
    struct {
        uint32_t flags;
        uint32_t background_color;
    } style;
};

void nui_element_begin(struct nui_element *el);
void nui_element_end(void);

#define NUI_ONCE(before, after) \
        for (size_t _t ## __LINE__ = 0; (_t ## __LINE__ < 1 && (before, 1), _t ## __LINE__ < 1); (after, _t ## __LINE__++))

#define NUI \
        NUI_ONCE(nui_element_begin(&(struct nui_element) {0}), nui_element_end())

void nui_fixed(int width, int height);
void nui_fixed_width(int width);
void nui_fixed_height(int height);
void nui_background_color(uint32_t color);
void nui_padding(int top, int right, int bottom, int left);

void nui_update(void);
void nui_render(void);

#endif
