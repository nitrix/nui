#include "nui.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

static struct {
    struct {
        struct nui_element *active_elements_first;
        struct nui_element *active_elements_last;
        struct nui_element *inactive_elements_first;
    } pool;
    struct {
        void *(*malloc)(size_t size);
        void (*free)(void *ptr);
    } memory;
    struct nui_backend *backend;
    struct nui_element root;
    struct nui_element *current;
} ctx;

struct nui_element *_new_element(void) {
    if (ctx.pool.inactive_elements_first) {
        struct nui_element *el = ctx.pool.inactive_elements_first;
        ctx.pool.inactive_elements_first = el->pool_next;
        memset(el, 0, sizeof *el);
        el->pool_next = ctx.pool.active_elements_first;
        ctx.pool.active_elements_first = el;
        if (!ctx.pool.active_elements_last) {
            ctx.pool.active_elements_last = el;
        }
        return el;
    }

    struct nui_element *el = ctx.memory.malloc(sizeof *el);
    memset(el, 0, sizeof *el);
    el->pool_next = ctx.pool.active_elements_first;
    ctx.pool.active_elements_first = el;
    if (!ctx.pool.active_elements_last) {
        ctx.pool.active_elements_last = el;
    }

    return el;
}

void nui_init(struct nui_backend *backend) {
    ctx.pool.active_elements_first = NULL;
    ctx.pool.active_elements_last = NULL;
    ctx.pool.inactive_elements_first = NULL;
    ctx.memory.malloc = malloc;
    ctx.memory.free = free;
    ctx.root.parent = NULL;
    ctx.root.first_child = NULL;
    ctx.root.last_child = NULL;
    ctx.root.children_count = 0;
    ctx.backend = backend;
    ctx.backend->init();
}

void nui_fini(void) {
    for (struct nui_element *el = ctx.pool.active_elements_first; el != NULL; ) {
        struct nui_element *next = el->pool_next;
        ctx.memory.free(el);
        el = next;
    }

    for (struct nui_element *el = ctx.pool.inactive_elements_first; el != NULL; ) {
        struct nui_element *next = el->pool_next;
        ctx.memory.free(el);
        el = next;
    }

    ctx.backend->fini();
}

void nui_frame(void) {
    ctx.root.first_child = NULL;
    ctx.root.last_child = NULL;
    ctx.root.children_count = 0;

    if (ctx.pool.active_elements_last) {
        ctx.pool.active_elements_last->pool_next = ctx.pool.inactive_elements_first;
        ctx.pool.inactive_elements_first = ctx.pool.active_elements_first;
        ctx.pool.active_elements_first = NULL;
        ctx.pool.active_elements_last = NULL;
    }
}

void nui_viewport(int width, int height) {
    ctx.current = &ctx.root;
    nui_fixed(width, height);
    ctx.current = NULL;
}

void nui_custom_memory(void *(*custom_malloc)(size_t size), void (*custom_free)(void *ptr)) {
    ctx.memory.malloc = custom_malloc;
    ctx.memory.free = custom_free;
}

void nui_element_begin(void) {
    struct nui_element *el = _new_element();

    el->parent = ctx.current;
    ctx.current = el;

    struct nui_element *target = (el->parent ? el->parent : &ctx.root);

    if (target->last_child) {
        target->last_child->next = el;
        target->last_child = el;
    } else {
        target->first_child = el;
        target->last_child = el;
    }

    target->children_count++;
}

void nui_element_end(void) {
    assert(ctx.current);
    ctx.current = ctx.current->parent;
}

void nui_fixed_width(int width) {
    ctx.current->fixed.width = width;
}

void nui_fixed_height(int height) {
    ctx.current->fixed.height = height;
}

void nui_fixed(int width, int height) {
    nui_fixed_width(width);
    nui_fixed_height(height);
}

void nui_grow_width(void) {
    ctx.current->flags |= NUI_ELEMENT_FLAG_GROW_WIDTH;
}

void nui_grow_height(void) {
    ctx.current->flags |= NUI_ELEMENT_FLAG_GROW_HEIGHT;
}

void nui_grow(void) {
    nui_grow_width();
    nui_grow_height();
}

void nui_background_color(uint32_t color) {
    ctx.current->flags |= NUI_ELEMENT_FLAG_HAS_BACKGROUND_COLOR;
    ctx.current->style.background_color = color;
}

void nui_background_image(struct nui_image *image) {
    ctx.current->style.background_image = image;
}

void nui_padding(int top, int right, int bottom, int left) {
    ctx.current->padding.top = top;
    ctx.current->padding.right = right;
    ctx.current->padding.bottom = bottom;
    ctx.current->padding.left = left;
}

void nui_layout(enum nui_layout layout) {
    ctx.current->layout = layout;
}

