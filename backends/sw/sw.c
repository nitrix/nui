#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdint.h>
#include <stdio.h>
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

void sw_prepare(int width, int height) {
    // Nothing to do.
}

void sw_draw_rect(int x, int y, int width, int height, uint32_t color) {
    // printf("sw_draw_rect: x=%d, y=%d, width=%d, height=%d, color=0x%08x\n", x, y, width, height, color);

    if (x + width > max_x || y + height > max_y) {
        int new_max_x = x + width;
        int new_max_y = y + height;

        size_t new_bytes = (new_max_x) * (new_max_y) * 4;
        unsigned char *new_pixels = malloc(new_bytes);
        if (!new_pixels) {
            fprintf(stderr, "sw_draw_rect: Failed to allocate memory for pixels\n");
            return;
        }

        // Clear new area to transparent black.
        for (int py = 0; py < new_max_y; py++) {
            for (int px = 0; px < new_max_x; px++) {
                if (px >= max_x || py >= max_y) {
                    unsigned char *at = &new_pixels[(new_max_x * py + px)*4];
                    at[0] = 0;
                    at[1] = 0;
                    at[2] = 0;
                    at[3] = 0;
                }
            }
        }

        // Copy old pixels.
        for (int py = 0; py < max_y; py++) {
            for (int px = 0; px < max_x; px++) {
                unsigned char *src = &pixels[(max_x * py + px)*4];
                unsigned char *dst = &new_pixels[(new_max_x * py + px)*4];
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = src[3];
            }
        }

        free(pixels);

        pixels = new_pixels;
        max_x = new_max_x;
        max_y = new_max_y;
    }

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
    stbi_write_png(filepath, max_x, max_y, 4, pixels, max_x * sizeof (uint32_t));
}
