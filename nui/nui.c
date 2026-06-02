#include "nui.h"
#include <assert.h>
#include <stdarg.h>
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
    struct nui_input input;
} ctx;

void _init_element(struct nui_element *el) {
    struct nui_element template = {
        .layout = NUI_LAYOUT_LEFT_TO_RIGHT,
        .image_mode = NUI_IMAGE_MODE_REPEAT,
    };

    *el = template;
}

void _free_element_content(struct nui_element *el) {
    if (el->text) {
        ctx.memory.free(el->text);
        el->text = NULL;
    }

    if (el->wrapped_text) {
        ctx.memory.free(el->wrapped_text);
        el->wrapped_text = NULL;
    }
}

const char *_text_for_rendering(const struct nui_element *el) {
    return el->wrapped_text ? el->wrapped_text : el->text;
}

struct nui_element *_new_element(void) {
    if (ctx.pool.inactive_elements_first) {
        struct nui_element *el = ctx.pool.inactive_elements_first;

        ctx.pool.inactive_elements_first = el->pool_next;
        _init_element(el);
        el->pool_next = ctx.pool.active_elements_first;
        ctx.pool.active_elements_first = el;
        if (!ctx.pool.active_elements_last) {
            ctx.pool.active_elements_last = el;
        }

        return el;
    }

    struct nui_element *el = ctx.memory.malloc(sizeof *el);
    _init_element(el);

    el->pool_next = ctx.pool.active_elements_first;
    ctx.pool.active_elements_first = el;
    if (!ctx.pool.active_elements_last) {
        ctx.pool.active_elements_last = el;
    }

    return el;
}

void _pool_init(void) {
    ctx.pool.active_elements_first = NULL;
    ctx.pool.active_elements_last = NULL;
    ctx.pool.inactive_elements_first = NULL;
}

void _pool_reset(void) {
    if (ctx.pool.active_elements_last) {
        for (struct nui_element *el = ctx.pool.active_elements_first; el != NULL; el = el->pool_next) {
            _free_element_content(el);
        }

        ctx.pool.active_elements_last->pool_next = ctx.pool.inactive_elements_first;
        ctx.pool.inactive_elements_first = ctx.pool.active_elements_first;
        ctx.pool.active_elements_first = NULL;
        ctx.pool.active_elements_last = NULL;
    }
}

void _pool_fini(void) {
    for (struct nui_element *el = ctx.pool.active_elements_first; el != NULL; ) {
        struct nui_element *next = el->pool_next;
        _free_element_content(el);
        ctx.memory.free(el);
        el = next;
    }

    for (struct nui_element *el = ctx.pool.inactive_elements_first; el != NULL; ) {
        struct nui_element *next = el->pool_next;
        _free_element_content(el);
        ctx.memory.free(el);
        el = next;
    }
}

void nui_init(struct nui_backend *backend) {
    _pool_init();
    _init_element(&ctx.root);
    ctx.memory.malloc = malloc;
    ctx.memory.free = free;
    ctx.backend = backend;
    ctx.backend->init();
}

void nui_fini(void) {
    _pool_fini();
    ctx.backend->fini();
}

void nui_frame(void) {
    _pool_reset();

    // Reset the children for the root element but keep the rest intact.
    ctx.root.first_child = NULL;
    ctx.root.last_child = NULL;
    ctx.root.children_count = 0;

    ctx.input.mouse_delta_x = 0;
    ctx.input.mouse_delta_y = 0;
    ctx.input.scroll_x = 0;
    ctx.input.scroll_y = 0;
    ctx.input.text_len = 0;
    ctx.input.text[0] = '\0';
    memset(ctx.input.mouse_pressed, 0, sizeof ctx.input.mouse_pressed);
    memset(ctx.input.mouse_released, 0, sizeof ctx.input.mouse_released);
    memset(ctx.input.key_pressed, 0, sizeof ctx.input.key_pressed);
    memset(ctx.input.key_released, 0, sizeof ctx.input.key_released);
}

