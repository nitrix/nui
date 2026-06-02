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
    char name[64] = "Cube.001";
    bool visible = true;
    bool selectable = true;
    bool tree_open = true;
    bool show_bounds = false;
    int demo_page = 0;
    int radio_mode = 0;
    float scale = 0.62f;
    float alpha = 0.78f;
    float roughness = 0.42f;
    float rgba[4] = {1.0f, 0.54f, 0.13f, 1.0f};
    const char *pivot = "Median Point";

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
                NUI {
                    nui_font_color(nuiw_theme_high_contrast()->accent);
                    nui_text("nui demo");
                }
                NUI {
                    nui_font_color(0xb8c0ccff);
                    nui_text("Immediate-mode widgets");
                }
                if (nuiw_tab("Overview", demo_page == 0)) demo_page = 0;
                if (nuiw_tab("Buttons", demo_page == 1)) demo_page = 1;
                if (nuiw_tab("Inputs", demo_page == 2)) demo_page = 2;
                if (nuiw_tab("Sliders", demo_page == 3)) demo_page = 3;
                if (nuiw_tab("Selection", demo_page == 4)) demo_page = 4;
                if (nuiw_tab("Popups", demo_page == 5)) demo_page = 5;
                if (nuiw_tab("Color", demo_page == 6)) demo_page = 6;
            nuiw_panel_end();

            nuiw_panel_begin();
                nui_grow();
                if (demo_page == 0) {
                    NUI {
                        nui_font_color(nuiw_theme_high_contrast()->accent);
                        nui_text("High Contrast widget demo");
                    }
                    nuiw_row_begin();
                        nuiw_button("Run");
                        nuiw_button("Apply");
                        nuiw_button("Reset");
                    nuiw_row_end();
                    nuiw_checkbox("Visible in viewport", &visible);
                    nuiw_checkbox("Selectable", &selectable);
                    nuiw_slider_float("Scale", &scale, 0.0f, 1.0f);
                    NUI {
                        nui_fixed(420, 180);
                        nui_background_color(0x10151bff);
                        nui_border(nuiw_theme_high_contrast()->line, 1);
                        nui_corner_radius(nuiw_theme_high_contrast()->radius);
                        nui_padding(16, 16, 16, 16);
                        NUI {
                            nui_fixed(110, 110);
                            nui_margin(22, 0, 0, 140);
                            nui_background_color(0xff8a2261);
                            nui_border(nuiw_theme_high_contrast()->accent, 2);
                            nui_corner_radius(4);
                        }
                    }
                } else if (demo_page == 1) {
                    NUI {
                        nui_font_color(nuiw_theme_high_contrast()->accent);
                        nui_text("Buttons");
                    }
                    nuiw_row_begin();
                        if (nuiw_button("Button")) alpha = 0.5f;
                        if (nuiw_button("Small")) alpha = 0.75f;
                        if (nuiw_button("Apply")) alpha = 1.0f;
                    nuiw_row_end();
                    nuiw_row_begin();
                        if (nuiw_tab("Object", radio_mode == 0)) radio_mode = 0;
                        if (nuiw_tab("Edit", radio_mode == 1)) radio_mode = 1;
                        if (nuiw_tab("Sculpt", radio_mode == 2)) radio_mode = 2;
                    nuiw_row_end();
                    nuiw_checkbox("Show bounds", &show_bounds);
                } else if (demo_page == 2) {
                    NUI {
                        nui_font_color(nuiw_theme_high_contrast()->accent);
                        nui_text("Text and combo inputs");
                    }
                    nuiw_input_text("Object name", name, sizeof name);
                    bool combo_open = nuiw_combo_begin("Pivot", pivot);
                    if (combo_open) {
                        if (nuiw_combo_item("Median Point", pivot[0] == 'M')) pivot = "Median Point";
                        if (nuiw_combo_item("Bounding Box", pivot[0] == 'B')) pivot = "Bounding Box";
                        if (nuiw_combo_item("3D Cursor", pivot[0] == '3')) pivot = "3D Cursor";
                    }
                    nuiw_combo_end();
                    nuiw_checkbox("Visible", &visible);
                    nuiw_checkbox("Selectable", &selectable);
                } else if (demo_page == 3) {
                    NUI {
                        nui_font_color(nuiw_theme_high_contrast()->accent);
                        nui_text("Sliders");
                    }
                    nuiw_slider_float("Scale", &scale, 0.0f, 1.0f);
                    nuiw_slider_float("Alpha", &alpha, 0.0f, 1.0f);
                    nuiw_slider_float("Roughness", &roughness, 0.0f, 1.0f);
                } else if (demo_page == 4) {
                    NUI {
                        nui_font_color(nuiw_theme_high_contrast()->accent);
                        nui_text("Selection, radios, and trees");
                    }
                    if (nuiw_radio("Vertex", radio_mode == 0)) radio_mode = 0;
                    if (nuiw_radio("Edge", radio_mode == 1)) radio_mode = 1;
                    if (nuiw_radio("Face", radio_mode == 2)) radio_mode = 2;
                    nuiw_tree_row("Scene Collection", &tree_open, false);
                    if (tree_open) {
                        bool leaf = false;
                        nuiw_tree_row("Cube", &leaf, true);
                        nuiw_tree_row("Camera", &leaf, false);
                        nuiw_tree_row("Area Light", &leaf, false);
                    }
                } else if (demo_page == 5) {
                    NUI {
                        nui_font_color(nuiw_theme_high_contrast()->accent);
                        nui_text("Popups and dialog-like panels");
                    }
                    nuiw_popover_begin();
                        nuiw_button("Increment");
                        nuiw_button("Vertex");
                        nuiw_button("Edge");
                    nuiw_popover_end();
                    nuiw_dialog_begin();
                        NUI {
                            nui_text("Apply Modifier?");
                        }
                        nuiw_row_begin();
                            nuiw_button("Cancel");
                            nuiw_button("Apply Modifier");
                        nuiw_row_end();
                    nuiw_dialog_end();
                } else if (demo_page == 6) {
                    NUI {
                        nui_font_color(nuiw_theme_high_contrast()->accent);
                        nui_text("Color");
                    }
                    nuiw_color_rgba("Base color", rgba);
                }
            nuiw_panel_end();
        }

        nui_update();
        nuiw_end_frame();
        nui_render();
        glfwSwapBuffers(window);
    }

    nui_unload_font(font);
    nuiw_fini();
    nui_fini();
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
