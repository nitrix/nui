#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "nui.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_truetype.h"
#include <stdint.h>
#include <stdlib.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

struct sw_font {
    unsigned char *font_buffer;
    stbtt_fontinfo info;
    unsigned char *pixels;
    stbtt_bakedchar *baked;
    int first_char;
    size_t num_chars;
    size_t pixels_width;
    size_t pixels_height;
    float scale;
    float ascent;
    float line_height;
};

struct sw_text_bounds {
    int min_x, min_y;
    int max_x, max_y;
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

// Emulates glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) without
// platform-dependent float rounding in generated test images.
uint32_t _blend_color(uint32_t src, uint32_t dst) {
    uint32_t src_r = (src >> 24) & 0xFF;
    uint32_t src_g = (src >> 16) & 0xFF;
    uint32_t src_b = (src >> 8) & 0xFF;
    uint32_t src_a = src & 0xFF;

    uint32_t dst_r = (dst >> 24) & 0xFF;
    uint32_t dst_g = (dst >> 16) & 0xFF;
    uint32_t dst_b = (dst >> 8) & 0xFF;
    uint32_t dst_a = dst & 0xFF;

    uint32_t src_weight = src_a * 255;
    uint32_t dst_weight = dst_a * (255 - src_a);
    uint32_t out_a = (src_weight + dst_weight) / 255;
    uint32_t out_weight = src_weight + dst_weight;

    if (out_weight == 0) {
        return 0;
    }

    uint32_t out_r = (src_r * src_weight + dst_r * dst_weight) / out_weight;
    uint32_t out_g = (src_g * src_weight + dst_g * dst_weight) / out_weight;
    uint32_t out_b = (src_b * src_weight + dst_b * dst_weight) / out_weight;

    uint32_t out_color =
        ((out_r & 0xFF) << 24) |
        ((out_g & 0xFF) << 16) |
        ((out_b & 0xFF) << 8) |
        (out_a & 0xFF);

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

static bool _inside_round_rect(int px, int py, int x, int y, int width, int height, int radius) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (px < x || px >= x + width || py < y || py >= y + height) {
        return false;
    }

    radius = MIN(radius, MIN(width, height) / 2);
    if (radius <= 0) {
        return true;
    }

    int left = x + radius;
    int right = x + width - radius - 1;
    int top = y + radius;
    int bottom = y + height - radius - 1;
    int cx = px < left ? left : (px > right ? right : px);
    int cy = py < top ? top : (py > bottom ? bottom : py);
    int dx = px - cx;
    int dy = py - cy;

    return dx * dx + dy * dy <= radius * radius;
}

void sw_draw_box(int x, int y, int width, int height, uint32_t fill, uint32_t border, int border_width, int radius) {
    int outer_radius = MAX(0, radius);
    int inner_x = x + border_width;
    int inner_y = y + border_width;
    int inner_width = width - border_width * 2;
    int inner_height = height - border_width * 2;
    int inner_radius = MAX(0, outer_radius - border_width);

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            int px = w + x;
            int py = h + y;
            if (!_inside_round_rect(px, py, x, y, width, height, outer_radius)) {
                continue;
            }

            bool is_border = border_width > 0 && !_inside_round_rect(px, py, inner_x, inner_y, inner_width, inner_height, inner_radius);
            uint32_t color = is_border ? border : fill;
            if (color) {
                _blend_draw_at(px, py, color);
            }
        }
    }
}

void sw_draw_rect(int x, int y, int width, int height, uint32_t color) {
    // printf("sw_draw_rect: x=%d, y=%d, width=%d, height=%d, color=0x%08x\n", x, y, width, height, color);

    sw_draw_box(x, y, width, height, color, 0x00000000, 0, 0);
}

void sw_draw_image(int x, int y, int width, int height, const struct nui_image *image, enum nui_image_mode mode) {
    // printf("sw_draw_image: x=%d, y=%d, width=%d, height=%d, image_width=%d, image_height=%d\n", x, y, width, height, image->width, image->height);

    unsigned char *image_pixels = (unsigned char *) image->handle;

    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            int px = x + ix;
            int py = y + iy;

            int ax = ix % image->width;
            int ay = iy % image->height;
            if (mode == NUI_IMAGE_MODE_STRETCH) {
                ax = ix * image->width / width;
                ay = iy * image->height / height;
            }

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
    int first_char = 32;
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
    if (!stbtt_InitFont(&info, font_buffer, 0)) {
        free(font_buffer);
        free(pixels);
        free(baked);
        return NULL;
    }

    int ascent_int, descent_int, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent_int, &descent_int, &line_gap);

    float scale = stbtt_ScaleForPixelHeight(&info, font_size);

    struct sw_font *sw_font = malloc(sizeof *sw_font);
    if (!sw_font) {
        free(font_buffer);
        free(pixels);
        free(baked);
        return NULL;
    }

    sw_font->font_buffer = font_buffer;
    sw_font->info = info;
    sw_font->pixels = pixels;
    sw_font->baked = baked;
    sw_font->first_char = first_char;
    sw_font->num_chars = num_chars;
    sw_font->pixels_width = pixels_width;
    sw_font->pixels_height = pixels_height;
    sw_font->scale = scale;
    sw_font->ascent = ascent_int * scale;
    sw_font->line_height = (ascent_int - descent_int + line_gap) * scale;

    struct nui_font *font = malloc(sizeof *font);
    if (!font) {
        free(font_buffer);
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
    free(sw_font->font_buffer);
    free(sw_font->pixels);
    free(sw_font->baked);
    free(sw_font);
    free(font);
}

static bool _sw_font_has_codepoint(const struct sw_font *font, unsigned char ch) {
    return ch >= font->first_char && (size_t)(ch - font->first_char) < font->num_chars;
}

