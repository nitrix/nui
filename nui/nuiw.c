#include "nuiw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUIW_MAX_RECORDS 512
#define NUIW_MAX_ID_STACK 64
#define NUIW_MIN(a, b) ((a) < (b) ? (a) : (b))
#define NUIW_MAX(a, b) ((a) > (b) ? (a) : (b))

struct nuiw_record {
    uint64_t id;
    int x, y, w, h;
    const struct nui_element *element;
    unsigned int touched_frame;
    bool open;
    size_t text_cursor;
    size_t text_scroll;
    int drag_start_x;
    int drag_start_y;
    int drag_start_w;
    int drag_start_h;
    struct nui_image *image_a;
    struct nui_image *image_b;
    float color_hue;
    float color_saturation;
    float color_lightness;
    float image_hue;
    bool color_initialized;
};

static const struct nuiw_theme high_contrast = {
    .window = 0x090b0eff,
    .work = 0x06080aff,
    .titlebar = 0x15191fff,
    .panel = 0x15191fff,
    .panel_strong = 0x1e242cff,
    .line = 0x030405ff,
    .line_soft = 0xffffff17,
    .control = 0x343b45ff,
    .control_hot = 0x424b57ff,
    .control_low = 0x0d1014ff,
    .control_line = 0x010203ff,
    .control_text = 0xf2f4f8ff,
    .input = 0x090c10ff,
    .input_line = 0x010203ff,
    .selected = 0xff8a224d,
    .selected_soft = 0xff8a2224,
    .hover_soft = 0xffffff12,
    .focus = 0x5ca4ff40,
    .focus_line = 0x5ca4ffff,
    .accent = 0xff8a22ff,
    .accent_text = 0x090909ff,
    .danger = 0xff6570ff,
    .danger_text = 0xffb3b8ff,
    .radius = 4,
    .control_radius = 3,
    .gap = 9,
    .pad = 11,
    .control_height = 28,
};

static struct {
    const struct nuiw_theme *theme;
    struct nuiw_record records[NUIW_MAX_RECORDS];
    size_t records_count;
    uint64_t id_stack[NUIW_MAX_ID_STACK];
    size_t id_stack_count;
    unsigned int frame;
    uint64_t hot_id;
    uint64_t active_id;
    uint64_t focused_id;
    uint64_t current_combo_id;
} ctx;

