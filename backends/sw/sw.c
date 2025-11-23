#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdint.h>
#include <stdlib.h>

unsigned char *pixels = NULL;
int max_x = 0;
int max_y = 0;

void sw_init(void) {
    // Nothing to do.
}

void sq_fini(void) {
    free(pixels);
}

void sw_before_render(int width, int height) {
    assert(pixels == NULL);

    pixels = malloc(width * height * 4);

    max_x = width;
    max_y = height;

    // Clear new area to transparent black.
    for (int y = 0; y < max_y; y++) {
        for (int x = 0; x < max_x; x++) {
            unsigned char *at = &pixels[(max_x * y + x)*4];
            at[0] = 0;
            at[1] = 0;
            at[2] = 0;
            at[3] = 0;
        }
    }
}

void sw_draw_rect(int x, int y, int width, int height, uint32_t color) {
    // printf("sw_draw_rect: x=%d, y=%d, width=%d, height=%d, color=0x%08x\n", x, y, width, height, color);

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            int px = w + x;
            int py = h + y;

            if (px >= 0 && px < max_x && py >= 0 && py < max_y) {
                unsigned char r = (color >> 24) & 0xFF;
                unsigned char g = (color >> 16) & 0xFF;
                unsigned char b = (color >> 8) & 0xFF;
                unsigned char a = (color >> 0) & 0xFF;

                unsigned char *at = &pixels[(max_x * py + px)*4];

                at[0] = r;
                at[1] = g;
                at[2] = b;
                at[3] = a;
            }
        }
    }
}

void sw_export(const char *filepath) {
    stbi_write_png(filepath, max_x, max_y, 4, pixels, max_x * 4);
}
