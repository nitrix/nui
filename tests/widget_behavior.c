#include "testing.h"
#include "nuiw.h"
#include <string.h>

static bool draw_button_frame(bool press, bool release) {
    nui_frame();
    nui_input_mouse_move(8, 8);
    if (press) {
        nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, true);
    }
    if (release) {
        nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, false);
    }
    nuiw_begin_frame();
    bool clicked = nuiw_button("Click");
    nui_update();
    nuiw_end_frame();
    return clicked;
}

static void draw_slider_frame(float *value) {
    nui_frame();
    nuiw_begin_frame();
    nuiw_slider_float("Amount", value, 0.0f, 1.0f);
    nui_update();
    nuiw_end_frame();
}

int widget_behavior(int argc, char **argv) {
    (void)argc;
    (void)argv;

    nui_init(NUI_BACKEND_SW);
    nuiw_init();
    nui_viewport(320, 240);

    const struct nui_element *aligned_child = NULL;
    nui_frame();
    NUI {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_align(NUI_ALIGN_CENTER);
        nui_fixed(100, 50);
        NUI {
            nui_fixed(10, 10);
            aligned_child = nui_current_element();
        }
    }
    nui_update();
    assert(aligned_child->y == 20);

    assert(!draw_button_frame(false, false));
    assert(!draw_button_frame(true, false));
    assert(draw_button_frame(false, true));

    bool checked = false;
    nui_frame();
    nuiw_begin_frame();
    nuiw_checkbox("Enabled", &checked);
    nui_update();
    nuiw_end_frame();

    nui_frame();
    nui_input_mouse_move(8, 8);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, true);
    nuiw_begin_frame();
    nuiw_checkbox("Enabled", &checked);
    nui_update();
    nuiw_end_frame();

    nui_frame();
    nui_input_mouse_move(8, 8);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, false);
    nuiw_begin_frame();
    bool checkbox_clicked = nuiw_checkbox("Enabled", &checked);
    nui_update();
    nuiw_end_frame();
    assert(checkbox_clicked);
    assert(checked);

    char text[32] = "A";
    nui_frame();
    nuiw_begin_frame();
    nuiw_input_text("Name", text, sizeof text);
    nui_update();
    nuiw_end_frame();

    nui_frame();
    nui_input_mouse_move(8, 8);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, true);
    nuiw_begin_frame();
    nuiw_input_text("Name", text, sizeof text);
    nui_update();
    nuiw_end_frame();

    nui_frame();
    nui_input_mouse_move(8, 8);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, false);
    nui_input_text_utf8("B");
    nuiw_begin_frame();
    bool text_changed = nuiw_input_text("Name", text, sizeof text);
    nui_update();
    nuiw_end_frame();
    assert(text_changed);
    assert(strcmp(text, "AB") == 0);

    nui_frame();
    nui_input_key(NUI_KEY_LEFT, true, 0);
    nuiw_begin_frame();
    nuiw_input_text("Name", text, sizeof text);
    nui_update();
    nuiw_end_frame();

    nui_frame();
    nui_input_key(NUI_KEY_LEFT, false, 0);
    nui_input_text_utf8("C");
    nuiw_begin_frame();
    text_changed = nuiw_input_text("Name", text, sizeof text);
    nui_update();
    nuiw_end_frame();
    assert(text_changed);
    assert(strcmp(text, "ACB") == 0);

    nui_frame();
    nui_input_key(NUI_KEY_DELETE, true, 0);
    nuiw_begin_frame();
    text_changed = nuiw_input_text("Name", text, sizeof text);
    nui_update();
    nuiw_end_frame();
    assert(text_changed);
    assert(strcmp(text, "AC") == 0);

    float amount = 0.0f;
    draw_slider_frame(&amount);

    nui_frame();
    nui_input_mouse_move(69, 5);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, true);
    nuiw_begin_frame();
    bool slider_changed = nuiw_slider_float("Amount", &amount, 0.0f, 1.0f);
    nui_update();
    nuiw_end_frame();
    assert(slider_changed);
    assert(amount > 0.49f && amount < 0.51f);

    nui_frame();
    nui_input_mouse_move(-20, 5);
    nuiw_begin_frame();
    nuiw_slider_float("Amount", &amount, 0.0f, 1.0f);
    nui_update();
    nuiw_end_frame();
    assert(amount == 0.0f);

    nui_frame();
    nui_input_mouse_move(200, 5);
    nuiw_begin_frame();
    nuiw_slider_float("Amount", &amount, 0.0f, 1.0f);
    nui_update();
    nuiw_end_frame();
    assert(amount == 1.0f);

    nui_frame();
    nuiw_begin_frame();
    bool radio_clicked = nuiw_radio("Option A", false);
    nui_update();
    nuiw_end_frame();
    assert(!radio_clicked);

    int resize_w = 80;
    int resize_h = 60;
    nui_frame();
    nuiw_begin_frame();
    nuiw_resize_handle("Resize", &resize_w, &resize_h, 40, 40, 160, 140);
    nui_update();
    nuiw_end_frame();

    nui_frame();
    nui_input_mouse_move(8, 8);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, true);
    nuiw_begin_frame();
    nuiw_resize_handle("Resize", &resize_w, &resize_h, 40, 40, 160, 140);
    nui_update();
    nuiw_end_frame();

    nui_frame();
    nui_input_mouse_move(38, 28);
    nuiw_begin_frame();
    bool resized = nuiw_resize_handle("Resize", &resize_w, &resize_h, 40, 40, 160, 140);
    nui_update();
    nuiw_end_frame();
    assert(resized);
    assert(resize_w == 110);
    assert(resize_h == 80);

    nui_frame();
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, false);
    nuiw_begin_frame();
    nuiw_resize_handle("Resize", &resize_w, &resize_h, 40, 40, 160, 140);
    nui_update();
    nuiw_end_frame();

    bool modal_open = true;
    nui_frame();
    nuiw_begin_frame();
    bool modal_visible = nuiw_modal_begin("Confirm", &modal_open);
    assert(modal_visible);
    nuiw_button("Inside");
    nuiw_modal_end();
    nui_update();
    nuiw_end_frame();
    assert(modal_open);

    nui_frame();
    nui_input_key(NUI_KEY_ESCAPE, true, 0);
    nuiw_begin_frame();
    modal_visible = nuiw_modal_begin("Confirm", &modal_open);
    nui_update();
    nuiw_end_frame();
    assert(!modal_visible);
    assert(!modal_open);

    float color[4] = {1.0f, 0.5f, 0.1f, 1.0f};
    nui_frame();
    nuiw_begin_frame();
    nuiw_color_rgba("Tint", color);
    nui_update();
    nuiw_end_frame();

    nuiw_fini();
    nui_fini();
    return EXIT_SUCCESS;
}