static uint64_t _hash_bytes(uint64_t hash, const void *data, size_t len) {
    const unsigned char *bytes = data;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static uint64_t _hash_str(uint64_t hash, const char *text) {
    return _hash_bytes(hash, text, strlen(text));
}

static uint64_t _seed_id(void) {
    return ctx.id_stack_count ? ctx.id_stack[ctx.id_stack_count - 1] : 1469598103934665603ULL;
}

static uint64_t _make_id(const char *label) {
    return _hash_str(_seed_id(), label ? label : "");
}

static struct nuiw_record *_record(uint64_t id) {
    for (size_t i = 0; i < ctx.records_count; i++) {
        if (ctx.records[i].id == id) {
            ctx.records[i].touched_frame = ctx.frame;
            return &ctx.records[i];
        }
    }

    if (ctx.records_count >= NUIW_MAX_RECORDS) {
        return &ctx.records[NUIW_MAX_RECORDS - 1];
    }

    struct nuiw_record *record = &ctx.records[ctx.records_count++];
    *record = (struct nuiw_record) {
        .id = id,
        .touched_frame = ctx.frame,
    };
    return record;
}

static bool _contains(const struct nuiw_record *record, int x, int y) {
    return x >= record->x && x < record->x + record->w && y >= record->y && y < record->y + record->h;
}

static bool _hot(const struct nuiw_record *record) {
    const struct nui_input *input = nui_get_input();
    return _contains(record, input->mouse_x, input->mouse_y);
}

static bool _activate(uint64_t id, const struct nuiw_record *record) {
    const struct nui_input *input = nui_get_input();
    bool hot = _hot(record);
    if (hot) {
        ctx.hot_id = id;
    }

    if (hot && input->mouse_pressed[NUI_MOUSE_BUTTON_LEFT]) {
        ctx.active_id = id;
        ctx.focused_id = id;
    }

    bool clicked = false;
    if (ctx.active_id == id && input->mouse_released[NUI_MOUSE_BUTTON_LEFT]) {
        clicked = hot;
        ctx.active_id = 0;
    }

    if (ctx.active_id == id && !input->mouse_down[NUI_MOUSE_BUTTON_LEFT] && !input->mouse_released[NUI_MOUSE_BUTTON_LEFT]) {
        ctx.active_id = 0;
    }

    return clicked;
}

static void _box(uint32_t fill, uint32_t border, int radius) {
    nui_background_color(fill);
    nui_border(border, 1);
    nui_corner_radius(radius);
}

static void _control_box(uint64_t id, const struct nuiw_record *record, bool selected) {
    const struct nuiw_theme *t = ctx.theme;
    bool hot = ctx.hot_id == id || _hot(record);
    bool active = ctx.active_id == id;
    bool focused = ctx.focused_id == id;

    uint32_t fill = hot ? t->control_hot : t->control;
    uint32_t border = focused ? t->focus_line : t->control_line;
    if (selected) {
        fill = t->selected;
        border = t->accent;
    }
    if (active) {
        fill = t->control_low;
    }

    _box(fill, border, t->control_radius);
}

static void _combo_field_box(uint64_t id, const struct nuiw_record *record) {
    const struct nuiw_theme *t = ctx.theme;
    bool hot = ctx.hot_id == id || _hot(record);
    uint32_t fill = hot ? t->control_low : t->input;
    uint32_t border = (ctx.focused_id == id || record->open) ? t->focus_line : t->input_line;
    _box(fill, border, t->control_radius);
}

static void _combo_item_box(uint64_t id, const struct nuiw_record *record, bool selected) {
    const struct nuiw_theme *t = ctx.theme;
    bool hot = ctx.hot_id == id || _hot(record);
    uint32_t fill = selected ? t->selected_soft : (hot ? t->hover_soft : 0x00000000);
    uint32_t border = selected ? t->accent : 0x00000000;
    _box(fill, border, t->control_radius);
}

static void _remember(struct nuiw_record *record) {
    record->element = nui_current_element();
}

static void _text_child(const char *text, uint32_t color) {
    nui_element_begin();
    {
        nui_font_color(color);
        nui_text("%s", text ? text : "");
    }
    nui_element_end();
}

static void _label_value_row_begin(void) {
    nui_element_begin();
    nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
    nui_align(NUI_ALIGN_CENTER);
    nui_child_gap(ctx.theme->gap);
}

static void _label_value_row_end(void) {
    nui_element_end();
}

void nuiw_init(void) {
    memset(&ctx, 0, sizeof ctx);
    ctx.theme = &high_contrast;
}

void nuiw_fini(void) {
    for (size_t i = 0; i < ctx.records_count; i++) {
        if (ctx.records[i].image_a) {
            nui_unload_image(ctx.records[i].image_a);
        }
        if (ctx.records[i].image_b) {
            nui_unload_image(ctx.records[i].image_b);
        }
    }
    memset(&ctx, 0, sizeof ctx);
}

void nuiw_begin_frame(void) {
    ctx.frame++;
    ctx.hot_id = 0;
    ctx.current_combo_id = 0;
    for (size_t i = 0; i < ctx.records_count; i++) {
        ctx.records[i].element = NULL;
    }
}

void nuiw_end_frame(void) {
    for (size_t i = 0; i < ctx.records_count; i++) {
        struct nuiw_record *record = &ctx.records[i];
        if (record->touched_frame == ctx.frame && record->element) {
            record->x = record->element->x;
            record->y = record->element->y;
            record->w = record->element->w;
            record->h = record->element->h;
        }
    }
}

void nuiw_set_theme(const struct nuiw_theme *theme) {
    ctx.theme = theme ? theme : &high_contrast;
}

const struct nuiw_theme *nuiw_theme_high_contrast(void) {
    return &high_contrast;
}

void nuiw_push_id_str(const char *id) {
    if (ctx.id_stack_count >= NUIW_MAX_ID_STACK) {
        return;
    }
    ctx.id_stack[ctx.id_stack_count++] = _hash_str(_seed_id(), id ? id : "");
}

void nuiw_push_id_u64(uint64_t id) {
    if (ctx.id_stack_count >= NUIW_MAX_ID_STACK) {
        return;
    }
    ctx.id_stack[ctx.id_stack_count++] = _hash_bytes(_seed_id(), &id, sizeof id);
}

void nuiw_pop_id(void) {
    if (ctx.id_stack_count > 0) {
        ctx.id_stack_count--;
    }
}

void nuiw_panel_begin(void) {
    nui_element_begin();
    nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
    nui_padding(ctx.theme->pad, ctx.theme->pad, ctx.theme->pad, ctx.theme->pad);
    nui_child_gap(ctx.theme->gap);
    _box(ctx.theme->panel, ctx.theme->line, ctx.theme->radius);
}

void nuiw_panel_end(void) {
    nui_element_end();
}

void nuiw_row_begin(void) {
    nui_element_begin();
    nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
    nui_align(NUI_ALIGN_CENTER);
    nui_child_gap(ctx.theme->gap);
}

void nuiw_row_end(void) {
    nui_element_end();
}

void nuiw_popover_begin(void) {
    nui_element_begin();
    nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
    nui_padding(ctx.theme->pad, ctx.theme->pad, ctx.theme->pad, ctx.theme->pad);
    nui_child_gap(ctx.theme->gap / 2);
    _box(ctx.theme->panel_strong, ctx.theme->line, ctx.theme->radius);
}

void nuiw_popover_end(void) {
    nui_element_end();
}

void nuiw_dialog_begin(void) {
    nui_element_begin();
    nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
    nui_padding(ctx.theme->pad + 3, ctx.theme->pad + 3, ctx.theme->pad + 3, ctx.theme->pad + 3);
    nui_child_gap(ctx.theme->gap);
    _box(ctx.theme->panel_strong, ctx.theme->focus_line, ctx.theme->radius + 2);
}

void nuiw_dialog_end(void) {
    nui_element_end();
}

bool nuiw_modal_begin(const char *title, bool *open) {
    if (!open || !*open) {
        return false;
    }

    if (nui_get_input()->key_pressed[NUI_KEY_ESCAPE]) {
        *open = false;
        return false;
    }

    nui_element_begin();
    {
        nui_overlay();
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_align(NUI_ALIGN_CENTER);
        nui_padding(96, 96, 96, 96);
        nui_background_color(0x00000099);

        nuiw_dialog_begin();
            NUI {
                nui_font_color(ctx.theme->accent);
                nui_text("%s", title ? title : "Modal");
            }
    }

    return true;
}

void nuiw_modal_end(void) {
        nuiw_dialog_end();
    nui_element_end();
}

bool nuiw_button(const char *label) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);

    nui_element_begin();
    {
        nui_padding(7, 7, 7, 7);
        _control_box(id, record, false);
        _remember(record);
        _text_child(label, ctx.active_id == id ? ctx.theme->accent_text : ctx.theme->control_text);
    }
    nui_element_end();

    return clicked;
}

