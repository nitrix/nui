#include "testing.h"

int simple_element(int argc, char **argv) {
    TEST_BEFORE();

    nui_viewport(960, 540);

    NUI {
        nui_fixed(960, 540);
        nui_background_color(0x0A6D89FF);
    }

    TEST_AFTER();
    return TEST_RESULT();
}
