#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "ngl.h"
#include "nui.h"
#include "nuiw.h"
#include <stdio.h>
#include <stdlib.h>

static enum nui_modifiers _mods(void) {
    enum nui_modifiers mods = 0;
    if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        mods |= NUI_MODIFIER_SHIFT;
    }
    if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        mods |= NUI_MODIFIER_CTRL;
    }
    if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
        mods |= NUI_MODIFIER_ALT;
    }
    if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS) {
        mods |= NUI_MODIFIER_SUPER;
    }
    return mods;
}

static void _on_framebuffer_size(GLFWwindow *window, int fb_width, int fb_height) {
    (void) window;
    nui_viewport(fb_width, fb_height);
}

static void _on_char(GLFWwindow *window, unsigned int codepoint) {
    (void) window;
    char utf8[5] = {0};
    if (codepoint <= 0x7F) {
        utf8[0] = (char)codepoint;
    } else if (codepoint <= 0x7FF) {
        utf8[0] = (char)(0xC0 | (codepoint >> 6));
        utf8[1] = (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        utf8[0] = (char)(0xE0 | (codepoint >> 12));
        utf8[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8[2] = (char)(0x80 | (codepoint & 0x3F));
    } else {
        utf8[0] = (char)(0xF0 | (codepoint >> 18));
        utf8[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        utf8[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8[3] = (char)(0x80 | (codepoint & 0x3F));
    }
    nui_input_text_utf8(utf8);
}

static void _feed_key(GLFWwindow *window, int glfw_key, enum nui_key nui_key) {
    nui_input_key(nui_key, glfwGetKey(window, glfw_key) == GLFW_PRESS, _mods());
}

static void _feed_input(GLFWwindow *window) {
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    nui_input_mouse_move((int)mx, (int)my);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_LEFT, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_RIGHT, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    nui_input_mouse_button(NUI_MOUSE_BUTTON_MIDDLE, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    _feed_key(window, GLFW_KEY_BACKSPACE, NUI_KEY_BACKSPACE);
    _feed_key(window, GLFW_KEY_DELETE, NUI_KEY_DELETE);
    _feed_key(window, GLFW_KEY_LEFT, NUI_KEY_LEFT);
    _feed_key(window, GLFW_KEY_RIGHT, NUI_KEY_RIGHT);
    _feed_key(window, GLFW_KEY_HOME, NUI_KEY_HOME);
    _feed_key(window, GLFW_KEY_END, NUI_KEY_END);
    _feed_key(window, GLFW_KEY_ENTER, NUI_KEY_ENTER);
    _feed_key(window, GLFW_KEY_ESCAPE, NUI_KEY_ESCAPE);
    _feed_key(window, GLFW_KEY_TAB, NUI_KEY_TAB);
}

static void _demo_heading(struct nui_font *font, const char *text) {
    NUI {
        nui_font(font);
        nui_font_color(nuiw_theme_high_contrast()->accent);
        nui_text("%s", text);
    }
}

static void _demo_note(struct nui_font *font, const char *text) {
    NUI {
        nui_font(font);
        nui_font_color(0xb8c0ccff);
        nui_text("%s", text);
    }
}

static void _demo_code(struct nui_font *font, const char *text) {
    NUI {
        nui_grow_width();
        nui_padding(8, 10, 8, 10);
        nui_background_color(0x090c10ff);
        nui_border(0xffffff17, 1);
        nui_corner_radius(4);
        nui_font(font);
        nui_font_color(0xd7dde8ff);
        nui_text("%s", text);
    }
}

static void _demo_alignment_box(struct nui_font *font, const char *label, enum nui_layout layout, enum nui_align align) {
    NUI {
        nui_fixed(180, 82);
        nui_layout(layout);
        nui_align(align);
        nui_padding(8, 8, 8, 8);
        nui_background_color(0x0d1014ff);
        nui_border(0x5ca4ff66, 1);
        nui_corner_radius(4);
        NUI {
            nui_fixed(72, 24);
            nui_background_color(0xff8a224d);
            nui_border(0xff8a22ff, 1);
            nui_corner_radius(4);
            nui_font(font);
            nui_font_color(0xf2f4f8ff);
            nui_text("%s", label);
        }
    }
}

int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "nui High Contrast Widgets", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetFramebufferSizeCallback(window, _on_framebuffer_size);
    glfwSetCharCallback(window, _on_char);

    nui_init(NUI_BACKEND_NGL);
    nuiw_init();

    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    nui_viewport(fb_width, fb_height);

    struct nui_font *font = nui_load_font_from_file("assets/DejaVuSans.ttf", 14.0f);
    struct nui_font *font_small = nui_load_font_from_file("assets/DejaVuSans.ttf", 11.0f);
    struct nui_font *font_large = nui_load_font_from_file("assets/DejaVuSans.ttf", 22.0f);
    struct nui_font *font_title = nui_load_font_from_file("assets/DejaVuSans.ttf", 30.0f);
    struct nui_image *logo = nui_load_image_from_file("assets/nui.png");
    char name[128] = "Editable text keeps this field stable";
    bool visible = true;
    bool selectable = true;
    bool tree_open = true;
    bool button_toggle = false;
    bool popover_open = false;
    bool modal_open = false;
    int demo_page = 0;
    int radio_mode = 0;
    int click_count = 0;
    int image_width = 280;
    int image_height = 160;
    float alpha = 0.85f;
    float rgba[4] = {1.0f, 0.54f, 0.13f, 1.0f};
    const char *pivot = "Stretch";

    while (!glfwWindowShouldClose(window)) {
        nui_frame();
        glfwPollEvents();
        _feed_input(window);
        nuiw_begin_frame();

        glClearColor(0.02f, 0.03f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        NUI {
            nui_grow();
            nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
            nui_background_color(nuiw_theme_high_contrast()->work);
            nui_padding(12, 12, 12, 12);
            nui_child_gap(10);
            nui_font(font);
            nui_font_color(nuiw_theme_high_contrast()->control_text);

            nuiw_panel_begin();
                nui_fixed_width(230);
                if (nuiw_tab("Buttons", demo_page == 0)) demo_page = 0;
                if (nuiw_tab("Text", demo_page == 1)) demo_page = 1;
                if (nuiw_tab("Inputs", demo_page == 2)) demo_page = 2;
                if (nuiw_tab("Images", demo_page == 3)) demo_page = 3;
                if (nuiw_tab("Selection", demo_page == 4)) demo_page = 4;
                if (nuiw_tab("Popups", demo_page == 5)) demo_page = 5;
                if (nuiw_tab("Color", demo_page == 6)) demo_page = 6;
            nuiw_panel_end();

            nuiw_panel_begin();
                nui_grow();
                if (demo_page == 0) {
                    _demo_heading(font_title, "Buttons");
                    _demo_note(font_small, "Start with plain click actions, then compose buttons with images and state.");
                    _demo_code(font_small, "if (nuiw_button(\"Increment\")) clicks++;\nif (nuiw_image_button(\"Icon button\", logo)) { ... }");
                    nuiw_row_begin();
                        if (nuiw_button("Primary")) alpha = 1.0f;
                        if (nuiw_button("Quiet")) alpha = 0.65f;
                        if (nuiw_button("Increment")) click_count++;
                    nuiw_row_end();
                    nuiw_row_begin();
                        if (nuiw_image_button("Image button", logo)) click_count++;
                        if (nuiw_button(button_toggle ? "Toggle on" : "Toggle off")) button_toggle = !button_toggle;
                    nuiw_row_end();
                    _demo_note(font, "Clicks: use the return value in the same frame. Current count: ");
                    NUI {
                        nui_font(font_large);
                        nui_font_color(0x5ca4ffff);
                        nui_text("%d", click_count);
                    }
                } else if (demo_page == 1) {
                    _demo_heading(font_title, "Text and alignment");
                    _demo_note(font_small, "Text is just element content. Container layout controls where child elements land.");
                    _demo_code(font_small, "nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);\nnui_align(NUI_ALIGN_CENTER); // cross-axis alignment");
                    NUI {
                        nui_font(font_large);
                        nui_font_color(0xf2f4f8ff);
                        nui_text("Font size, color, wrapping, and alignment are all regular element properties.");
                    }
                    nuiw_row_begin();
                        _demo_alignment_box(font_small, "start", NUI_LAYOUT_LEFT_TO_RIGHT, NUI_ALIGN_START);
                        _demo_alignment_box(font_small, "center", NUI_LAYOUT_LEFT_TO_RIGHT, NUI_ALIGN_CENTER);
                        _demo_alignment_box(font_small, "end", NUI_LAYOUT_LEFT_TO_RIGHT, NUI_ALIGN_END);
                    nuiw_row_end();
                    _demo_note(font_small, "With left-to-right layout, align moves children vertically.");
                    nuiw_row_begin();
                        _demo_alignment_box(font_small, "start", NUI_LAYOUT_TOP_TO_BOTTOM, NUI_ALIGN_START);
                        _demo_alignment_box(font_small, "center", NUI_LAYOUT_TOP_TO_BOTTOM, NUI_ALIGN_CENTER);
                        _demo_alignment_box(font_small, "end", NUI_LAYOUT_TOP_TO_BOTTOM, NUI_ALIGN_END);
                    nuiw_row_end();
                    _demo_note(font_small, "With top-to-bottom layout, align moves children horizontally.");
                } else if (demo_page == 2) {
                    _demo_heading(font_title, "Inputs");
                    _demo_note(font_small, "Text fields keep a fixed visual size, show focus with a caret, and scroll horizontally when content overflows.");
                    _demo_code(font_small, "char name[128] = \"Editable\";\nnuiw_input_text(\"Name\", name, sizeof name);");
                    nuiw_input_text("Name", name, sizeof name);
                    bool combo_open = nuiw_combo_begin("Image mode", pivot);
                    if (combo_open) {
                        if (nuiw_combo_item("Stretch", pivot[0] == 'S')) pivot = "Stretch";
                        if (nuiw_combo_item("Repeat", pivot[0] == 'R')) pivot = "Repeat";
                    }
                    nuiw_combo_end();
                    nuiw_checkbox("Visible", &visible);
                    nuiw_checkbox("Selectable", &selectable);
                    nuiw_slider_float("Alpha", &alpha, 0.0f, 1.0f);
                } else if (demo_page == 3) {
                    _demo_heading(font_title, "Images");
                    _demo_note(font_small, "Images can render at native size, repeat as a texture, or stretch to an element box.");
                    _demo_code(font_small, "nui_image(logo);\nnui_image_mode(NUI_IMAGE_MODE_STRETCH);\nnui_background_image(logo);");
                    nuiw_row_begin();
                        NUI {
                            nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
                            nui_child_gap(6);
                            _demo_note(font_small, "Native");
                            NUI {
                                nui_padding(8, 8, 8, 8);
                                nui_background_color(0x0d1014ff);
                                nui_border(0xffffff17, 1);
                                nui_corner_radius(4);
                                nui_image(logo);
                            }
                        }
                        NUI {
                            nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
                            nui_child_gap(6);
                            _demo_note(font_small, "Repeat");
                            NUI {
                                nui_fixed(160, 90);
                                nui_background_image(logo);
                                nui_image_mode(NUI_IMAGE_MODE_REPEAT);
                            }
                        }
                        NUI {
                            nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
                            nui_child_gap(6);
                            _demo_note(font_small, "Stretch");
                            NUI {
                                nui_fixed(160, 90);
                                nui_background_image(logo);
                                nui_image_mode(NUI_IMAGE_MODE_STRETCH);
                            }
                        }
                    nuiw_row_end();
                    _demo_note(font_small, "Drag the bottom-right handle to resize this bordered image container.");
                    NUI {
                        nui_fixed(image_width, image_height);
                        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
                        nui_padding(8, 8, 8, 8);
                        nui_child_gap(6);
                        nui_background_color(0x0d1014ff);
                        nui_border(0x5ca4ff66, 1);
                        nui_corner_radius(6);
                        NUI {
                            nui_grow_width();
                            nui_grow_height();
                            nui_background_image(logo);
                            nui_image_mode(pivot[0] == 'R' ? NUI_IMAGE_MODE_REPEAT : NUI_IMAGE_MODE_STRETCH);
                        }
                        nuiw_row_begin();
                            NUI { nui_grow_width(); }
                            nuiw_resize_handle("Image resize", &image_width, &image_height, 180, 120, 560, 340);
                        nuiw_row_end();
                    }
                } else if (demo_page == 4) {
                    _demo_heading(font_title, "Selection");
                    _demo_note(font_small, "Radios return clicks; your app owns the selected value.");
                    _demo_code(font_small, "if (nuiw_radio(\"Edge\", mode == 1)) mode = 1;\nnuiw_tree_row(\"Scene\", &open, false);");
                    if (nuiw_radio("Vertex", radio_mode == 0)) radio_mode = 0;
                    if (nuiw_radio("Edge", radio_mode == 1)) radio_mode = 1;
                    if (nuiw_radio("Face", radio_mode == 2)) radio_mode = 2;
                    nuiw_tree_row("Scene", &tree_open, false);
                    if (tree_open) {
                        bool leaf = false;
                        nuiw_tree_row("Canvas", &leaf, radio_mode == 0);
                        nuiw_tree_row("Layer", &leaf, radio_mode == 1);
                        nuiw_tree_row("Selection bounds", &leaf, radio_mode == 2);
                    }
                } else if (demo_page == 5) {
                    _demo_heading(font_title, "Popups and modals");
                    _demo_note(font_small, "Use popovers for local choices and modals when the rest of the app should be dimmed.");
                    _demo_code(font_small, "if (nuiw_modal_begin(\"Confirm\", &open)) {\n    ...\n    nuiw_modal_end();\n}");
                    if (nuiw_button(popover_open ? "Hide popover" : "Show popover")) popover_open = !popover_open;
                    if (popover_open) {
                        nuiw_popover_begin();
                            if (nuiw_button("Increment counter")) click_count++;
                            nuiw_button("Local action");
                            nuiw_button("Another action");
                        nuiw_popover_end();
                    }
                    if (nuiw_button("Open full-screen modal")) modal_open = true;
                    if (nuiw_modal_begin("Full-screen modal", &modal_open)) {
                        _demo_note(font, "This dialog is rendered over a faded full-screen overlay. Press Escape or close it below.");
                        nuiw_row_begin();
                            if (nuiw_button("Close")) modal_open = false;
                            if (nuiw_button("Increment")) click_count++;
                        nuiw_row_end();
                        nuiw_modal_end();
                    }
                } else if (demo_page == 6) {
                    _demo_heading(font_title, "Color");
                    _demo_note(font_small, "The color widget generates its hue strip and triangle texture at runtime.");
                    _demo_code(font_small, "float rgba[4] = {1, .54f, .13f, 1};\nnuiw_color_rgba(\"Base color\", rgba);");
                    nuiw_color_rgba("Base color", rgba);
                }
            nuiw_panel_end();
        }

        nui_update();
        nuiw_end_frame();
        nui_render();
        glfwSwapBuffers(window);
    }

    if (logo) {
        nui_unload_image(logo);
    }
    nui_unload_font(font_title);
    nui_unload_font(font_large);
    nui_unload_font(font_small);
    nui_unload_font(font);
    nuiw_fini();
    nui_fini();
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