bool nuiw_image_button(const char *label, const struct nui_image *image) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);

    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_align(NUI_ALIGN_CENTER);
        nui_padding(6, 8, 6, 8);
        nui_child_gap(7);
        _control_box(id, record, false);
        _remember(record);
        if (image) {
            nui_element_begin();
            {
                nui_fixed(18, 18);
                nui_image(image);
                nui_image_mode(NUI_IMAGE_MODE_STRETCH);
            }
            nui_element_end();
        }
        _text_child(label, ctx.active_id == id ? ctx.theme->accent_text : ctx.theme->control_text);
    }
    nui_element_end();

    return clicked;
}

bool nuiw_checkbox(const char *label, bool *value) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);
    if (clicked && value) {
        *value = !*value;
    }

    bool checked = value && *value;
    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_align(NUI_ALIGN_CENTER);
        nui_fixed_height(ctx.theme->control_height);
        nui_padding(6, 8, 6, 8);
        nui_child_gap(8);
        _box(_hot(record) ? ctx.theme->hover_soft : 0x00000000, ctx.theme->line, ctx.theme->control_radius);
        _remember(record);

        nui_element_begin();
        {
            nui_fixed(14, 14);
            _box(checked ? ctx.theme->accent : ctx.theme->input, checked ? ctx.theme->accent : ctx.theme->input_line, ctx.theme->control_radius);
        }
        nui_element_end();
        _text_child(label, ctx.theme->control_text);
    }
    nui_element_end();

    return clicked;
}