void nui_viewport(int width, int height) {
    ctx.root.fixed.width = width;
    ctx.root.fixed.height = height;
}

void nui_custom_memory(void *(*custom_malloc)(size_t size), void (*custom_free)(void *ptr)) {
    ctx.memory.malloc = custom_malloc;
    ctx.memory.free = custom_free;
}

void nui_input_mouse_move(int x, int y) {
    ctx.input.mouse_delta_x += x - ctx.input.mouse_x;
    ctx.input.mouse_delta_y += y - ctx.input.mouse_y;
    ctx.input.mouse_x = x;
    ctx.input.mouse_y = y;
}

void nui_input_mouse_button(enum nui_mouse_button button, bool down) {
    if (button < 0 || button >= NUI_MOUSE_BUTTON_COUNT) {
        return;
    }

    if (ctx.input.mouse_down[button] != down) {
        ctx.input.mouse_pressed[button] = down;
        ctx.input.mouse_released[button] = !down;
    }

    ctx.input.mouse_down[button] = down;
}

void nui_input_scroll(float dx, float dy) {
    ctx.input.scroll_x += dx;
    ctx.input.scroll_y += dy;
}

void nui_input_key(enum nui_key key, bool down, enum nui_modifiers modifiers) {
    if (key < 0 || key >= NUI_KEY_COUNT) {
        return;
    }

    if (ctx.input.key_down[key] != down) {
        ctx.input.key_pressed[key] = down;
        ctx.input.key_released[key] = !down;
    }

    ctx.input.key_down[key] = down;
    ctx.input.modifiers = modifiers;
}

void nui_input_text_utf8(const char *text) {
    if (!text) {
        return;
    }

    size_t available = sizeof ctx.input.text - ctx.input.text_len - 1;
    size_t len = strlen(text);
    if (len > available) {
        len = available;
    }

    memcpy(ctx.input.text + ctx.input.text_len, text, len);
    ctx.input.text_len += len;
    ctx.input.text[ctx.input.text_len] = '\0';
}

