#include "nuiw.h"
#include <stdio.h>
#include <string.h>

#define NUIW_MAX_RECORDS 512
#define NUIW_MAX_ID_STACK 64

struct nuiw_record {
    uint64_t id;
    int x, y, w, h;
    const struct nui_element *element;
    unsigned int touched_frame;
    bool open;
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

bool nuiw_button(const char *label) {
    uint64_t id = _make_id(label);
    struct nuiw_record *record = _record(id);
    bool clicked = _activate(id, record);

    nui_element_begin();
    {
        nui_fixed_height(ctx.theme->control_height);
        nui_padding(5, 10, 5, 10);
        _control_box(id, record, false);
        _remember(record);
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
        nui_fixed_height(ctx.theme->control_height);
        nui_padding(6, 8, 6, 8);
        nui_child_gap(8);
        _box(_hot(record) ? ctx.theme->hover_soft : 0x00000000, ctx.theme->line, ctx.theme->control_radius);
        _remember(record);

        nui_element_begin();
        {
            nui_fixed(14, 14);
            nui_margin(1, 0, 0, 0);
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
        nui_fixed_height(ctx.theme->control_height);
        nui_padding(6, 8, 6, 8);
        nui_child_gap(8);
        _box(_hot(record) ? ctx.theme->hover_soft : 0x00000000, ctx.theme->line, ctx.theme->control_radius);
        _remember(record);

        nui_element_begin();
        {
            nui_fixed(14, 14);
            nui_margin(1, 0, 0, 0);
            _box(selected ? ctx.theme->accent : ctx.theme->input, selected ? ctx.theme->accent : ctx.theme->input_line, 7);
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
    const struct nui_input *input = nui_get_input();
    bool changed = false;

    _activate(id, record);
    if (ctx.active_id == id && value && max > min && record->w > 0) {
        float t = (float)(input->mouse_x - record->x) / (float)record->w;
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
            nui_margin(9, 0, 0, 0);
            nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
            _box(ctx.theme->control_low, ctx.theme->input_line, ctx.theme->control_radius);
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
    bool changed = _activate(id, record);
    const struct nui_input *input = nui_get_input();

    if (ctx.focused_id == id && buffer && capacity > 0) {
        size_t len = strlen(buffer);
        if (input->key_pressed[NUI_KEY_BACKSPACE] && len > 0) {
            buffer[len - 1] = '\0';
            changed = true;
            len--;
        }

        if (input->text_len > 0 && len + input->text_len < capacity) {
            memcpy(buffer + len, input->text, input->text_len + 1);
            changed = true;
        }
    }

    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_child_gap(4);
        _remember(record);
        _text_child(label, ctx.theme->control_text);
        nui_element_begin();
        {
            nui_fixed_height(ctx.theme->control_height);
            nui_padding(5, 8, 5, 8);
            _box(ctx.theme->input, ctx.focused_id == id ? ctx.theme->focus_line : ctx.theme->input_line, ctx.theme->control_radius);
            _text_child(buffer ? buffer : "", ctx.theme->control_text);
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
            nui_fixed_height(ctx.theme->control_height);
            nui_padding(5, 8, 5, 8);
            _control_box(id, record, record->open);
            _text_child(preview, ctx.theme->control_text);
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
        nui_padding(5, 8, 5, 8);
        _control_box(id, record, selected);
        _remember(record);
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
        nui_fixed_height(ctx.theme->control_height + 2);
        nui_padding(5, 10, 5, 10);
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

bool nuiw_color_rgba(const char *label, float rgba[4]) {
    bool changed = false;
    nui_element_begin();
    {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_child_gap(ctx.theme->gap);
        _text_child(label, ctx.theme->control_text);

        nui_element_begin();
        {
            nui_fixed(80, 40);
            uint32_t r = (uint32_t)(rgba[0] * 255.0f) & 0xFF;
            uint32_t g = (uint32_t)(rgba[1] * 255.0f) & 0xFF;
            uint32_t b = (uint32_t)(rgba[2] * 255.0f) & 0xFF;
            uint32_t a = (uint32_t)(rgba[3] * 255.0f) & 0xFF;
            uint32_t color = (r << 24) | (g << 16) | (b << 8) | a;
            _box(color, ctx.theme->line, ctx.theme->control_radius);
        }
        nui_element_end();

        const char *names[] = { "R", "G", "B", "A" };
        for (size_t i = 0; i < 4; i++) {
            nuiw_push_id_u64(i);
            changed |= nuiw_slider_float(names[i], &rgba[i], 0.0f, 1.0f);
            nuiw_pop_id();
        }
    }
    nui_element_end();
    return changed;
}
