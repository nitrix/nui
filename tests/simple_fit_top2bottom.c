#include "testing.h"

int simple_fit_top2bottom(int argc, char **argv) {
    TEST_BEFORE();

    NUI {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
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
