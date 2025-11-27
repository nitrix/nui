#include "testing.h"

int sanity_blend_and_image_repeat(int argc, char **argv) {
    TEST_BEFORE();

    struct nui_image *logo = nui_load_image_from_file("../assets/nui.png");
    assert(logo);

    nui_frame();

    NUI {
        nui_fixed_width(1200);
        nui_background_color(BLUE);
        nui_padding(32, 32, 32, 32);
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);

        NUI {
            nui_fixed_height(100);
            nui_grow_width();
            nui_background_color(0xFF000033);
            nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);

            NUI_IMAGE(logo);

            NUI {
                nui_fixed_width(200);
                nui_fixed_height(21);
                nui_background_image(logo);
            }
        }

        NUI {
            nui_child_gap(32);
            nui_grow_width();

            NUI {
                nui_fixed_height(100);
                nui_grow_width();
                nui_background_color(PINK);
            }

            NUI {
                nui_fixed_height(100);
                nui_grow_width();
            }

            NUI {
                nui_fixed_height(100);
                nui_grow_width();
                nui_background_color(PINK);
            }
        }
    }

    TEST_AFTER();

    nui_unload_image(logo);

    return TEST_RESULT();
}
