#ifndef NUI_H
#define NUI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum nui_layout {
    NUI_LAYOUT_LEFT_TO_RIGHT,
    NUI_LAYOUT_TOP_TO_BOTTOM,
};

enum nui_style_flags {
    NUI_STYLE_FLAG_BACKGROUND_COLOR = 1,
};

struct nui_element {
    int x, y, w, h; // px
    struct { int width, height; } fixed; // px
    struct { int top, right, bottom, left; } padding; // px
    bool grow_width, grow_height;
    int child_gap; // px
    enum nui_layout layout;
    struct nui_element *parent;
    struct nui_element **children;
    size_t children_count;
    struct {
        uint32_t flags; // enum nui_style_flags
        uint32_t background_color; // RGBA8888
    } style;
};

struct nui_backend {
    void (*init)(void);
    void (*fini)(void);
    void (*prepare)(int width, int height);
    void (*draw_rect)(int x, int y, int w, int h, uint32_t color);
};

// Lifetime.
void nui_init(struct nui_backend *backend);
void nui_fini(void);
void nui_viewport(int width, int height);

// Main loop.
void nui_frame(void);
void nui_update(void);
void nui_render(void);

// Custom memory management.
void nui_custom_memory(void *(*custom_malloc)(size_t size), void (*custom_free)(void *ptr));

// Elements.
void nui_element_begin(struct nui_element *el);
void nui_element_end(void);
#define NUI_ONCE(before, after) for (size_t _t ## __LINE__ = 0; (_t ## __LINE__ < 1 ? (before, 1) : 0); (after, _t ## __LINE__++))
#define NUI NUI_ONCE(nui_element_begin(&(struct nui_element) {0}), nui_element_end())

// Sizing and layout.
void nui_layout(enum nui_layout layout);
void nui_fixed(int width, int height);
void nui_fixed_width(int width);
void nui_fixed_height(int height);
void nui_grow_width(void);
void nui_grow_height(void);
void nui_grow(void);
void nui_padding(int top, int right, int bottom, int left);
void nui_child_gap(int gap);

// Styling.
void nui_background_color(uint32_t color);

#endif
