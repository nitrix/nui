#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "nui.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_truetype.h"
#include <stdint.h>
#include <stdlib.h>

struct sw_font {
    unsigned char *pixels;
    stbtt_bakedchar *baked;
    size_t num_chars;
    size_t pixels_width;
    size_t pixels_height;
    float ascent;
};

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
    // printf("sw_draw_image: x=%d, y=%d, width=%d, height=%d, image_width=%d, image_height=%d\n", x, y, width, height, image->width, image->height);

    unsigned char *image_pixels = (unsigned char *) image->handle;

    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            int px = x + ix;
            int py = y + iy;

            int ax = ix % image->width;
            int ay = iy % image->height;

            uint8_t r = image_pixels[(image->width * ay + ax)*4 + 0];
            uint8_t g = image_pixels[(image->width * ay + ax)*4 + 1];
            uint8_t b = image_pixels[(image->width * ay + ax)*4 + 2];
            uint8_t a = image_pixels[(image->width * ay + ax)*4 + 3];
            uint32_t color = (r << 24) | (g << 16) | (b << 8) | a;

            _blend_draw_at(px, py, color);
        }
    }
}

void sw_export(const char *filepath) {
    // printf("sw_export: %s\n", filepath);

    stbi_write_png(filepath, max_x, max_y, 4, pixels, max_x * 4);
    free(pixels);
}

struct nui_font *sw_load_font_from_file(const char *filename, float font_size) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *font_buffer = malloc(file_size);
    if (!font_buffer) {
        fclose(file);
        return NULL;
    }

    unsigned long long n = fread(font_buffer, 1, file_size, file);
    fclose(file);

    if (n != file_size) {
        free(font_buffer);
        return NULL;
    }

    size_t pixels_width = 512;
    size_t pixels_height = 512;
    char first_char = 32;
    size_t num_chars = 255 - first_char;
    unsigned char *pixels = malloc(pixels_width*pixels_height);
    if (!pixels) {
        free(font_buffer);
        return NULL;
    }

    stbtt_bakedchar *baked = malloc(sizeof *baked * num_chars);
    if (!baked) {
        free(font_buffer);
        free(pixels);
        return NULL;
    }

    // TODO: Check for failures, try bigger image repeatedly.
    stbtt_BakeFontBitmap(font_buffer, 0, font_size, pixels, pixels_width, pixels_height, first_char, num_chars, baked);

    stbtt_fontinfo info;
    stbtt_InitFont(&info, font_buffer, 0);

    int ascent_int, descent_int, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent_int, &descent_int, &line_gap);

    float scale = stbtt_ScaleForPixelHeight(&info, font_size);

    free(font_buffer);

    struct sw_font *sw_font = malloc(sizeof *sw_font);
    if (!sw_font) {
        free(pixels);
        free(baked);
        return NULL;
    }

    sw_font->pixels = pixels;
    sw_font->baked = baked;
    sw_font->num_chars = num_chars;
    sw_font->pixels_width = pixels_width;
    sw_font->pixels_height = pixels_height;
    sw_font->ascent = ascent_int * scale;

    struct nui_font *font = malloc(sizeof *font);
    if (!font) {
        free(pixels);
        free(baked);
        free(sw_font);
        return NULL;
    }

    font->handle = sw_font;

    return font;
}

void sw_unload_font(struct nui_font *font) {
    struct sw_font *sw_font = font->handle;
    free(sw_font->pixels);
    free(sw_font->baked);
    free(sw_font);
    free(font);
}

void sw_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color) {
    printf("SW_DRAW_TEXT\n");

    struct sw_font *sw_font = font->handle;

    float fx = 0;
    float fy = sw_font->ascent;

    while (*text) {
        stbtt_aligned_quad q;

        bool opengl = true;
        stbtt_GetBakedQuad(sw_font->baked, sw_font->pixels_width, sw_font->pixels_height, *text-32, &fx, &fy, &q, opengl);

        printf("char='%c' q.x0=%f q.y0=%f q.x1=%f q.y1=%f s0=%f t0=%f s1=%f t1=%f\n",
            *text, q.x0, q.y0, q.x1, q.y1, q.s0, q.t0, q.s1, q.t1);

        for (int iy = (int)q.y0; iy < (int)q.y1; iy++) {
            for (int ix = (int)q.x0; ix < (int)q.x1; ix++) {
                int px = x + ix;
                int py = y + iy;

                float u = q.s0 + (ix - q.x0) / (q.x1 - q.x0) * (q.s1 - q.s0);
                float v = q.t0 + (iy - q.y0) / (q.y1 - q.y0) * (q.t1 - q.t0);

                int ax = (int)(u * sw_font->pixels_width);
                int ay = (int)(v * sw_font->pixels_height);

                if (ax < 0) ax = 0;
                if (ax >= (int)sw_font->pixels_width) ax = sw_font->pixels_width - 1;
                if (ay < 0) ay = 0;
                if (ay >= (int)sw_font->pixels_height) ay = sw_font->pixels_height - 1;

                uint8_t a = sw_font->pixels[(sw_font->pixels_width * ay + ax)];
                uint32_t src_color = ((color & 0xFFFFFF00) | a);

                _blend_draw_at(px, py, src_color);
            }
        }

        text++;
    }
}
