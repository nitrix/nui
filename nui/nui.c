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
        char *data;
        size_t used;
        size_t capacity;
        void *last_ptr;
        size_t last_size;
    } arena;
    struct {
        void *(*malloc)(size_t size);
        void (*free)(void *ptr);
    } memory;
    struct nui_backend *backend;
    struct nui_element root;
    struct nui_element *current;
} ctx;

void *_arena_realloc(void *old, size_t size) {
    // Round up to nearest multiple of the largest alignment requirement.
    size_t alignment = sizeof (max_align_t);
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    // Brand new allocation.
    if (old == NULL) {
        if (aligned_size > ctx.arena.capacity) {
            size_t new_capacity = ctx.arena.capacity == 0 ? 1024 : ctx.arena.capacity;
            while (new_capacity < aligned_size) {
                new_capacity *= 2;
            }
            char *new_data = ctx.memory.malloc(new_capacity);
            if (ctx.arena.data) {
                memcpy(new_data, ctx.arena.data, ctx.arena.used);
                ctx.memory.free(ctx.arena.data);
            }
            ctx.arena.data = new_data;
            ctx.arena.capacity = new_capacity;
        }

        void *new = ctx.arena.data + ctx.arena.used;

        ctx.arena.used += aligned_size;
        ctx.arena.last_ptr = new;
        ctx.arena.last_size = size;

        return new;
    }

    // Reallocation of the most recent allocation.
    if (old == ctx.arena.last_ptr) {
        // Pretend the old allocation is gone.
        ctx.arena.used = (char *) old - ctx.arena.data;

        // Keep the old data for the size that we do share in common.
        size_t copy_size = size < ctx.arena.last_size ? size : ctx.arena.last_size;
        ctx.arena.used += copy_size;

        // Allocate for the remaining size.
        size_t remaining_size = size > copy_size ? size - copy_size : 0;
        if (remaining_size) {
            _arena_realloc(NULL, remaining_size);
        }

        ctx.arena.last_ptr = old;
        ctx.arena.last_size = size;

        return old;
    }

    // Reallocation of an older allocation: abandon it and make a new one.
    return _arena_realloc(NULL, size);
}

void nui_init(struct nui_backend *backend) {
    ctx.arena.data = NULL;
    ctx.arena.used = 0;
    ctx.arena.capacity = 0;
    ctx.memory.malloc = malloc;
    ctx.memory.free = free;
    ctx.root.parent = NULL;
    ctx.root.children = NULL;
    ctx.root.children_count = 0;
    ctx.backend = backend;
    ctx.backend->init();
}

void nui_fini(void) {
    ctx.memory.free(ctx.arena.data);
    ctx.backend->fini();
}

