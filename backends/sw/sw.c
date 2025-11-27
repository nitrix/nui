#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "nui.h"
#include "stb_image.h"
#include <stdint.h>
#include <stdlib.h>

unsigned char *pixels = NULL;
int max_x = 0;
int max_y = 0;

void sw_init(void) {
    // Nothing to do.
}

void sw_fini(void) {
    // Nothing to do.
}

void sw_before_render(int width, int height) {
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

void sw_after_render(void) {
    // Keep pixels until the export call.
}

struct nui_image *sw_load_image_from_file(const char *filename) {
    // printf("sw_load_image_from_file: %s\n", filename);

    int width, height, channels;
    unsigned char *pixels = stbi_load(filename, &width, &height, &channels, 4);
    if (!pixels) {
        return NULL;
    }

    struct nui_image *image = malloc(sizeof *image);
    if (!image) {
        stbi_image_free(pixels);
        return NULL;
    }

    image->width = width;
    image->height = height;
    image->handle = pixels;

    return image;
}

void sw_unload_image(struct nui_image *image) {
    // printf("sw_unload_image\n");

    stbi_image_free(image->handle);
    free(image);
}

// Emulates glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
uint32_t _blend_color(uint32_t src, uint32_t dst) {
    float src_a = (src & 0xFF) / 255.0f;
    float dst_a = (dst & 0xFF) / 255.0f;

    float out_a = src_a + dst_a * (1 - src_a);
    if (out_a == 0) {
        return 0;
    }

    float out_r = (((src >> 24) & 0xFF) * src_a + ((dst >> 24) & 0xFF) * dst_a * (1 - src_a)) / out_a;
    float out_g = (((src >> 16) & 0xFF) * src_a + ((dst >> 16) & 0xFF) * dst_a * (1 - src_a)) / out_a;
    float out_b = (((src >> 8) & 0xFF) * src_a + ((dst >> 8) & 0xFF) * dst_a * (1 - src_a)) / out_a;

    uint32_t out_color =
        (((uint32_t)(out_r) & 0xFF) << 24) |
        (((uint32_t)(out_g) & 0xFF) << 16) |
        (((uint32_t)(out_b) & 0xFF) << 8) |
        (((uint32_t)(out_a * 255.0f) & 0xFF));

    return out_color;
}

void _blend_draw_at(int x, int y, uint32_t src_color) {
    if (x >= 0 && x < max_x && y >= 0 && y < max_y) {
        unsigned char *at = &pixels[(max_x * y + x)*4];

        uint32_t dst_color = (((uint32_t)at[0]) << 24) | (((uint32_t)at[1]) << 16) | (((uint32_t)at[2]) << 8) | (((uint32_t)at[3]) << 0);
        uint32_t out_color = _blend_color(src_color, dst_color);

        at[0] = (out_color >> 24) & 0xFF;
        at[1] = (out_color >> 16) & 0xFF;
        at[2] = (out_color >> 8) & 0xFF;
        at[3] = (out_color >> 0) & 0xFF;
    }
}

void sw_draw_rect(int x, int y, int width, int height, uint32_t color) {
    // printf("sw_draw_rect: x=%d, y=%d, width=%d, height=%d, color=0x%08x\n", x, y, width, height, color);

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            int px = w + x;
            int py = h + y;

            if (px >= 0 && px < max_x && py >= 0 && py < max_y) {
                _blend_draw_at(px, py, color);
            }
        }
    }
}

void sw_draw_image(int x, int y, int width, int height, struct nui_image *image) {
    // FIXME: Support width/height scaling.

    // printf("sw_draw_image: x=%d, y=%d, width=%d, height=%d, image_width=%d, image_height=%d\n", x, y, width, height, image->width, image->height);

    unsigned char *image_pixels = (unsigned char *) image->handle;

    for (int h = 0; h < image->height; h++) {
        for (int w = 0; w < image->width; w++) {
            int px = w + x;
            int py = h + y;
            _blend_draw_at(px, py,
                (((uint32_t)image_pixels[(image->width * h + w)*4 + 0]) << 24) |
                (((uint32_t)image_pixels[(image->width * h + w)*4 + 1]) << 16) |
                (((uint32_t)image_pixels[(image->width * h + w)*4 + 2]) << 8) |
                (((uint32_t)image_pixels[(image->width * h + w)*4 + 3]) << 0)
            );
        }
    }
}

void sw_export(const char *filepath) {
    // printf("sw_export: %s\n", filepath);

    stbi_write_png(filepath, max_x, max_y, 4, pixels, max_x * 4);
    free(pixels);
}
