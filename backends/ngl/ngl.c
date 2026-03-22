#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "glad/gl.h"
#include "nui.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include <stdio.h>

static GLuint program;
static GLuint vao, vbo;
static struct {
    int width, height;
} viewport;
static struct {
    GLint position, size, color;
    GLint viewport;
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
    stbtt_bakedchar *baked;
    size_t num_chars;
    size_t pixels_width;
    size_t pixels_height;
    float ascent;
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
        uniform vec2 u_uv_offset;
        uniform vec2 u_uv_scale;

        void main() {
            vec2 pos = u_position + a_position * u_size;
            float ndc_x = (pos.x / u_viewport.x) * 2 - 1;
            float ndc_y = (pos.y / u_viewport.y) * -2 + 1;

            gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);

            if (u_mode == 1) {
                v_uv = a_uv * (u_size / u_image_size);
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
        uniform sampler2D u_texture;

        void main() {
            if (u_mode == 1) {
                frag_color = texture(u_texture, v_uv);
            } else if (u_mode == 2) {
                frag_color = u_color;
                frag_color.a = texture(u_texture, v_uv).a;
            } else {
                frag_color = u_color;
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
    uniforms.viewport = glGetUniformLocation(program, "u_viewport");
    uniforms.texture = glGetUniformLocation(program, "u_texture");
    uniforms.mode = glGetUniformLocation(program, "u_mode");
    uniforms.image_size = glGetUniformLocation(program, "u_image_size");
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
        return NULL;
    }

    ngl_font->texture = font_texture;
    ngl_font->baked = baked;
    ngl_font->num_chars = num_chars;
    ngl_font->pixels_width = pixels_width;
    ngl_font->pixels_height = pixels_height;
    ngl_font->ascent = ascent_int * scale;

    struct nui_font *font = malloc(sizeof *font);
    if (!font) {
        glDeleteTextures(1, &font_texture);
        free(ngl_font);
        return NULL;
    }

    font->handle = ngl_font;

    return font;
}

void ngl_unload_font(struct nui_font *font) {
    struct ngl_font *ngl_font = font->handle;
    glDeleteTextures(1, &ngl_font->texture);
    free(ngl_font);
    free(font);
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

void ngl_draw_rect(int x, int y, int w, int h, uint32_t color) {
    float r, g, b, a;
    _color_to_floats(color, &r, &g, &b, &a);

    glUniform4f(uniforms.color, r, g, b, a);
    glUniform2f(uniforms.position, (float) x, (float) y);
    glUniform2f(uniforms.size, (float) w, (float) h);
    glUniform1i(uniforms.mode, MODE_NORMAL);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ngl_draw_image(int x, int y, int w, int h, struct nui_image *image) {
    GLuint texture = (GLuint)(uintptr_t) image->handle;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform2f(uniforms.position, (float) x, (float) y);
    glUniform2f(uniforms.size, (float) w, (float) h);
    glUniform1i(uniforms.texture, 0);
    glUniform2f(uniforms.image_size, (float) image->width, (float) image->height);
    glUniform1i(uniforms.mode, MODE_IMAGE);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ngl_draw_text(const struct nui_font *font, int x, int y, const char *text, uint32_t color) {
    struct ngl_font *ngl_font = font->handle;


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ngl_font->texture);
    glUniform1i(uniforms.texture, 0);
    glUniform1i(uniforms.mode, MODE_TEXT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float fx = 0;
    float fy = ngl_font->ascent;

    while (*text) {
        stbtt_aligned_quad q;

        bool opengl = true;
        stbtt_GetBakedQuad(ngl_font->baked, ngl_font->pixels_width, ngl_font->pixels_height, *text-32, &fx, &fy, &q, opengl);

        glUniform2f(uniforms.uv_offset, q.s0, q.t0);
        glUniform2f(uniforms.uv_scale, q.s1 - q.s0, q.t1 - q.t0);

        float r, g, b, a;
        _color_to_floats(color, &r, &g, &b, &a);
        glUniform4f(uniforms.color, r, g, b, a);

        glUniform2f(uniforms.position, (float)x + q.x0, (float)y + q.y0);
        glUniform2f(uniforms.size, q.x1 - q.x0, q.y1 - q.y0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        text++;
    }
}