bool nuiw_radio(const char *label, bool selected) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);

    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_align(NUI_ALIGN_CENTER);
        nui_fixed_height(ctx.theme->control_height);
        nui_padding(6, 8, 6, 8);
        nui_child_gap(8);
        _box(_hot(record) ? ctx.theme->hover_soft : 0x00000000, ctx.theme->line, ctx.theme->control_radius);
        _remember(record);

        nui_element_begin();
        {
            nui_fixed(16, 16);
            nui_padding(4, 4, 4, 4);
            _box(ctx.theme->input, selected ? ctx.theme->accent : ctx.theme->input_line, 8);
            if (selected) {
                nui_element_begin();
                {
                    nui_fixed(8, 8);
                    _box(ctx.theme->accent, ctx.theme->accent, 4);
                }
                nui_element_end();
            }
        }
        nui_element_end();
        _text_child(label, ctx.theme->control_text);
    }
    nui_element_end();

    return clicked;
}

bool nuiw_slider_float(const char *label, float *value, float min, float max) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    uint64_t track_id = _hash_str(id, ":track");
    struct nuiw_record *track_record = _record(track_id);
    const struct nui_input *input = nui_get_input();
    bool changed = false;

    _activate(id, record);
    if (ctx.active_id == id && value && max > min && track_record->w > 0) {
        float t = (float)(input->mouse_x - track_record->x) / (float)track_record->w;
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        float next = min + (max - min) * t;
        changed = next != *value;
        *value = next;
    }

    float normalized = 0;
    if (value && max > min) {
        normalized = (*value - min) / (max - min);
        if (normalized < 0) normalized = 0;
        if (normalized > 1) normalized = 1;
    }
    int fill_width = 120;
    int filled = (int)(fill_width * normalized);

    _label_value_row_begin();
        _remember(record);
        _text_child(label, ctx.theme->control_text);
        nui_element_begin();
        {
            nui_fixed(fill_width, 10);
            nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
            _box(ctx.theme->control_low, ctx.theme->input_line, ctx.theme->control_radius);
            _remember(track_record);
            nui_element_begin();
            {
                nui_fixed(filled, 10);
                _box(ctx.theme->accent, ctx.theme->accent, ctx.theme->control_radius);
            }
            nui_element_end();
        }
        nui_element_end();
        char text[32];
        snprintf(text, sizeof text, "%.2f", value ? *value : 0.0f);
        _text_child(text, ctx.theme->control_text);
    _label_value_row_end();

    return changed;
}

