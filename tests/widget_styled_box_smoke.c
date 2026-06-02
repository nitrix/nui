#include "testing.h"

int widget_styled_box_smoke(int argc, char **argv) {
    (void)argc;
    (void)argv;

    nui_init(NUI_BACKEND_SW);
    nui_viewport(160, 90);

    nui_frame();
    NUI {
        nui_fixed(160, 90);
        nui_background_color(0x06080aff);
        nui_padding(12, 12, 12, 12);

        NUI {
            nui_fixed(80, 28);
            nui_background_color(0x343b45ff);
            nui_border(0xff8a22ff, 2);
            nui_corner_radius(6);
        }
    }

    nui_update();
    nui_render();
    sw_export("output/widget_styled_box_smoke.png");
    nui_fini();
    return EXIT_SUCCESS;
}
