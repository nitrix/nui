#ifndef NUI_H
#define NUI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum nui_layout {
    NUI_LAYOUT_LEFT_TO_RIGHT,
    NUI_LAYOUT_TOP_TO_BOTTOM,
};

enum nui_element_flags {
    NUI_ELEMENT_FLAG_HAS_BACKGROUND_COLOR = 1,
    NUI_ELEMENT_FLAG_GROW_WIDTH = 2,
    NUI_ELEMENT_FLAG_GROW_HEIGHT = 4,
};

struct nui_image {
    void *handle; // Backend-specific image handle.
    int width;
    int height;
};

struct nui_element {
    // User-defined properties.
    enum nui_layout layout;
    struct { int width, height; } fixed; // px
    struct { int top, right, bottom, left; } padding; // px
    struct { int top, right, bottom, left; } margin; // px
    int child_gap; // px
    enum nui_element_flags flags;
    struct {
        uint32_t background_color; // RGBA8888
        struct nui_image *background_image;
    } style;

    // Position and size which gets computed during the update phase.
    int x, y, w, h; // px

    // Hierarchy.
    // Quite intrusive by having elements point to their parent and siblings directly.
    struct nui_element *parent;
    struct nui_element *first_child;
    struct nui_element *last_child;
    struct nui_element *next;
    size_t children_count;

    // Pool management.
    // Also intrusive, to form the pool's active/inactive element lists.
    struct nui_element *pool_next;
};

struct nui_backend {
    void (*init)(void);
    void (*fini)(void);
    void (*before_render)(int width, int height);
    void (*after_render)(void);
    void (*draw_rect)(int x, int y, int w, int h, uint32_t color);
    void (*draw_image)(int x, int y, int w, int h, struct nui_image *image);
    struct nui_image *(*load_image_from_file)(const char *filename);
    void (*unload_image)(struct nui_image *image);
};

// Lifetime.
void nui_init(struct nui_backend *backend);
void nui_viewport(int width, int height);
void nui_fini(void);

// Main loop.
void nui_frame(void);
void nui_update(void);
void nui_render(void);

// Custom memory management.
void nui_custom_memory(void *(*custom_malloc)(size_t size), void (*custom_free)(void *ptr));

// Elements.
void nui_element_begin(void);
void nui_element_end(void);

// Layout.
void nui_layout(enum nui_layout layout);

// Sizing.
// By default, an element will automatically adapt itself to neatly "fit" all of its children.
// Inversely, a child element can be configured to "grow" as big as possible within its parent,
// sharing available space with other "growing" siblings.
void nui_grow_width(void);
void nui_grow_height(void);
void nui_grow(void);
// An element can be given a fixed size which simultanously acts as the minimum size for the purpose of
// "fitting" and as the maximum size for the purpose of "growing".
void nui_fixed(int width, int height);
void nui_fixed_width(int width);
void nui_fixed_height(int height);

// Spacing.
// Padding adds an offset inside an element along its edges.
void nui_padding(int top, int right, int bottom, int left);
// Margin adds an offset outside an element along its edges.
void nui_margin(int top, int right, int bottom, int left);
// Child gap on a parent element determines the spacing in-between its children.
void nui_child_gap(int gap);

// Styling.
void nui_background_color(uint32_t color);
void nui_background_image(struct nui_image *image);

// Images.
struct nui_image *nui_load_image_from_file(const char *filename);
void nui_unload_image(struct nui_image *image);

// Convenience macros.
#define NUI_ONCE(before, after) for (size_t _t ## __LINE__ = 0; (_t ## __LINE__ < 1 ? (before, 1) : 0); (after, _t ## __LINE__++))
#define NUI NUI_ONCE(nui_element_begin(), nui_element_end())
#define NUI_IMAGE(image) NUI{nui_fixed((image)->width, (image)->height); nui_background_image(image);}

#endif