bool nuiw_input_text(const char *label, char *buffer, size_t capacity) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    _activate(id, record);
    const struct nui_input *input = nui_get_input();
    bool changed = false;

    if (buffer && capacity > 0) {
        size_t len = strlen(buffer);
        if (record->text_cursor > len) {
            record->text_cursor = len;
        }
        if (_hot(record) && input->mouse_pressed[NUI_MOUSE_BUTTON_LEFT]) {
            record->text_cursor = len;
        }

        if (ctx.focused_id == id) {
            if (input->key_pressed[NUI_KEY_HOME]) {
                record->text_cursor = 0;
            }
            if (input->key_pressed[NUI_KEY_END]) {
                record->text_cursor = len;
            }
            if (input->key_pressed[NUI_KEY_LEFT] && record->text_cursor > 0) {
                record->text_cursor--;
            }
            if (input->key_pressed[NUI_KEY_RIGHT] && record->text_cursor < len) {
                record->text_cursor++;
            }

            if (input->key_pressed[NUI_KEY_BACKSPACE] && record->text_cursor > 0) {
                memmove(buffer + record->text_cursor - 1, buffer + record->text_cursor, len - record->text_cursor + 1);
                record->text_cursor--;
                len--;
                changed = true;
            }
            if (input->key_pressed[NUI_KEY_DELETE] && record->text_cursor < len) {
                memmove(buffer + record->text_cursor, buffer + record->text_cursor + 1, len - record->text_cursor);
                len--;
                changed = true;
            }

            if (input->text_len > 0 && len + 1 < capacity) {
                size_t available = capacity - len - 1;
                size_t insert_len = NUIW_MIN(input->text_len, available);
                memmove(buffer + record->text_cursor + insert_len, buffer + record->text_cursor, len - record->text_cursor + 1);
                memcpy(buffer + record->text_cursor, input->text, insert_len);
                record->text_cursor += insert_len;
                len += insert_len;
                changed = insert_len > 0;
            }
        }

        size_t visible_chars = 28;
        if (record->text_cursor < record->text_scroll) {
            record->text_scroll = record->text_cursor;
        }
        if (record->text_cursor > record->text_scroll + visible_chars) {
            record->text_scroll = record->text_cursor - visible_chars;
        }
        if (record->text_scroll > len) {
            record->text_scroll = len;
        }
    }

    const char *source = buffer ? buffer : "";
    size_t len = strlen(source);
    size_t visible_chars = 28;
    size_t scroll = NUIW_MIN(record->text_scroll, len);
    size_t visible_len = NUIW_MIN(visible_chars, len - scroll);
    char visible[64];
    memcpy(visible, source + scroll, visible_len);
    visible[visible_len] = '\0';

    size_t cursor_in_view = 0;
    if (record->text_cursor > scroll) {
        cursor_in_view = NUIW_MIN(record->text_cursor - scroll, visible_len);
    }

    char before[64];
    char after[64];
    memcpy(before, visible, cursor_in_view);
    before[cursor_in_view] = '\0';
    memcpy(after, visible + cursor_in_view, visible_len - cursor_in_view + 1);

    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_child_gap(4);
        _remember(record);
        _text_child(label, ctx.theme->control_text);
        nui_element_begin();
        {
            nui_fixed(260, ctx.theme->control_height);
            nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
            nui_align(NUI_ALIGN_CENTER);
            nui_padding(5, 8, 5, 8);
            nui_child_gap(0);
            _box(ctx.theme->input, ctx.focused_id == id ? ctx.theme->focus_line : ctx.theme->input_line, ctx.theme->control_radius);
            _text_child(before, ctx.theme->control_text);
            if (ctx.focused_id == id) {
                nui_element_begin();
                {
                    nui_fixed(1, 16);
                    if ((ctx.frame / 30) % 2 == 0) {
                        nui_background_color(ctx.theme->focus_line);
                    }
                }
                nui_element_end();
            }
            _text_child(after, ctx.theme->control_text);
        }
        nui_element_end();
    }
    nui_element_end();

    return changed;
}

bool nuiw_resize_handle(const char *label, int *width, int *height, int min_width, int min_height, int max_width, int max_height) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    const struct nui_input *input = nui_get_input();
    bool hot = _hot(record);

    if (hot && input->mouse_pressed[NUI_MOUSE_BUTTON_LEFT]) {
        ctx.active_id = id;
        record->drag_start_x = input->mouse_x;
        record->drag_start_y = input->mouse_y;
        record->drag_start_w = width ? *width : 0;
        record->drag_start_h = height ? *height : 0;
    }

    bool changed = false;
    if (ctx.active_id == id && input->mouse_down[NUI_MOUSE_BUTTON_LEFT]) {
        int next_w = record->drag_start_w + input->mouse_x - record->drag_start_x;
        int next_h = record->drag_start_h + input->mouse_y - record->drag_start_y;
        if (next_w < min_width) next_w = min_width;
        if (next_h < min_height) next_h = min_height;
        if (max_width > min_width && next_w > max_width) next_w = max_width;
        if (max_height > min_height && next_h > max_height) next_h = max_height;
        if (width && *width != next_w) {
            *width = next_w;
            changed = true;
        }
        if (height && *height != next_h) {
            *height = next_h;
            changed = true;
        }
    }
    if (ctx.active_id == id && input->mouse_released[NUI_MOUSE_BUTTON_LEFT]) {
        ctx.active_id = 0;
    }

    nui_element_begin();
    {
        nui_fixed(18, 18);
        nui_padding(3, 3, 3, 3);
        _box(ctx.active_id == id ? ctx.theme->accent : (hot ? ctx.theme->control_hot : ctx.theme->control), ctx.theme->control_line, ctx.theme->control_radius);
        _remember(record);
        nui_element_begin();
        {
            nui_fixed(10, 10);
            nui_background_color(ctx.theme->line_soft);
        }
        nui_element_end();
    }
    nui_element_end();

    return changed;
}