const struct nui_input *nui_get_input(void) {
    return &ctx.input;
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

const struct nui_element *nui_current_element(void) {
    return ctx.current;
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

void nui_font_color(uint32_t color) {
    ctx.current->flags |= NUI_ELEMENT_FLAG_HAS_FONT_COLOR;
    ctx.current->style.font_color = color;
}

void nui_background_color(uint32_t color) {
    ctx.current->flags |= NUI_ELEMENT_FLAG_HAS_BACKGROUND_COLOR;
    ctx.current->style.background_color = color;
}

void nui_border(uint32_t color, int width) {
    ctx.current->flags |= NUI_ELEMENT_FLAG_HAS_BORDER;
    ctx.current->style.border_color = color;
    ctx.current->style.border_width = width;
}

void nui_corner_radius(int radius) {
    ctx.current->flags |= NUI_ELEMENT_FLAG_HAS_CORNER_RADIUS;
    ctx.current->style.corner_radius = radius;
}

void nui_background_image(const struct nui_image *image) {
    ctx.current->style.background_image = image;
}

void nui_image_mode(enum nui_image_mode mode) {
    ctx.current->image_mode = mode;
}

void nui_padding(int top, int right, int bottom, int left) {
    ctx.current->padding.top = top;
    ctx.current->padding.right = right;
    ctx.current->padding.bottom = bottom;
    ctx.current->padding.left = left;
}

void nui_margin(int top, int right, int bottom, int left) {
    ctx.current->margin.top = top;
    ctx.current->margin.right = right;
    ctx.current->margin.bottom = bottom;
    ctx.current->margin.left = left;
}

void nui_layout(enum nui_layout layout) {
    ctx.current->layout = layout;
}

void nui_child_gap(int gap) {
    ctx.current->child_gap = gap;
}

bool _is_text_separator(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void _measure_text_range(const struct nui_font *font, const char *text, size_t len, int *width, int *height) {
    *width = 0;
    *height = 0;

    if (!font || !text || len == 0 || !ctx.backend->measure_text) {
        return;
    }

    char *buffer = ctx.memory.malloc(len + 1);
    memcpy(buffer, text, len);
    buffer[len] = '\0';

    ctx.backend->measure_text(font, buffer, width, height);
    ctx.memory.free(buffer);
}

int _measure_text_minimum_width(const struct nui_font *font, const char *text) {
    int minimum_width = 0;

    for (const char *at = text; *at; ) {
        while (*at && _is_text_separator(*at)) {
            at++;
        }

        const char *start = at;
        while (*at && !_is_text_separator(*at)) {
            at++;
        }

        size_t len = (size_t)(at - start);
        if (len > 0) {
            int word_width, word_height;
            _measure_text_range(font, start, len, &word_width, &word_height);
            minimum_width = MAX(minimum_width, word_width);
        }
    }

    return minimum_width;
}

int _nui_margin_before(const struct nui_element *el, bool xaxis) {
    return xaxis ? el->margin.left : el->margin.top;
}

int _nui_margin_after(const struct nui_element *el, bool xaxis) {
    return xaxis ? el->margin.right : el->margin.bottom;
}

int _nui_margin_total(const struct nui_element *el, bool xaxis) {
    return _nui_margin_before(el, xaxis) + _nui_margin_after(el, xaxis);
}

int _nui_outer_preferred(const struct nui_element *el, bool xaxis) {
    return (xaxis ? el->preferred.width : el->preferred.height) + _nui_margin_total(el, xaxis);
}

int _nui_outer_minimum(const struct nui_element *el, bool xaxis) {
    return (xaxis ? el->minimum.width : el->minimum.height) + _nui_margin_total(el, xaxis);
}

void _nui_fit_sizing_element(struct nui_element *el, bool xaxis) {
    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_fit_sizing_element(child, xaxis);
    }

    if (xaxis) {
        el->w = el->fixed.width;
        el->preferred.width = el->fixed.width;
        el->minimum.width = el->fixed.width;
    } else {
        el->h = el->fixed.height;
        el->preferred.height = el->fixed.height;
        el->minimum.height = el->fixed.height;
    }

    bool fit = xaxis ? !el->fixed.width : !el->fixed.height;
    if (fit) {
        // Children sizes.
        for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
            if (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT) {
                if (xaxis) {
                    el->preferred.width += _nui_outer_preferred(child, true);
                    el->minimum.width += _nui_outer_minimum(child, true);
                } else {
                    el->preferred.height = MAX(el->preferred.height, _nui_outer_preferred(child, false));
                    el->minimum.height = MAX(el->minimum.height, _nui_outer_minimum(child, false));
                }
            } else if (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM) {
                if (xaxis) {
                    el->preferred.width = MAX(el->preferred.width, _nui_outer_preferred(child, true));
                    el->minimum.width = MAX(el->minimum.width, _nui_outer_minimum(child, true));
                } else {
                    el->preferred.height += _nui_outer_preferred(child, false);
                    el->minimum.height += _nui_outer_minimum(child, false);
                }
            }
        }

        // Gaps.
        if (el->children_count > 0) {
            bool along = xaxis ? (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT) : (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM);
            if (along) {
                if (xaxis) {
                    el->preferred.width += el->child_gap * (el->children_count - 1);
                    el->minimum.width += el->child_gap * (el->children_count - 1);
                } else {
                    el->preferred.height += el->child_gap * (el->children_count - 1);
                    el->minimum.height += el->child_gap * (el->children_count - 1);
                }
            }
        }

        // Text content.
        if (el->text && el->style.font && ctx.backend->measure_text) {
            int text_width, text_height;
            const char *text = xaxis ? el->text : _text_for_rendering(el);
            ctx.backend->measure_text(el->style.font, text, &text_width, &text_height);
            if (xaxis) {
                el->preferred.width = MAX(el->preferred.width, text_width);
                el->minimum.width = MAX(el->minimum.width, _measure_text_minimum_width(el->style.font, el->text));
            } else {
                el->preferred.height = MAX(el->preferred.height, text_height);
                el->minimum.height = MAX(el->minimum.height, text_height);
            }
        }

        // Image content.
        if (el->image) {
            if (xaxis) {
                el->preferred.width = MAX(el->preferred.width, el->image->width);
                el->minimum.width = MAX(el->minimum.width, el->image->width);
            } else {
                el->preferred.height = MAX(el->preferred.height, el->image->height);
                el->minimum.height = MAX(el->minimum.height, el->image->height);
            }
        }

        // Padding.
        if (xaxis) {
            el->preferred.width += el->padding.left + el->padding.right;
            el->minimum.width += el->padding.left + el->padding.right;
        } else {
            el->preferred.height += el->padding.top + el->padding.bottom;
            el->minimum.height += el->padding.top + el->padding.bottom;
        }
    }

    if (xaxis) {
        el->w = el->preferred.width;
    } else {
        el->h = el->preferred.height;
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

int _nui_axis_size(const struct nui_element *el, bool xaxis) {
    return xaxis ? el->w : el->h;
}

int _nui_axis_minimum(const struct nui_element *el, bool xaxis) {
    return xaxis ? el->minimum.width : el->minimum.height;
}

void _nui_set_axis_size(struct nui_element *el, bool xaxis, int size) {
    if (xaxis) {
        el->w = size;
    } else {
        el->h = size;
    }
}

void _nui_shrink_axis_size(struct nui_element *el, bool xaxis, int amount) {
    _nui_set_axis_size(el, xaxis, _nui_axis_size(el, xaxis) - amount);
}

int _nui_content_size(const struct nui_element *el, bool xaxis) {
    int padding = xaxis ? (el->padding.left + el->padding.right) : (el->padding.top + el->padding.bottom);
    return MAX(0, _nui_axis_size(el, xaxis) - padding);
}

void _nui_shrink_children_along_axis(struct nui_element *el, bool xaxis) {
    int used = 0;
    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        used += _nui_axis_size(child, xaxis) + _nui_margin_total(child, xaxis);
    }

    if (el->children_count > 0) {
        used += el->child_gap * (el->children_count - 1);
    }

    int overflow = used - _nui_content_size(el, xaxis);
    while (overflow > 0) {
        int shrinkable_count = 0;
        for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
            if (_nui_axis_size(child, xaxis) > _nui_axis_minimum(child, xaxis)) {
                shrinkable_count++;
            }
        }

        if (shrinkable_count == 0) {
            break;
        }

        int share = MAX(1, (overflow + shrinkable_count - 1) / shrinkable_count);
        bool shrunk = false;
        for (struct nui_element *child = el->first_child; child != NULL && overflow > 0; child = child->next) {
            int room = _nui_axis_size(child, xaxis) - _nui_axis_minimum(child, xaxis);
            if (room <= 0) {
                continue;
            }

            int take = MIN(room, MIN(share, overflow));
            _nui_shrink_axis_size(child, xaxis, take);
            overflow -= take;
            shrunk = true;
        }

        if (!shrunk) {
            break;
        }
    }
}

void _nui_shrink_children_across_axis(struct nui_element *el, bool xaxis) {
    int available = _nui_content_size(el, xaxis);

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        int child_size = _nui_axis_size(child, xaxis) + _nui_margin_total(child, xaxis);
        int child_minimum = _nui_axis_minimum(child, xaxis);
        if (child_size > available) {
            _nui_set_axis_size(child, xaxis, MAX(child_minimum, available - _nui_margin_total(child, xaxis)));
        }
    }
}

