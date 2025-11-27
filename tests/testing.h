#ifndef TESTING_H
#define TESTING_H

// IWYU pragma: begin_keep
#include "nui.h"
#include "sw.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
// IWYU pragma: end_keep

#define BLUE 0x0a6d89ff
#define PINK 0xf1928fff
#define YELLOW 0xfed84dff
#define LIGHT_BLUE 0x5ecbe4ff

static char output_filepath[2048];
static char expected_filepath[2048];

static bool _compare_files(const char *file1, const char *file2) {
    FILE *f1 = fopen(file1, "rb");
    FILE *f2 = fopen(file2, "rb");

    if (!f1 || !f2) {
        fprintf(stderr, "Failed to open files for comparison: %s, %s\n", file1, file2);
        return false;
    }

    bool diff_found = false;
    int byte1, byte2;
    size_t pos = 0;

    while (1) {
        byte1 = fgetc(f1);
        byte2 = fgetc(f2);

        if (byte1 == EOF && byte2 == EOF) {
            break; // Both files ended
        }

        if (byte1 != byte2) {
            diff_found = true;
            fprintf(stderr, "Files differ at byte %zu: %02X != %02X\n", pos, byte1, byte2);
            break;
        }

        pos++;
    }

    fclose(f1);
    fclose(f2);

    if (diff_found) {
        return false;
    }

    return true;
}

#define TEST_BEFORE(body) \
        (void)argc; \
        (void)argv; \
        nui_init(NUI_BACKEND_SW); \

#define TEST_AFTER() \
        nui_update(); \
        nui_render(); \
        snprintf(output_filepath, sizeof output_filepath, "output/%s.png", __func__); \
        snprintf(expected_filepath, sizeof expected_filepath, "expected/%s.png", __func__); \
        sw_export(output_filepath); \
        nui_fini(); \
        bool result = _compare_files(output_filepath, expected_filepath); \

#define TEST_RESULT() (result ? EXIT_SUCCESS : EXIT_FAILURE)

#endif