bool nuiw_combo_begin(const char *label, const char *preview) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    if (_activate(id, record)) {
        record->open = !record->open;
    }
    ctx.current_combo_id = id;

    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_child_gap(4);
        _remember(record);
        _text_child(label, ctx.theme->control_text);
        nui_element_begin();
        {
            nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
            nui_align(NUI_ALIGN_CENTER);
            nui_fixed_height(ctx.theme->control_height);
            nui_padding(5, 8, 5, 8);
            nui_child_gap(8);
            _combo_field_box(id, record);
            _text_child(preview, ctx.theme->control_text);
            _text_child(record->open ? "^" : "v", ctx.theme->control_text);
        }
        nui_element_end();
    }
    nui_element_end();

    if (record->open) {
        nuiw_popover_begin();
    }

    return record->open;
}

bool nuiw_combo_item(const char *label, bool selected) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);
    if (clicked) {
        struct nuiw_record *combo = _record(ctx.current_combo_id);
        combo->open = false;
    }

    nui_element_begin();
    {
        nui_fixed_height(ctx.theme->control_height);
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_align(NUI_ALIGN_CENTER);
        nui_padding(5, 8, 5, 8);
        nui_child_gap(6);
        _combo_item_box(id, record, selected);
        _remember(record);
        if (selected) {
            _text_child("*", ctx.theme->accent);
        }
        _text_child(label, ctx.theme->control_text);
    }
    nui_element_end();

    return clicked;
}

void nuiw_combo_end(void) {
    if (ctx.current_combo_id) {
        struct nuiw_record *combo = _record(ctx.current_combo_id);
        if (combo->open) {
            nuiw_popover_end();
        }
    }
    ctx.current_combo_id = 0;
}

bool nuiw_tab(const char *label, bool selected) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);

    nui_element_begin();
    {
        nui_padding(7, 7, 7, 7);
        _control_box(id, record, selected);
        _remember(record);
        _text_child(label, ctx.theme->control_text);
    }
    nui_element_end();

    return clicked;
}

bool nuiw_tree_row(const char *label, bool *open, bool selected) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);
    if (clicked && open) {
        *open = !*open;
    }

    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_align(NUI_ALIGN_CENTER);
        nui_fixed_height(ctx.theme->control_height);
        nui_padding(5, 8, 5, 8);
        nui_child_gap(6);
        _control_box(id, record, selected);
        _remember(record);
        _text_child(open && *open ? "v" : ">", ctx.theme->control_text);
        _text_child(label, ctx.theme->control_text);
    }
    nui_element_end();

    return clicked;
}

static float _clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static unsigned char _float_to_byte(float value) {
    value = _clamp01(value);
    return (unsigned char)(value * 255.0f + 0.5f);
}

static void _hue_to_rgb(float hue, float *r, float *g, float *b) {
    while (hue < 0.0f) hue += 1.0f;
    while (hue > 1.0f) hue -= 1.0f;
    float scaled = hue * 6.0f;
    int sector = (int)scaled;
    float f = scaled - (float)sector;
    if (sector >= 6) {
        sector = 0;
        f = 0.0f;
    }

    switch (sector) {
        case 0: *r = 1.0f; *g = f; *b = 0.0f; break;
        case 1: *r = 1.0f - f; *g = 1.0f; *b = 0.0f; break;
        case 2: *r = 0.0f; *g = 1.0f; *b = f; break;
        case 3: *r = 0.0f; *g = 1.0f - f; *b = 1.0f; break;
        case 4: *r = f; *g = 0.0f; *b = 1.0f; break;
        default: *r = 1.0f; *g = 0.0f; *b = 1.0f - f; break;
    }
}

static void _rgb_to_hsl(float r, float g, float b, float *h, float *s, float *l) {
    float max = NUIW_MAX(r, NUIW_MAX(g, b));
    float min = NUIW_MIN(r, NUIW_MIN(g, b));
    *l = (max + min) * 0.5f;

    if (max == min) {
        *h = 0.0f;
        *s = 0.0f;
        return;
    }

    float d = max - min;
    *s = *l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
    if (max == r) {
        *h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    } else if (max == g) {
        *h = (b - r) / d + 2.0f;
    } else {
        *h = (r - g) / d + 4.0f;
    }
    *h /= 6.0f;
}

