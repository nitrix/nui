#include "testing.h"

int sanity_loop(int argc, char **argv) {
    TEST_BEFORE();

    NUI {
        nui_fixed_width(1200);
        nui_background_color(BLUE);
        nui_padding(32, 32, 32, 32);
        nui_child_gap(2);

        for (size_t i = 0; i < 100; i++) {
            NUI {
                nui_fixed_height(100);
                nui_grow_width();
                nui_background_color(PINK);
            }
        }
    }

    TEST_AFTER();
    return TEST_RESULT();
}
