#include "testing.h"

int sanity_text_fit(int argc, char **argv) {
    TEST_BEFORE();

    struct nui_font *font = nui_load_font_from_file("../assets/DejaVuSans.ttf", 64.0f);
    assert(font);

    NUI {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_background_color(BLUE);
        nui_padding(16, 16, 16, 16);
        nui_font(font);

        NUI {
            nui_background_color(PINK);
            nui_padding(16, 16, 16, 16);
            nui_font_color(YELLOW);
            nui_text("AV");
        }
    }

    TEST_AFTER();

    nui_unload_font(font);

    return TEST_RESULT();
}
