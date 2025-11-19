#include "testing.h"

int simple_two_elements(int argc, char **argv) {
    TEST_BEFORE();

    nui_viewport(960, 540);

    NUI {
        nui_fixed(960, 540);
        nui_background_color(BLUE);

        NUI {
            nui_fixed(300, 300);
            nui_background_color(PINK);
        }
    }

    TEST_AFTER();
    return TEST_RESULT();
}
