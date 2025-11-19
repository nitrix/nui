#include "nui.h"
#include "testing.h"

int simple_layout_top2bottom(int argc, char **argv) {
    TEST_BEFORE();

    nui_viewport(960, 540);

    NUI {
        nui_fixed(960, 540);
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_background_color(BLUE);
        nui_padding(32, 32, 32, 32);

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