void nui_frame(void) {
    ctx.arena.used = 0;
    ctx.root.children = NULL;
    ctx.root.children_count = 0;
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

void nui_element_begin(struct nui_element *el) {
    assert(el);

    el->parent = ctx.current;
    ctx.current = el;

    struct nui_element *target = (el->parent ? el->parent : &ctx.root);

    target->children = _arena_realloc(target->children, sizeof target->children_count + 1);
    target->children[target->children_count] = el;
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
    ctx.current->grow_width = true;
}

void nui_grow_height(void) {
    ctx.current->grow_height = true;
}

void nui_grow(void) {
    nui_grow_width();
    nui_grow_height();
}

void nui_background_color(uint32_t color) {
    ctx.current->style.background_color = color;
    ctx.current->style.flags |= NUI_STYLE_FLAG_BACKGROUND_COLOR;
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

void _nui_fit_sizing_pass_element(struct nui_element *el) {
    for (size_t i = 0; i < el->children_count; i++) {
        struct nui_element *child = el->children[i];
        _nui_fit_sizing_pass_element(child);
    }

    if (el->fixed.width > 0) el->w = el->fixed.width;
    if (el->fixed.height > 0) el->h = el->fixed.height;

    bool fit_width = el->fixed.width == 0;
    bool fit_height = el->fixed.height == 0;

    if (fit_width) {
        // Children sizes.
        for (size_t i = 0; i < el->children_count; i++) {
            struct nui_element *child = el->children[i];
            switch (el->layout) {
                case NUI_LAYOUT_LEFT_TO_RIGHT: el->w += child->w; break;
                case NUI_LAYOUT_TOP_TO_BOTTOM: el->w = MAX(el->w, child->w); break;
            }
        }

        // Padding.
        el->w += el->padding.left + el->padding.right;

        // Gaps.
        if (el->children_count > 0) {
            if (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT) {
                el->w += el->child_gap * (el->children_count - 1);
            }
        }
    }

    if (fit_height) {
        // Children sizes.
        for (size_t i = 0; i < el->children_count; i++) {
            struct nui_element *child = el->children[i];
            switch (el->layout) {
                case NUI_LAYOUT_LEFT_TO_RIGHT: el->h = MAX(el->h, child->h); break;
                case NUI_LAYOUT_TOP_TO_BOTTOM: el->h += child->h; break;
            }
        }

        // Padding.
        el->h += el->padding.top + el->padding.bottom;

        // Gaps.
        if (el->children_count > 0) {
            if (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM) {
                el->h += el->child_gap * (el->children_count - 1);
            }
        }
    }
}

void _nui_find_smallest_growable_along_children(struct nui_element *el, struct nui_element **first_smallest, struct nui_element **second_smallest) {
    *first_smallest = NULL;
    *second_smallest = NULL;

    for (size_t i = 0; i < el->children_count; i++) {
        struct nui_element *child = el->children[i];

        bool growing_along_axis = el->layout == NUI_LAYOUT_LEFT_TO_RIGHT ? child->grow_width : child->grow_height;

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

void _nui_grow_sizing_pass_element(struct nui_element *el) {
    bool xaxis = el->layout == NUI_LAYOUT_LEFT_TO_RIGHT;

    int remaining_along = xaxis ? el->w : el->h;
    int remaining_across = xaxis ? el->h : el->w;

    // Children sizes.
    for (size_t i = 0; i < el->children_count; i++) {
        struct nui_element *child = el->children[i];
        remaining_along -= xaxis ? child->w : child->h;
    }

    // Padding.
    if (xaxis) {
        remaining_along -= (el->padding.left + el->padding.right);
        remaining_across -= (el->padding.top + el->padding.bottom);
    } else {
        remaining_along -= (el->padding.top + el->padding.bottom);
        remaining_across -= (el->padding.left + el->padding.right);
    }

    // Gaps.
    if (el->children_count > 0) {
        remaining_along -= el->child_gap * (el->children_count - 1);
    }

    // Distribute remaining space along the layout axis.
    while (remaining_along > 0) {
        // Find the smallest and next smallest growable element.
        struct nui_element *first_smallest = NULL;
        struct nui_element *second_smallest = NULL;
        _nui_find_smallest_growable_along_children(el, &first_smallest, &second_smallest);

        if (first_smallest != NULL && second_smallest != NULL) {
            int diff = xaxis ? (second_smallest->w - first_smallest->w) : (second_smallest->h - first_smallest->h);
            if (diff > 0) {
                int give = MIN(diff, remaining_along);
                if (xaxis) {
                    first_smallest->w += give;
                } else {
                    first_smallest->h += give;
                }
                remaining_along -= give;
            } else {
                size_t target_size = xaxis ? first_smallest->w : first_smallest->h;

                // Count how many have the same size.
                size_t peers_count = 0;
                for (size_t i = 0; i < el->children_count; i++) {
                    struct nui_element *child = el->children[i];

                    bool growing_along_axis = xaxis ? child->grow_width : child->grow_height;
                    if (!growing_along_axis) {
                        continue;
                    }

                    bool same_size = xaxis ? (child->w == target_size) : (child->h == target_size);
                    if (same_size) {
                        peers_count++;
                    }
                }

                int give = MIN(remaining_along / (int) peers_count, remaining_along);

                for (size_t i = 0; i < el->children_count; i++) {
                    struct nui_element *child = el->children[i];

                    bool growing_along_axis = xaxis ? child->grow_width : child->grow_height;
                    if (!growing_along_axis) {
                        continue;
                    }

                    bool same_size = xaxis ? (child->w == target_size) : (child->h == target_size);
                    if (same_size) {
                        if (xaxis) {
                            child->w += give;
                        } else {
                            child->h += give;
                        }
                        remaining_along -= give;
                    }
                }

                // Unfair leftover.
                if (remaining_along < (int) peers_count) {
                    for (size_t i = 0; i < el->children_count && remaining_along > 0; i++) {
                        struct nui_element *child = el->children[i];

                        bool growing_along_axis = xaxis ? child->grow_width : child->grow_height;
                        if (!growing_along_axis) {
                            continue;
                        }

                        bool same_size = xaxis ? (child->w == target_size) : (child->h == target_size);
                        if (same_size) {
                            if (xaxis) {
                                child->w++;
                            } else {
                                child->h++;
                            }
                            remaining_along--;
                        }
                    }
                }
            }
        } else if (first_smallest != NULL) {
            if (xaxis) {
                first_smallest->w += remaining_along;
            } else {
                first_smallest->h += remaining_along;
            }
            remaining_along = 0;
        } else {
            // No more growable children.
            break;
        }
    }

    // Distribute remaining space across the layout axis.
    for (size_t i = 0; i < el->children_count; i++) {
        struct nui_element *child = el->children[i];

        bool growing_across = xaxis ? child->grow_height : child->grow_width;
        if (!growing_across) {
            continue;
        }

        remaining_across -= xaxis ? child->h : child->w;

        if (xaxis) {
            child->h += remaining_across;
        } else {
            child->w += remaining_across;
        }
    }

    // Recursively.
    for (size_t i = 0; i < el->children_count; i++) {
        struct nui_element *child = el->children[i];
        _nui_grow_sizing_pass_element(child);
    }
}

void _nui_positioning_pass_element(struct nui_element *el) {
    for (size_t i = 0; i < el->children_count; i++) {
        struct nui_element *child = el->children[i];
        _nui_positioning_pass_element(child);
    }
}

void nui_update(void) {
    // Order is VERY important.
    _nui_fit_sizing_pass_element(&ctx.root);
    _nui_grow_sizing_pass_element(&ctx.root);
    _nui_positioning_pass_element(&ctx.root);
}

void _nui_render_element(const struct nui_element *el, int offset_x, int offset_y) {
    uint32_t color = 00000000;
    if (el->style.flags & NUI_STYLE_FLAG_BACKGROUND_COLOR) {
        color = el->style.background_color;
    }

    ctx.backend->draw_rect(offset_x + el->x, offset_y + el->y, el->w, el->h, color);

    // Padding.
    offset_x += el->padding.left;
    offset_y += el->padding.top;

    for (size_t i = 0; i < el->children_count; i++) {
        struct nui_element *child = el->children[i];

        _nui_render_element(child, offset_x, offset_y);

        // Layout.
        switch (el->layout) {
            case NUI_LAYOUT_LEFT_TO_RIGHT: offset_x += child->w; break;
            case NUI_LAYOUT_TOP_TO_BOTTOM: offset_y += child->h; break;
        }

        // Child gap.
        switch (el->layout) {
            case NUI_LAYOUT_LEFT_TO_RIGHT: offset_x += el->child_gap; break;
            case NUI_LAYOUT_TOP_TO_BOTTOM: offset_y += el->child_gap; break;
        }
    }
}

void nui_render(void) {
    ctx.backend->prepare(ctx.root.w, ctx.root.h);
    _nui_render_element(&ctx.root, 0, 0);
}
