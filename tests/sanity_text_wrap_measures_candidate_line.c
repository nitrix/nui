#include "layout_testing.h"

int sanity_text_wrap_measures_candidate_line(int argc, char **argv) {
    (void)argc;
    (void)argv;

    layout_test.word_join_extra_width = 10;

    nui_init(&layout_test_backend);
    nui_viewport(65, 100);

    NUI {
        nui_layout(NUI_LAYOUT_TOP_TO_BOTTOM);
        nui_background_color(BLUE);
        nui_font(&layout_test_font);

        NUI {
            nui_background_color(PINK);
            nui_text("aaaa b c");
        }
    }

    nui_update();
    nui_render();

    const struct layout_test_rect *text = layout_test_find_rect(PINK);

    assert(text);
    assert(text->w == 65);
    assert(strcmp(layout_test.text, "aaaa\nb c") == 0);

    nui_fini();
    layout_test.word_join_extra_width = 0;
    return EXIT_SUCCESS;
}