void nui_child_gap(int gap) {
    ctx.current->child_gap = gap;
}

void _nui_fit_sizing_element(struct nui_element *el, bool xaxis) {
    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_fit_sizing_element(child, xaxis);
    }

    if (xaxis) {
        el->w = el->fixed.width;
    } else {
        el->h = el->fixed.height;
    }

    bool fit = xaxis ? !el->fixed.width : !el->fixed.height;
    if (fit) {
        // Children sizes.
        for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
            if (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT) {
                if (xaxis) {
                    el->w += child->w;
                } else {
                    el->h = MAX(el->h, child->h);
                }
            } else if (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM) {
                if (xaxis) {
                    el->w = MAX(el->w, child->w);
                } else {
                    el->h += child->h;
                }
            }
        }

        // Padding.
        if (xaxis) {
            el->w += el->padding.left + el->padding.right;
        } else {
            el->h += el->padding.top + el->padding.bottom;
        }

        // Gaps.
        if (el->children_count > 0) {
            bool along = xaxis ? (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT) : (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM);
            if (along) {
                if (xaxis) {
                    el->w += el->child_gap * (el->children_count - 1);
                } else {
                    el->h += el->child_gap * (el->children_count - 1);
                }
            }
        }
    }
}

void _nui_find_smallest_growable_along_children(struct nui_element *el, struct nui_element **first_smallest, struct nui_element **second_smallest) {
    *first_smallest = NULL;
    *second_smallest = NULL;

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        bool grow_width = (child->flags & NUI_ELEMENT_FLAG_GROW_WIDTH) != 0;
        bool grow_height = (child->flags & NUI_ELEMENT_FLAG_GROW_HEIGHT) != 0;

        bool growing_along_axis = el->layout == NUI_LAYOUT_LEFT_TO_RIGHT ? grow_width : grow_height;

        if (!growing_along_axis) {
            continue;
        }

        if (*first_smallest == NULL) {
            *first_smallest = child;
            continue;
        }

        bool smaller_than_first = el->layout == NUI_LAYOUT_LEFT_TO_RIGHT ? (child->w < (*first_smallest)->w) : (child->h < (*first_smallest)->h);

        if (smaller_than_first) {
            *second_smallest = *first_smallest;
            *first_smallest = child;
            continue;
        }

        if (*second_smallest == NULL) {
            *second_smallest = child;
            continue;
        }

        bool smaller_than_second = el->layout == NUI_LAYOUT_LEFT_TO_RIGHT ? (child->w < (*second_smallest)->w) : (child->h < (*second_smallest)->h);

        if (smaller_than_second) {
            *second_smallest = child;
            continue;
        }
    }
}

void _nui_distribute_remaining_space_among_growable_elements(struct nui_element *el, int remaining, bool xaxis) {
    while (remaining > 0) {
        // Find the smallest and next smallest growable element.
        struct nui_element *first_smallest = NULL;
        struct nui_element *second_smallest = NULL;
        _nui_find_smallest_growable_along_children(el, &first_smallest, &second_smallest);

        if (first_smallest != NULL && second_smallest != NULL) {
            int diff = xaxis ? (second_smallest->w - first_smallest->w) : (second_smallest->h - first_smallest->h);
            if (diff > 0) {
                int give = MIN(diff, remaining);
                if (xaxis) {
                    first_smallest->w += give;
                } else {
                    first_smallest->h += give;
                }
                remaining -= give;
            } else {
                size_t target_size = xaxis ? first_smallest->w : first_smallest->h;

                // Count how many have the same size.
                size_t peers_count = 0;
                for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
                    bool grow_width = (child->flags & NUI_ELEMENT_FLAG_GROW_WIDTH) != 0;
                    bool grow_height = (child->flags & NUI_ELEMENT_FLAG_GROW_HEIGHT) != 0;
                    bool grow = xaxis ? grow_width : grow_height;
                    bool same_size = xaxis ? (child->w == target_size) : (child->h == target_size);
                    if (grow && same_size) {
                        peers_count++;
                    }
                }

                int give = MIN(remaining / (int) peers_count, remaining);

                for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
                    bool grow_width = (child->flags & NUI_ELEMENT_FLAG_GROW_WIDTH) != 0;
                    bool grow_height = (child->flags & NUI_ELEMENT_FLAG_GROW_HEIGHT) != 0;
                    bool grow = xaxis ? grow_width : grow_height;
                    bool same_size = xaxis ? (child->w == target_size) : (child->h == target_size);
                    if (grow && same_size) {
                        if (xaxis) {
                            child->w += give;
                        } else {
                            child->h += give;
                        }
                        remaining -= give;
                    }
                }

                // Unfair leftover.
                if (remaining < (int) peers_count) {
                    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
                        if (remaining <= 0) {
                            break;
                        }

                        bool grow_width = (child->flags & NUI_ELEMENT_FLAG_GROW_WIDTH) != 0;
                        bool grow_height = (child->flags & NUI_ELEMENT_FLAG_GROW_HEIGHT) != 0;
                        bool grow = xaxis ? grow_width : grow_height;
                        bool same_size = xaxis ? (child->w == target_size) : (child->h == target_size);
                        if (grow && same_size) {
                            if (xaxis) {
                                child->w++;
                            } else {
                                child->h++;
                            }
                            remaining--;
                        }
                    }
                }
            }
        } else if (first_smallest != NULL) {
            if (xaxis) {
                first_smallest->w += remaining;
            } else {
                first_smallest->h += remaining;
            }
            remaining = 0;
        } else {
            // No more growable children.
            break;
        }
    }
}

