#include "testing.h"

int sanity_fixed_width_only(int argc, char **argv) {
    TEST_BEFORE();

    nui_viewport(1280, 720);

    NUI {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_fixed_width(1200);
        nui_background_color(BLUE);
        nui_padding(32, 32, 32, 32);
        nui_child_gap(32);

        NUI {
            nui_fixed(300, 300);
            nui_background_color(PINK);
        }

        NUI {
            nui_fixed(350, 200);
            nui_background_color(YELLOW);
        }
    }

    TEST_AFTER();
    return TEST_RESULT();
}