void _nui_shrink_sizing_element(struct nui_element *el, bool xaxis) {
    bool along = xaxis ? (el->layout == NUI_LAYOUT_LEFT_TO_RIGHT) : (el->layout == NUI_LAYOUT_TOP_TO_BOTTOM);

    if (along) {
        _nui_shrink_children_along_axis(el, xaxis);
    } else {
        _nui_shrink_children_across_axis(el, xaxis);
    }

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_shrink_sizing_element(child, xaxis);
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
            remaining -= _nui_axis_size(child, xaxis) + _nui_margin_total(child, xaxis);
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
                    child->w += MAX(0, remaining - _nui_margin_total(child, true)) - child->w;
                } else {
                    child->h += MAX(0, remaining - _nui_margin_total(child, false)) - child->h;
                }
            }
        }
    }

    // Recursively.
    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_grow_sizing_element(child, xaxis);
    }
}

char *_nui_wrap_text_to_width(const struct nui_font *font, const char *text, int max_width) {
    size_t len = strlen(text);
    char *wrapped = ctx.memory.malloc(len + 1);
    size_t wrapped_len = 0;
    size_t line_start = 0;
    bool line_has_word = false;

    for (const char *at = text; *at; ) {
        if (*at == '\n') {
            wrapped[wrapped_len++] = '\n';
            at++;
            line_start = wrapped_len;
            line_has_word = false;
            continue;
        }

        if (*at == ' ' || *at == '\t' || *at == '\r') {
            at++;
            continue;
        }

        const char *start = at;
        while (*at && !_is_text_separator(*at)) {
            at++;
        }

        size_t word_len = (size_t)(at - start);

        if (line_has_word) {
            size_t before_word = wrapped_len;
            wrapped[wrapped_len++] = ' ';
            memcpy(wrapped + wrapped_len, start, word_len);
            wrapped_len += word_len;

            int candidate_width, candidate_height;
            _measure_text_range(font, wrapped + line_start, wrapped_len - line_start, &candidate_width, &candidate_height);
            if (max_width > 0 && candidate_width > max_width) {
                wrapped_len = before_word;
                wrapped[wrapped_len++] = '\n';
                line_start = wrapped_len;
                memcpy(wrapped + wrapped_len, start, word_len);
                wrapped_len += word_len;
            }
        } else {
            memcpy(wrapped + wrapped_len, start, word_len);
            wrapped_len += word_len;
        }

        line_has_word = true;
    }

    wrapped[wrapped_len] = '\0';
    return wrapped;
}