void _nui_grow_sizing_element(struct nui_element *el, bool xaxis) {
    bool along = xaxis ? (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT) : (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM);
    bool across = xaxis ? (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM) : (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT);

    int remaining = xaxis ? el->w : el->h;

    // Padding.
    remaining -= xaxis ? (el->padding.left + el->padding.right) : (el->padding.top + el->padding.bottom);

    // Along the axis.
    if (along) {
        // Children sizes.
        for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
            remaining -= child->w;
        }

        // Gaps between children.
        if (el->children_count > 0) {
            remaining -= el->child_gap * (el->children_count - 1);
        }

        // Distribute remaining space along the width.
        _nui_distribute_remaining_space_among_growable_elements(el, remaining, xaxis);
    }

    // Across the axis.
    if (across) {
        for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
            bool grow_width = (child->flags & NUI_ELEMENT_FLAG_GROW_WIDTH) != 0;
            bool grow_height = (child->flags & NUI_ELEMENT_FLAG_GROW_HEIGHT) != 0;
            bool grow = xaxis ? grow_width : grow_height;
            if (grow) {
                if (xaxis) {
                    child->w += remaining - child->w;
                } else {
                    child->h += remaining - child->h;
                }
            }
        }
    }

    // Recursively.
    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_grow_sizing_element(child, xaxis);
    }
}

void _nui_positioning_element(struct nui_element *el, int marker_x, int marker_y) {
    el->x = marker_x;
    el->y = marker_y;

    // Padding.
    marker_x += el->padding.left;
    marker_y += el->padding.top;

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_positioning_element(child, marker_x, marker_y);

        // Layout.
        switch (el->layout) {
            case NUI_LAYOUT_LEFT_TO_RIGHT: marker_x += child->w; break;
            case NUI_LAYOUT_TOP_TO_BOTTOM: marker_y += child->h; break;
        }

        // Child gap.
        switch (el->layout) {
            case NUI_LAYOUT_LEFT_TO_RIGHT: marker_x += el->child_gap; break;
            case NUI_LAYOUT_TOP_TO_BOTTOM: marker_y += el->child_gap; break;
        }
    }
}

void nui_update(void) {
    // Order is VERY important.

    // 1 - Fit sizing widths pass.
    _nui_fit_sizing_element(&ctx.root, true);
    // 2 - Grow and shrink sizing widths pass.
    _nui_grow_sizing_element(&ctx.root, true);
    // _nui_shrink_sizing_element();

    // 3 - Wrap text pass.
    // _nui_wrap_text_pass();

    // 4 - Fit sizing heights pass.
    _nui_fit_sizing_element(&ctx.root, false);
    // 5 - Grow and shrink sizing heights pass.
    // _nui_shrink_sizing_element(&ctx.root, false);
    _nui_grow_sizing_element(&ctx.root, false);

    // 6 - Positioning pass.
    _nui_positioning_element(&ctx.root, 0, 0);
}

void _nui_render_element(struct nui_element *el) {
    uint32_t color = 0x00000000; // Transparent.
    if (el->flags & NUI_ELEMENT_FLAG_HAS_BACKGROUND_COLOR) {
        color = el->style.background_color;
    }

    if (el->style.background_image) {
        ctx.backend->draw_image(el->x, el->y, el->w, el->h, el->style.background_image);
    } else if (color) { // Only draw if color is not fully transparent.
        ctx.backend->draw_rect(el->x, el->y, el->w, el->h, color);
    }

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_render_element(child);
    }
}

void nui_render(void) {
    ctx.backend->before_render(ctx.root.w, ctx.root.h);

    _nui_render_element(&ctx.root);

    ctx.backend->after_render();
}

struct nui_image *nui_load_image_from_file(const char *filename) {
    return ctx.backend->load_image_from_file(filename);
}

void nui_unload_image(struct nui_image *image) {
    ctx.backend->unload_image(image);
}
