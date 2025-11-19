#include "testing.h"

int simple_child_gap(int argc, char **argv) {
    TEST_BEFORE();

    nui_viewport(960, 540);

    NUI {
        nui_fixed(960, 540);
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
