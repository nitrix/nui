#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char *pixels = NULL;
int dimx = 0;
int dimy = 0;

void sw_init(void) {
    // Nothing to do.
}

void sq_fini(void) {
    // Nothing to do.
}

void sw_prepare(int width, int height) {
    size_t bytes = (width) * (height) * 4;
    pixels = realloc(pixels, bytes);
    dimx = width;
    dimy = height;
}

void sw_draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            int px = w + x;
            int py = h + y;

            if (px >= 0 && px < dimx && py >= 0 && py < dimy) {
                unsigned char r = (color >> 24) & 0xFF;
                unsigned char g = (color >> 16) & 0xFF;
                unsigned char b = (color >> 8) & 0xFF;
                unsigned char a = (color >> 0) & 0xFF;

                unsigned char *at = &pixels[(dimx * py + px)*4];

                at[0] = r;
                at[1] = g;
                at[2] = b;
                at[3] = a;
            }
        }
    }
}

void sw_export(const char *filepath) {
    stbi_write_png(filepath, dimx, dimy, 4, pixels, dimx * sizeof (uint32_t));
}
