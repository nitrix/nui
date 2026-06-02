#include "layout_testing.h"

int sanity_fixed_width_no_shrink(int argc, char **argv) {
    (void)argc;
    (void)argv;

    nui_init(&layout_test_backend);
    nui_viewport(70, 100);

    NUI {
        nui_layout(NUI_LAYOUT_LEFT_TO_RIGHT);
        nui_background_color(BLUE);

        NUI {
            nui_fixed_width(100);
            nui_fixed_height(10);
            nui_background_color(PINK);
        }
    }

    nui_update();
    nui_render();

    const struct layout_test_rect *fixed = layout_test_find_rect(PINK);

    assert(fixed);
    assert(fixed->w == 100);

    nui_fini();
    return EXIT_SUCCESS;
}