void _nui_wrap_text_element(struct nui_element *el) {
    if (el->wrapped_text) {
        ctx.memory.free(el->wrapped_text);
        el->wrapped_text = NULL;
    }

    if (el->text && el->style.font && ctx.backend->measure_text) {
        int text_width, text_height;
        ctx.backend->measure_text(el->style.font, el->text, &text_width, &text_height);

        int content_width = MAX(0, el->w - el->padding.left - el->padding.right);
        if (content_width > 0 && text_width > content_width) {
            el->wrapped_text = _nui_wrap_text_to_width(el->style.font, el->text, content_width);
        }
    }

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_wrap_text_element(child);
    }
}

void _nui_positioning_element(struct nui_element *el, int marker_x, int marker_y) {
    el->x = marker_x;
    el->y = marker_y;

    // Padding.
    marker_x += el->padding.left;
    marker_y += el->padding.top;

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_positioning_element(child, marker_x + child->margin.left, marker_y + child->margin.top);

        // Layout.
        switch (el->layout) {
            case NUI_LAYOUT_LEFT_TO_RIGHT: marker_x += child->margin.left + child->w + child->margin.right; break;
            case NUI_LAYOUT_TOP_TO_BOTTOM: marker_y += child->margin.top + child->h + child->margin.bottom; break;
        }

        // Child gap.
        switch (el->layout) {
            case NUI_LAYOUT_LEFT_TO_RIGHT: marker_x += el->child_gap; break;
            case NUI_LAYOUT_TOP_TO_BOTTOM: marker_y += el->child_gap; break;
        }
    }
}

void _nui_fonting_element(struct nui_element *el, const struct nui_font *font, uint32_t font_color) {
    if (el->style.font) {
        font = el->style.font;
    } else {
        el->style.font = font;
    }

    if (el->flags & NUI_ELEMENT_FLAG_HAS_FONT_COLOR) {
        font_color = el->style.font_color;
    } else {
        el->style.font_color = font_color;
    }

    for (struct nui_element *child = el->first_child; child != NULL; child = child->next) {
        _nui_fonting_element(child, font, font_color);
    }
}

