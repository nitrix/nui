#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "glad/gl.h"
#include "nui.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include <stdio.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

static GLuint program;
static GLuint vao, vbo;
static struct {
    int width, height;
} viewport;
static struct {
    GLint position, size, color, border_color;
    GLint border_width, radius;
    GLint viewport, image_mode;
    GLint texture, mode, image_size;
    GLint uv_offset, uv_scale;
} uniforms;
static struct {
    GLboolean blend;
    GLint blend_src;
    GLint blend_dst;
} previous;

struct ngl_font {
    GLuint texture;
    unsigned char *font_buffer;
    stbtt_fontinfo info;
    stbtt_bakedchar *baked;
    int first_char;
    size_t num_chars;
    size_t pixels_width;
    size_t pixels_height;
    float scale;
    float ascent;
    float line_height;
};

struct ngl_text_bounds {
    int min_x, min_y;
    int max_x, max_y;
};

enum {
    MODE_NORMAL = 0,
    MODE_IMAGE = 1,
    MODE_TEXT = 2,
};

#define SHADER_SOURCE(...) "#version 410 core\n" #__VA_ARGS__

void _load_shader(void) {
    const char *vertex_shader_src = SHADER_SOURCE(
        \x20 // Trick to avoid my editor formatter ruining the indentation.

        in vec2 a_position;
        in vec2 a_uv;

        out vec2 v_uv;

        uniform int u_mode; // 0 = normal, 1 = image, 2 = text.
        uniform vec2 u_position;
        uniform vec2 u_size;
        uniform vec2 u_viewport;
        uniform vec2 u_image_size;
        uniform int u_image_mode; // 0 = repeat, 1 = stretch.
        uniform vec2 u_uv_offset;
        uniform vec2 u_uv_scale;

        void main() {
            vec2 pos = u_position + a_position * u_size;
            float ndc_x = (pos.x / u_viewport.x) * 2 - 1;
            float ndc_y = (pos.y / u_viewport.y) * -2 + 1;

            gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);
            v_uv = a_uv;

            if (u_mode == 1) {
                if (u_image_mode == 1) {
                    v_uv = a_uv;
                } else {
                    v_uv = a_uv * (u_size / u_image_size);
                }
            } else if (u_mode == 2) {
                v_uv = u_uv_offset + a_uv * u_uv_scale;
            }
        }
    );

    const char *fragment_shader_src = SHADER_SOURCE(
        \x20 // Trick to avoid my editor formatter ruining the indentation.

        in vec2 v_uv;
        out vec4 frag_color;

        uniform int u_mode; // 0 = normal, 1 = image, 2 = text.
        uniform vec4 u_color;
        uniform vec4 u_border_color;
        uniform float u_border_width;
        uniform float u_radius;
        uniform vec2 u_size;
        uniform sampler2D u_texture;

        void main() {
            if (u_mode == 1) {
                frag_color = texture(u_texture, v_uv);
            } else if (u_mode == 2) {
                frag_color = u_color;
                frag_color.a = texture(u_texture, v_uv).a;
            } else {
                vec2 p = v_uv * u_size;
                float radius = min(u_radius, min(u_size.x, u_size.y) * 0.5);
                float border = max(u_border_width, 0.0);
                float edge_distance = min(min(p.x, p.y), min(u_size.x - p.x, u_size.y - p.y));
                bool border_pixel = border > 0.0 && edge_distance < border;

                if (radius > 0.0) {
                    vec2 half_size = u_size * 0.5;
                    vec2 q = abs(p - half_size) - (half_size - vec2(radius));
                    float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
                    if (dist > 0.0) {
                        discard;
                    }
                    border_pixel = border > 0.0 && dist > -border;
                }

                frag_color = border_pixel ? u_border_color : u_color;
            }
        }
    );

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const char * const *) &vertex_shader_src, NULL);
    glCompileShader(vertex_shader);

    // Check success.
    int vertex_shader_success = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_success);
    if (!vertex_shader_success) {
        char info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        fprintf(stderr, "Vertex shader compilation failed: %s\n", info_log);
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const char * const *) &fragment_shader_src, NULL);
    glCompileShader(fragment_shader);

    int fragment_shader_success = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &fragment_shader_success);
    if (!fragment_shader_success) {
        char info_log[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        fprintf(stderr, "Fragment shader compilation failed: %s\n", info_log);
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int link_success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_success);
    if (!link_success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        fprintf(stderr, "Shader program linking failed: %s\n", info_log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    uniforms.position = glGetUniformLocation(program, "u_position");
    uniforms.size = glGetUniformLocation(program, "u_size");
    uniforms.color = glGetUniformLocation(program, "u_color");
    uniforms.border_color = glGetUniformLocation(program, "u_border_color");
    uniforms.border_width = glGetUniformLocation(program, "u_border_width");
    uniforms.radius = glGetUniformLocation(program, "u_radius");
    uniforms.viewport = glGetUniformLocation(program, "u_viewport");
    uniforms.texture = glGetUniformLocation(program, "u_texture");
    uniforms.mode = glGetUniformLocation(program, "u_mode");
    uniforms.image_size = glGetUniformLocation(program, "u_image_size");
    uniforms.image_mode = glGetUniformLocation(program, "u_image_mode");
    uniforms.uv_offset = glGetUniformLocation(program, "u_uv_offset");
    uniforms.uv_scale = glGetUniformLocation(program, "u_uv_scale");
}

void _load_quad(void) {
    static const float data[] = {
        // First triangle (positions + uv).
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        // Second triangle (positions + uv).
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof (data), data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void *) (2 * sizeof (float)));
    glEnableVertexAttribArray(1);
}

void _unload_quad(void) {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void _unload_shader(void) {
    glDeleteProgram(program);
}

void ngl_init(void) {
    _load_quad();
    _load_shader();
}

void ngl_fini(void) {
    _unload_quad();
    _unload_shader();
}

struct nui_image *ngl_load_image_from_file(const char *filename) {
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

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(pixels);

    image->handle = (void *)(uintptr_t)texture;
    image->width = width;
    image->height = height;

    return image;
}

struct nui_image *ngl_create_image_rgba(int width, int height, const unsigned char *pixels) {
    if (width <= 0 || height <= 0 || !pixels) {
        return NULL;
    }

    struct nui_image *image = malloc(sizeof *image);
    if (!image) {
        return NULL;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    image->handle = (void *)(uintptr_t)texture;
    image->width = width;
    image->height = height;
    return image;
}

bool ngl_update_image_rgba(struct nui_image *image, const unsigned char *pixels) {
    if (!image || !pixels) {
        return false;
    }

    GLuint texture = (GLuint)(uintptr_t) image->handle;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return true;
}

void ngl_unload_image(struct nui_image *image) {
    GLuint id = (GLuint)(uintptr_t) image->handle;
    glDeleteTextures(1, &id);
    free(image);
}

struct nui_font *ngl_load_font_from_file(const char *filename, float font_size) {
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

    GLuint font_texture;
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, pixels_width, pixels_height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLint swizzle[] = { GL_ZERO, GL_ZERO, GL_ZERO, GL_RED };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);

    free(pixels);

    struct ngl_font *ngl_font = malloc(sizeof *ngl_font);
    if (!ngl_font) {
        glDeleteTextures(1, &font_texture);
        free(font_buffer);
        free(baked);
        return NULL;
    }

    ngl_font->texture = font_texture;
    ngl_font->font_buffer = font_buffer;
    ngl_font->info = info;
    ngl_font->baked = baked;
    ngl_font->first_char = first_char;
    ngl_font->num_chars = num_chars;
    ngl_font->pixels_width = pixels_width;
    ngl_font->pixels_height = pixels_height;
    ngl_font->scale = scale;
    ngl_font->ascent = ascent_int * scale;
    ngl_font->line_height = (ascent_int - descent_int + line_gap) * scale;

    struct nui_font *font = malloc(sizeof *font);
    if (!font) {
        glDeleteTextures(1, &font_texture);
        free(font_buffer);
        free(baked);
        free(ngl_font);
        return NULL;
    }

    font->handle = ngl_font;

    return font;
}

void ngl_unload_font(struct nui_font *font) {
    struct ngl_font *ngl_font = font->handle;
    glDeleteTextures(1, &ngl_font->texture);
    free(ngl_font->font_buffer);
    free(ngl_font->baked);
    free(ngl_font);
    free(font);
}

static bool _ngl_font_has_codepoint(const struct ngl_font *font, unsigned char ch) {
    return ch >= font->first_char && (size_t)(ch - font->first_char) < font->num_chars;
}

static float _ngl_font_kerning(const struct ngl_font *font, unsigned char ch, unsigned char next) {
    if (!_ngl_font_has_codepoint(font, next)) {
        return 0;
    }

    return font->scale * stbtt_GetCodepointKernAdvance(&font->info, ch, next);
}

void ngl_before_render(int width, int height) {
    glViewport(0, 0, width, height);
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniform2f(uniforms.viewport, (float) width, (float) height);

    glGetBooleanv(GL_BLEND, &previous.blend);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previous.blend_src);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previous.blend_dst);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ngl_after_render(void) {
    if (!previous.blend) {
        glDisable(GL_BLEND);
    }

    glBlendFunc(previous.blend_src, previous.blend_dst);
}

void _color_to_floats(uint32_t color, float *r, float *g, float *b, float *a) {
    *r = ((color >> 24) & 0xFF) / 255.0f;
    *g = ((color >> 16) & 0xFF) / 255.0f;
    *b = ((color >> 8) & 0xFF) / 255.0f;
    *a = (color & 0xFF) / 255.0f;
}

void ngl_draw_box(int x, int y, int w, int h, uint32_t fill, uint32_t border, int border_width, int radius) {
    float fr, fg, fb, fa;
    float rr, rg, rb, ra;
    _color_to_floats(fill, &fr, &fg, &fb, &fa);
    _color_to_floats(border, &rr, &rg, &rb, &ra);

    glUniform4f(uniforms.color, fr, fg, fb, fa);
    glUniform4f(uniforms.border_color, rr, rg, rb, ra);
    glUniform1f(uniforms.border_width, (float) border_width);
    glUniform1f(uniforms.radius, (float) radius);
    glUniform2f(uniforms.position, (float) x, (float) y);
    glUniform2f(uniforms.size, (float) w, (float) h);
    glUniform1i(uniforms.mode, MODE_NORMAL);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ngl_draw_rect(int x, int y, int w, int h, uint32_t color) {
    ngl_draw_box(x, y, w, h, color, 0x00000000, 0, 0);
}

void ngl_draw_image(int x, int y, int w, int h, const struct nui_image *image, enum nui_image_mode mode) {
    GLuint texture = (GLuint)(uintptr_t) image->handle;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode == NUI_IMAGE_MODE_STRETCH ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode == NUI_IMAGE_MODE_STRETCH ? GL_CLAMP_TO_EDGE : GL_REPEAT);

    glUniform2f(uniforms.position, (float) x, (float) y);
    glUniform2f(uniforms.size, (float) w, (float) h);
    glUniform1i(uniforms.texture, 0);
    glUniform2f(uniforms.image_size, (float) image->width, (float) image->height);
    glUniform1i(uniforms.image_mode, mode);
    glUniform1i(uniforms.mode, MODE_IMAGE);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static bool _ngl_measure_text_line_bounds(const struct ngl_font *font, const char *text, size_t len, struct ngl_text_bounds *bounds) {
    float fx = 0;
    float fy = font->ascent;
    bool has_bounds = false;

    for (size_t i = 0; i < len; i++) {
        stbtt_aligned_quad q;
        unsigned char ch = (unsigned char)text[i];

        if (!_ngl_font_has_codepoint(font, ch)) {
            continue;
        }

        bool opengl = true;
        stbtt_GetBakedQuad(font->baked, font->pixels_width, font->pixels_height, ch - font->first_char, &fx, &fy, &q, opengl);
        if (i + 1 < len) {
            fx += _ngl_font_kerning(font, ch, (unsigned char)text[i + 1]);
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

static bool _ngl_measure_text_bounds(const struct ngl_font *font, const char *text, struct ngl_text_bounds *bounds) {
    int max_width = 0;
    int max_height = 0;
    bool has_bounds = false;
    size_t line_index = 0;

    for (const char *line = text; ; line_index++) {
        const char *line_end = line;
        while (*line_end && *line_end != '\n') {
            line_end++;
        }

        struct ngl_text_bounds line_bounds;
        size_t line_len = (size_t)(line_end - line);
        if (_ngl_measure_text_line_bounds(font, line, line_len, &line_bounds)) {
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

void ngl_measure_text(const struct nui_font *font, const char *text, int *width, int *height) {
    *width = 0;
    *height = 0;

    if (!font || !text) {
        return;
    }

    struct ngl_font *ngl_font = font->handle;
    struct ngl_text_bounds bounds;
    if (!_ngl_measure_text_bounds(ngl_font, text, &bounds)) {
        return;
    }

    *width = bounds.max_x - bounds.min_x;
    *height = bounds.max_y - bounds.min_y;
}

static void _ngl_draw_text_line(const struct ngl_font *font, int x, int y, const char *text, size_t len, uint32_t color) {
    struct ngl_text_bounds bounds;
    if (!_ngl_measure_text_line_bounds(font, text, len, &bounds)) {
        return;
    }

    float fx = 0;
    float fy = font->ascent;

    for (size_t i = 0; i < len; i++) {
        stbtt_aligned_quad q;
        unsigned char ch = (unsigned char)text[i];

        if (!_ngl_font_has_codepoint(font, ch)) {
            continue;
        }

        bool opengl = true;
        stbtt_GetBakedQuad(font->baked, font->pixels_width, font->pixels_height, ch - font->first_char, &fx, &fy, &q, opengl);
        if (i + 1 < len) {
            fx += _ngl_font_kerning(font, ch, (unsigned char)text[i + 1]);
        }

        glUniform2f(uniforms.uv_offset, q.s0, q.t0);
        glUniform2f(uniforms.uv_scale, q.s1 - q.s0, q.t1 - q.t0);

        float r, g, b, a;
        _color_to_floats(color, &r, &g, &b, &a);
        glUniform4f(uniforms.color, r, g, b, a);

        glUniform2f(uniforms.position, (float)x + q.x0 - bounds.min_x, (float)y + q.y0 - bounds.min_y);
        glUniform2f(uniforms.size, q.x1 - q.x0, q.y1 - q.y0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

    }
}

void ngl_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color) {
    struct ngl_font *ngl_font = font->handle;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ngl_font->texture);
    glUniform1i(uniforms.texture, 0);
    glUniform1i(uniforms.mode, MODE_TEXT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    size_t line_index = 0;
    for (const char *line = text; ; line_index++) {
        const char *line_end = line;
        while (*line_end && *line_end != '\n') {
            line_end++;
        }

        int line_y = y + (int)(line_index * ngl_font->line_height);
        _ngl_draw_text_line(ngl_font, x, line_y, line, (size_t)(line_end - line), color);

        if (!*line_end) {
            break;
        }

        line = line_end + 1;
    }
}