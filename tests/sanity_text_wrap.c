#include "layout_testing.h"

int sanity_text_wrap(int argc, char **argv) {
    (void)argc;
    (void)argv;

    nui_init(&layout_test_backend);
    nui_viewport(70, 100);

    NUI {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_background_color(BLUE);
        nui_font(&layout_test_font);

        NUI {
            nui_background_color(PINK);
            nui_text("aaa bbb ccc");
        }
    }

    nui_update();
    nui_render();

    const struct layout_test_rect *container = layout_test_find_rect(BLUE);
    const struct layout_test_rect *text = layout_test_find_rect(PINK);

    assert(container);
    assert(text);
    assert(container->w == 70);
    assert(text->w == 70);
    assert(text->h == 20);
    assert(strcmp(layout_test.text, "aaa bbb\nccc") == 0);

    nui_fini();
    return EXIT_SUCCESS;
}
