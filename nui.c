#include "ngl.h"
#include "nui.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void nui_init(void) {
    ngl_init();
    ctx.arena.data = NULL;
    ctx.arena.used = 0;
    ctx.arena.capacity = 0;
    ctx.memory.malloc = malloc;
    ctx.memory.free = free;
    ctx.root.parent = NULL;
    ctx.root.children = NULL;
    ctx.root.children_count = 0;
}

void nui_fini(void) {
    ngl_fini();
    ctx.memory.free(ctx.arena.data);
}

void nui_frame(void) {
    ctx.arena.used = 0;
    ctx.root.children = NULL;
    ctx.root.children_count = 0;
}

void nui_viewport(int width, int height) {
    ctx.root.w = width;
    ctx.root.h = height;
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
    ctx.current->w = width;
}

void nui_fixed_height(int height) {
    ctx.current->h = height;
}

void nui_fixed(int width, int height) {
    nui_fixed_width(width);
    nui_fixed_height(height);
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

void _nui_update_element(struct nui_element *el) {
    if (el->parent) {
        el->x += el->parent->padding.left;
        el->y += el->parent->padding.top;
    }

    for (size_t i = 0; i < el->children_count; i++) {
        _nui_update_element(el->children[i]);
    }
}

void nui_update(void) {
    _nui_update_element(&ctx.root);
}

void _nui_render_element(const struct nui_element *el) {
    uint32_t color = 00000000;
    if (el->style.flags & NUI_STYLE_FLAG_BACKGROUND_COLOR) {
        color = el->style.background_color;
    }

    ngl_draw_rect(el->x, el->y, el->w, el->h, color);

    for (size_t i = 0; i < el->children_count; i++) {
        _nui_render_element(el->children[i]);
    }
}

void nui_render(void) {
    ngl_prepare(ctx.root.w, ctx.root.h);
    _nui_render_element(&ctx.root);
}
