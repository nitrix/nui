#ifndef NUIW_H
#define NUIW_H

#include "nui.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct nuiw_theme {
    uint32_t window;
    uint32_t work;
    uint32_t titlebar;
    uint32_t panel;
    uint32_t panel_strong;
    uint32_t line;
    uint32_t line_soft;
    uint32_t control;
    uint32_t control_hot;
    uint32_t control_low;
    uint32_t control_line;
    uint32_t control_text;
    uint32_t input;
    uint32_t input_line;
    uint32_t selected;
    uint32_t selected_soft;
    uint32_t hover_soft;
    uint32_t focus;
    uint32_t focus_line;
    uint32_t accent;
    uint32_t accent_text;
    uint32_t danger;
    uint32_t danger_text;
    int radius;
    int control_radius;
    int gap;
    int pad;
    int control_height;
};

void nuiw_init(void);
void nuiw_fini(void);
void nuiw_begin_frame(void);
void nuiw_end_frame(void);

void nuiw_set_theme(const struct nuiw_theme *theme);
const struct nuiw_theme *nuiw_theme_high_contrast(void);

void nuiw_push_id_str(const char *id);
void nuiw_push_id_u64(uint64_t id);
void nuiw_pop_id(void);

void nuiw_panel_begin(void);
void nuiw_panel_end(void);
void nuiw_row_begin(void);
void nuiw_row_end(void);
void nuiw_popover_begin(void);
void nuiw_popover_end(void);
void nuiw_dialog_begin(void);
void nuiw_dialog_end(void);
bool nuiw_modal_begin(const char *title, bool *open);
void nuiw_modal_end(void);

bool nuiw_button(const char *label);
bool nuiw_image_button(const char *label, const struct nui_image *image);
bool nuiw_checkbox(const char *label, bool *value);
bool nuiw_radio(const char *label, bool selected);
bool nuiw_slider_float(const char *label, float *value, float min, float max);
bool nuiw_input_text(const char *label, char *buffer, size_t capacity);
bool nuiw_resize_handle(const char *label, int *width, int *height, int min_width, int min_height, int max_width, int max_height);
bool nuiw_combo_begin(const char *label, const char *preview);
bool nuiw_combo_item(const char *label, bool selected);
void nuiw_combo_end(void);
bool nuiw_tab(const char *label, bool selected);
bool nuiw_tree_row(const char *label, bool *open, bool selected);
bool nuiw_color_rgba(const char *label, float rgba[4]);

#endif