void nui_update(void) {
    // Order is VERY important.

    // 1 - Custom font pass.
    _nui_fonting_element(&ctx.root, NULL, 0x000000000);

    // 2 - Fit sizing widths pass.
    _nui_fit_sizing_element(&ctx.root, true);
    // 3 - Grow and shrink sizing widths pass.
    _nui_grow_sizing_element(&ctx.root, true);
    _nui_shrink_sizing_element(&ctx.root, true);

    // 4 - Wrap text pass.
    _nui_wrap_text_element(&ctx.root);

    // 5 - Fit sizing heights pass.
    _nui_fit_sizing_element(&ctx.root, false);
    // 6 - Grow and shrink sizing heights pass.
    _nui_grow_sizing_element(&ctx.root, false);
    _nui_shrink_sizing_element(&ctx.root, false);

    // 7 - Positioning pass.
    _nui_positioning_element(&ctx.root, 0, 0);
}

void _nui_render_element(struct nui_element *el) {
    uint32_t color = 0x00000000; // Transparent.
    if (el->flags & NUI_ELEMENT_FLAG_HAS_BACKGROUND_COLOR) {
        color = el->style.background_color;
    }

    if (el->style.background_image) {
        ctx.backend->draw_image(el->x, el->y, el->w, el->h, el->style.background_image, el->image_mode);
    } else {
        bool styled = (el->flags & (NUI_ELEMENT_FLAG_HAS_BORDER | NUI_ELEMENT_FLAG_HAS_CORNER_RADIUS)) != 0;
        if (styled && ctx.backend->draw_box) {
            uint32_t border = (el->flags & NUI_ELEMENT_FLAG_HAS_BORDER) ? el->style.border_color : 0x00000000;
            int border_width = (el->flags & NUI_ELEMENT_FLAG_HAS_BORDER) ? el->style.border_width : 0;
            int radius = (el->flags & NUI_ELEMENT_FLAG_HAS_CORNER_RADIUS) ? el->style.corner_radius : 0;
            ctx.backend->draw_box(el->x, el->y, el->w, el->h, color, border, border_width, radius);
        } else if (color) { // Only draw if color is not fully transparent.
            ctx.backend->draw_rect(el->x, el->y, el->w, el->h, color);
        }
    }

    if (el->image) {
        int content_x = el->x + el->padding.left;
        int content_y = el->y + el->padding.top;
        int content_w = MAX(0, el->w - el->padding.left - el->padding.right);
        int content_h = MAX(0, el->h - el->padding.top - el->padding.bottom);
        ctx.backend->draw_image(content_x, content_y, content_w, content_h, el->image, el->image_mode);
    }

    if (el->text) {
        ctx.backend->draw_text(el->style.font, el->x + el->padding.left, el->y + el->padding.top, _text_for_rendering(el), el->style.font_color);
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

struct nui_font *nui_load_font_from_file(const char *filename, float font_size) {
    return ctx.backend->load_font_from_file(filename, font_size);
}

void nui_unload_font(struct nui_font *font) {
    ctx.backend->unload_font(font);
}

void nui_font(const struct nui_font *font) {
    ctx.current->style.font = font;
}

void nui_image(const struct nui_image *image) {
    ctx.current->image = image;
    if (ctx.current->text) {
        ctx.memory.free(ctx.current->text);
        ctx.current->text = NULL;
    }
    if (ctx.current->wrapped_text) {
        ctx.memory.free(ctx.current->wrapped_text);
        ctx.current->wrapped_text = NULL;
    }
}

void nui_text(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    size_t n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *buffer = ctx.memory.malloc(n + 1);

    va_start(args, fmt);
    vsnprintf(buffer, n + 1, fmt, args);
    va_end(args);

    if (ctx.current->text) {
        ctx.memory.free(ctx.current->text);
    }
    if (ctx.current->wrapped_text) {
        ctx.memory.free(ctx.current->wrapped_text);
        ctx.current->wrapped_text = NULL;
    }

    ctx.current->text = buffer;
    ctx.current->image = NULL;
}
