#include "testing.h"

int widget_core_input(int argc, char **argv) {
    (void)argc;
    (void)argv;

    nui_init(NUI_BACKEND_SW);

    nui_frame();
    nui_input_mouse_move(10, 20);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, true);
    nui_input_key(NUI_KEY_BACKSPACE, true, NUI_MODIFIER_CTRL);
    nui_input_text_utf8("a");

    const struct nui_input *input = nui_get_input();
    assert(input->mouse_x == 10);
    assert(input->mouse_y == 20);
    assert(input->mouse_delta_x == 10);
    assert(input->mouse_delta_y == 20);
    assert(input->mouse_down[NUI_MOUSE_BUTTON_LEFT]);
    assert(input->mouse_pressed[NUI_MOUSE_BUTTON_LEFT]);
    assert(!input->mouse_released[NUI_MOUSE_BUTTON_LEFT]);
    assert(input->key_down[NUI_KEY_BACKSPACE]);
    assert(input->key_pressed[NUI_KEY_BACKSPACE]);
    assert(input->modifiers == NUI_MODIFIER_CTRL);
    assert(input->text_len == 1);
    assert(input->text[0] == 'a');

    nui_frame();
    input = nui_get_input();
    assert(input->mouse_down[NUI_MOUSE_BUTTON_LEFT]);
    assert(!input->mouse_pressed[NUI_MOUSE_BUTTON_LEFT]);
    assert(!input->mouse_released[NUI_MOUSE_BUTTON_LEFT]);
    assert(input->key_down[NUI_KEY_BACKSPACE]);
    assert(!input->key_pressed[NUI_KEY_BACKSPACE]);
    assert(input->text_len == 0);
    assert(input->text[0] == '\0');

    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, false);
    nui_input_key(NUI_KEY_BACKSPACE, false, 0);
    input = nui_get_input();
    assert(!input->mouse_down[NUI_MOUSE_BUTTON_LEFT]);
    assert(input->mouse_released[NUI_MOUSE_BUTTON_LEFT]);
    assert(!input->key_down[NUI_KEY_BACKSPACE]);
    assert(input->key_released[NUI_KEY_BACKSPACE]);

    nui_fini();
    return EXIT_SUCCESS;
}
