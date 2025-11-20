#include "testing.h"

// Specficially ends with 2 pixels to distribute unfairly among 3 growable elements.
// Only the first two gets 1 pixel each, the last one doesn't.

int sanity_triple_grow_unfair(int argc, char **argv) {
    TEST_BEFORE();

    NUI {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_fixed_width(1200);
        nui_fixed_height(350);
        nui_background_color(BLUE);
        nui_padding(32, 32, 32, 32);
        nui_child_gap(32);

        NUI {
            nui_fixed(200, 200);
            nui_background_color(PINK);
        }

        NUI {
            nui_fixed_width(50);
            nui_grow_height();
            nui_grow_width();
            nui_background_color(YELLOW);
        }

        NUI {
            nui_fixed_width(50);
            nui_grow_height();
            nui_grow_width();
            nui_background_color(YELLOW);
        }

        NUI {
            nui_fixed_width(10);
            nui_grow_height();
            nui_grow_width();
            nui_background_color(YELLOW);
        }

        NUI {
            nui_fixed(200, 200);
            nui_background_color(LIGHT_BLUE);
        }
    }

    TEST_AFTER();
    return TEST_RESULT();
}