static float _sw_font_kerning(const struct sw_font *font, unsigned char ch, unsigned char next) {
    if (!_sw_font_has_codepoint(font, next)) {
        return 0;
    }

    return font->scale * stbtt_GetCodepointKernAdvance(&font->info, ch, next);
}

static bool _sw_measure_text_line_bounds(const struct sw_font *font, const char *text, size_t len, struct sw_text_bounds *bounds) {
    float fx = 0;
    float fy = font->ascent;
    bool has_bounds = false;

    for (size_t i = 0; i < len; i++) {
        stbtt_aligned_quad q;
        unsigned char ch = (unsigned char)text[i];

        if (!_sw_font_has_codepoint(font, ch)) {
            continue;
        }

        bool opengl = true;
        stbtt_GetBakedQuad(font->baked, font->pixels_width, font->pixels_height, ch - font->first_char, &fx, &fy, &q, opengl);
        if (i + 1 < len) {
            fx += _sw_font_kerning(font, ch, (unsigned char)text[i + 1]);
        }

        if (!has_bounds) {
            bounds->min_x = (int) q.x0;
            bounds->min_y = (int) q.y0;
            bounds->max_x = (int) q.x1;
            bounds->max_y = (int) q.y1;
            has_bounds = true;
        } else {
            if ((int) q.x0 < bounds->min_x) bounds->min_x = (int) q.x0;
            if ((int) q.y0 < bounds->min_y) bounds->min_y = (int) q.y0;
            if ((int) q.x1 > bounds->max_x) bounds->max_x = (int) q.x1;
            if ((int) q.y1 > bounds->max_y) bounds->max_y = (int) q.y1;
        }
    }

    return has_bounds;
}

static bool _sw_measure_text_bounds(const struct sw_font *font, const char *text, struct sw_text_bounds *bounds) {
    int max_width = 0;
    int max_height = 0;
    bool has_bounds = false;
    size_t line_index = 0;

    for (const char *line = text; ; line_index++) {
        const char *line_end = line;
        while (*line_end && *line_end != '\n') {
            line_end++;
        }

        struct sw_text_bounds line_bounds;
        size_t line_len = (size_t)(line_end - line);
        if (_sw_measure_text_line_bounds(font, line, line_len, &line_bounds)) {
            int line_width = line_bounds.max_x - line_bounds.min_x;
            max_width = MAX(max_width, line_width);

            int line_height = line_bounds.max_y - line_bounds.min_y;
            int line_bottom = (int)(line_index * font->line_height) + line_height;
            max_height = MAX(max_height, line_bottom);
            has_bounds = true;
        }

        if (!*line_end) {
            break;
        }

        line = line_end + 1;
    }

    if (has_bounds) {
        bounds->min_x = 0;
        bounds->max_x = max_width;
        bounds->min_y = 0;
        bounds->max_y = max_height;
    }

    return has_bounds;
}

void sw_measure_text(const struct nui_font *font, const char *text, int *width, int *height) {
    *width = 0;
    *height = 0;

    if (!font || !text) {
        return;
    }

    struct sw_font *sw_font = font->handle;
    struct sw_text_bounds bounds;
    if (!_sw_measure_text_bounds(sw_font, text, &bounds)) {
        return;
    }

    *width = bounds.max_x - bounds.min_x;
    *height = bounds.max_y - bounds.min_y;
}

static void _sw_draw_text_line(const struct sw_font *font, int x, int y, const char *text, size_t len, uint32_t color) {
    struct sw_text_bounds bounds;
    if (!_sw_measure_text_line_bounds(font, text, len, &bounds)) {
        return;
    }

    float fx = 0;
    float fy = font->ascent;

    for (size_t i = 0; i < len; i++) {
        stbtt_aligned_quad q;
        unsigned char ch = (unsigned char)text[i];

        if (!_sw_font_has_codepoint(font, ch)) {
            continue;
        }

        bool opengl = true;
        stbtt_GetBakedQuad(font->baked, font->pixels_width, font->pixels_height, ch - font->first_char, &fx, &fy, &q, opengl);
        if (i + 1 < len) {
            fx += _sw_font_kerning(font, ch, (unsigned char)text[i + 1]);
        }

        for (int iy = (int)q.y0; iy < (int)q.y1; iy++) {
            for (int ix = (int)q.x0; ix < (int)q.x1; ix++) {
                int px = x + ix - bounds.min_x;
                int py = y + iy - bounds.min_y;

                float u = q.s0 + (ix - q.x0) / (q.x1 - q.x0) * (q.s1 - q.s0);
                float v = q.t0 + (iy - q.y0) / (q.y1 - q.y0) * (q.t1 - q.t0);

                int ax = (int)(u * font->pixels_width);
                int ay = (int)(v * font->pixels_height);

                if (ax < 0) ax = 0;
                if (ax >= (int)font->pixels_width) ax = font->pixels_width - 1;
                if (ay < 0) ay = 0;
                if (ay >= (int)font->pixels_height) ay = font->pixels_height - 1;

                uint8_t a = font->pixels[(font->pixels_width * ay + ax)];
                uint32_t src_color = ((color & 0xFFFFFF00) | a);

                _blend_draw_at(px, py, src_color);
            }
        }
    }
}

void sw_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color) {
    struct sw_font *sw_font = font->handle;
    size_t line_index = 0;

    for (const char *line = text; ; line_index++) {
        const char *line_end = line;
        while (*line_end && *line_end != '\n') {
            line_end++;
        }

        int line_y = y + (int)(line_index * sw_font->line_height);
        _sw_draw_text_line(sw_font, x, line_y, line, (size_t)(line_end - line), color);

        if (!*line_end) {
            break;
        }

        line = line_end + 1;
    }
}
