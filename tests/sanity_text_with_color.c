#include "testing.h"

int sanity_text_with_color(int argc, char **argv) {
    TEST_BEFORE();

    struct nui_font *font = nui_load_font_from_file("../assets/DejaVuSans.ttf", 64.0f);

    NUI {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_fixed(1000, 300);
        nui_background_color(BLUE);
        nui_font(font);

        NUI {
            nui_grow();
            nui_background_color(PINK);
        }

        NUI {
            nui_grow();
            nui_background_color(PINK);
            nui_font_color(YELLOW);
            nui_text("Hello World!");
        }
    }

    TEST_AFTER();

    nui_unload_font(font);

    return TEST_RESULT();
}