static bool _triangle_color(float hue, float nx, float ny, float *r, float *g, float *b) {
    float center = 0.5f;
    ny = _clamp01(ny);
    float half_width = ny * 0.5f;
    float min_x = center - half_width;
    float max_x = center + half_width;
    if (max_x <= min_x) {
        nx = center;
    } else {
        if (nx < min_x) nx = min_x;
        if (nx > max_x) nx = max_x;
    }

    float hue_r, hue_g, hue_b;
    _hue_to_rgb(hue, &hue_r, &hue_g, &hue_b);

    float black = ny <= 0.0f ? 0.0f : (nx - min_x) / (max_x - min_x);
    float white = ny <= 0.0f ? 0.0f : (max_x - nx) / (max_x - min_x);
    float pure = 1.0f - ny;
    if (black < 0.0f) black = 0.0f;
    if (white < 0.0f) white = 0.0f;
    if (pure < 0.0f) pure = 0.0f;

    *r = pure * hue_r + white;
    *g = pure * hue_g + white;
    *b = pure * hue_b + white;
    return nx >= min_x && nx <= max_x;
}

static void _write_pixel(unsigned char *pixels, int width, int x, int y, float r, float g, float b, float a) {
    unsigned char *at = pixels + ((size_t)y * (size_t)width + (size_t)x) * 4;
    at[0] = _float_to_byte(r);
    at[1] = _float_to_byte(g);
    at[2] = _float_to_byte(b);
    at[3] = _float_to_byte(a);
}

static void _build_hue_strip(unsigned char *pixels, int width, int height) {
    for (int y = 0; y < height; y++) {
        float hue = (float)y / (float)NUIW_MAX(1, height - 1);
        float r, g, b;
        _hue_to_rgb(hue, &r, &g, &b);
        for (int x = 0; x < width; x++) {
            _write_pixel(pixels, width, x, y, r, g, b, 1.0f);
        }
    }
}

static void _build_color_triangle(unsigned char *pixels, int width, int height, float hue) {
    for (int y = 0; y < height; y++) {
        float ny = (float)y / (float)NUIW_MAX(1, height - 1);
        float half_width = ny * (float)(width - 1) * 0.5f;
        float center = (float)(width - 1) * 0.5f;
        float min_x = center - half_width;
        float max_x = center + half_width;
        for (int x = 0; x < width; x++) {
            if ((float)x < min_x || (float)x > max_x) {
                _write_pixel(pixels, width, x, y, 0.0f, 0.0f, 0.0f, 0.0f);
                continue;
            }
            float r, g, b;
            _triangle_color(hue, (float)x / (float)NUIW_MAX(1, width - 1), ny, &r, &g, &b);
            _write_pixel(pixels, width, x, y, r, g, b, 1.0f);
        }
    }
}

static void _ensure_color_images(struct nuiw_record *record) {
    enum { TRIANGLE_W = 180, TRIANGLE_H = 140, HUE_W = 22, HUE_H = 140 };
    float hue_delta = record->image_hue - record->color_hue;
    if (hue_delta < 0.0f) hue_delta = -hue_delta;

    if (!record->image_b) {
        unsigned char *pixels = malloc((size_t)HUE_W * HUE_H * 4);
        if (pixels) {
            _build_hue_strip(pixels, HUE_W, HUE_H);
            record->image_b = nui_create_image_rgba(HUE_W, HUE_H, pixels);
            free(pixels);
        }
    }

    if (!record->image_a || hue_delta > 0.001f) {
        unsigned char *pixels = malloc((size_t)TRIANGLE_W * TRIANGLE_H * 4);
        if (pixels) {
            _build_color_triangle(pixels, TRIANGLE_W, TRIANGLE_H, record->color_hue);
            if (record->image_a) {
                nui_update_image_rgba(record->image_a, pixels);
            } else {
                record->image_a = nui_create_image_rgba(TRIANGLE_W, TRIANGLE_H, pixels);
            }
            record->image_hue = record->color_hue;
            free(pixels);
        }
    }
}

