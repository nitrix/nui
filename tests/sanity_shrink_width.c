#include "layout_testing.h"

int sanity_shrink_width(int argc, char **argv) {
    (void)argc;
    (void)argv;

    nui_init(&layout_test_backend);
    nui_viewport(150, 100);

    NUI {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_background_color(BLUE);
        nui_font(&layout_test_font);

        NUI {
            nui_background_color(PINK);
            nui_text("aaa bbb ccc");
        }

        NUI {
            nui_fixed_width(100);
            nui_fixed_height(10);
            nui_background_color(YELLOW);
        }
    }

    nui_update();
    nui_render();

    const struct layout_test_rect *flexible = layout_test_find_rect(PINK);
    const struct layout_test_rect *fixed = layout_test_find_rect(YELLOW);

    assert(flexible);
    assert(fixed);
    assert(flexible->w == 50);
    assert(fixed->x == 50);
    assert(fixed->w == 100);

    nui_fini();
    return EXIT_SUCCESS;
}