bool nuiw_color_rgba(const char *label, float rgba[4]) {
    enum { TRIANGLE_W = 180, TRIANGLE_H = 140, HUE_W = 22, HUE_H = 140 };
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    uint64_t triangle_id = _hash_str(id, ":triangle");
    uint64_t hue_id = _hash_str(id, ":hue");
    struct nuiw_record *triangle_record = _record(triangle_id);
    struct nuiw_record *hue_record = _record(hue_id);
    const struct nui_input *input = nui_get_input();
    bool changed = false;
    bool hue_changed = false;

    if (!record->color_initialized && rgba) {
        _rgb_to_hsl(rgba[0], rgba[1], rgba[2], &record->color_hue, &record->color_saturation, &record->color_lightness);
        record->color_lightness = 1.0f - record->color_lightness;
        record->color_initialized = true;
    }
    _ensure_color_images(record);

    if (_hot(triangle_record) && input->mouse_pressed[NUI_MOUSE_BUTTON_LEFT]) {
        ctx.active_id = triangle_id;
    }
    if (_hot(hue_record) && input->mouse_pressed[NUI_MOUSE_BUTTON_LEFT]) {
        ctx.active_id = hue_id;
    }
    if (input->mouse_released[NUI_MOUSE_BUTTON_LEFT] && (ctx.active_id == triangle_id || ctx.active_id == hue_id)) {
        ctx.active_id = 0;
    }

    if (ctx.active_id == hue_id && hue_record->h > 0) {
        record->color_hue = _clamp01((float)(input->mouse_y - hue_record->y) / (float)hue_record->h);
        changed = true;
        hue_changed = true;
    }
    if (ctx.active_id == triangle_id && triangle_record->w > 0 && triangle_record->h > 0) {
        float nx = _clamp01((float)(input->mouse_x - triangle_record->x) / (float)triangle_record->w);
        float ny = _clamp01((float)(input->mouse_y - triangle_record->y) / (float)triangle_record->h);
        float half_width = ny * 0.5f;
        float min_x = 0.5f - half_width;
        float max_x = 0.5f + half_width;
        if (max_x <= min_x) {
            nx = 0.5f;
        } else {
            if (nx < min_x) nx = min_x;
            if (nx > max_x) nx = max_x;
        }
        record->color_saturation = nx;
        record->color_lightness = ny;
        changed = true;
    }

    if (rgba && changed) {
        _triangle_color(record->color_hue, record->color_saturation, record->color_lightness, &rgba[0], &rgba[1], &rgba[2]);
    }
    if (hue_changed) {
        _ensure_color_images(record);
    }

    uint32_t preview = 0x000000ff;
    if (rgba) {
        preview = ((uint32_t)_float_to_byte(rgba[0]) << 24) | ((uint32_t)_float_to_byte(rgba[1]) << 16) | ((uint32_t)_float_to_byte(rgba[2]) << 8) | _float_to_byte(rgba[3]);
    }

    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_child_gap(ctx.theme->gap);
        _text_child(label, ctx.theme->control_text);

        nuiw_row_begin();
            nui_element_begin();
            {
                nui_fixed(68, 44);
                _box(preview, ctx.theme->line, ctx.theme->control_radius);
            }
            nui_element_end();

            nui_element_begin();
            {
                nui_fixed(TRIANGLE_W, TRIANGLE_H);
                _box(ctx.theme->control_low, ctx.theme->line, ctx.theme->control_radius);
                if (record->image_a) {
                    nui_image(record->image_a);
                    nui_image_mode(NUI_IMAGE_MODE_STRETCH);
                }
                _remember(triangle_record);
            }
            nui_element_end();

            nui_element_begin();
            {
                nui_fixed(HUE_W, HUE_H);
                if (record->image_b) {
                    nui_image(record->image_b);
                    nui_image_mode(NUI_IMAGE_MODE_STRETCH);
                }
                nui_border(ctx.theme->line, 1);
                _remember(hue_record);
            }
            nui_element_end();
        nuiw_row_end();

        if (rgba) {
            nuiw_push_id_str("alpha");
            changed |= nuiw_slider_float("Alpha", &rgba[3], 0.0f, 1.0f);
            nuiw_pop_id();
        }

        char text[96];
        snprintf(text, sizeof text, "H %.0f  triangle %.2f %.2f", record->color_hue * 360.0f, record->color_saturation, record->color_lightness);
        _text_child(text, ctx.theme->control_text);
    }
    nui_element_end();

    return changed;
}
