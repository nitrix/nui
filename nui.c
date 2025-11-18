#include "glad/glad.h"
#include "nui.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
    struct {
        char *data;
        size_t used;
        size_t capacity;
        void *last_ptr;
        size_t last_size;
    } arena;
    struct {
        void *(*malloc)(size_t size);
        void (*free)(void *ptr);
    } memory;
    struct {
        GLuint shader;
        GLuint vao, vbo;
        struct {
            GLint position, size, color;
            GLint viewport;
        } uniforms;
    } renderer;
    struct nui_element root;
    struct nui_element *current;
} ctx;

void *_arena_realloc(void *old, size_t size) {
    // Round up to nearest multiple of the largest alignment requirement.
    size_t alignment = sizeof (max_align_t);
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    // Brand new allocation.
    if (old == NULL) {
        if (aligned_size > ctx.arena.capacity) {
            size_t new_capacity = ctx.arena.capacity == 0 ? 1024 : ctx.arena.capacity;
            while (new_capacity < aligned_size) {
                new_capacity *= 2;
            }
            char *new_data = ctx.memory.malloc(new_capacity);
            if (ctx.arena.data) {
                memcpy(new_data, ctx.arena.data, ctx.arena.used);
                ctx.memory.free(ctx.arena.data);
            }
            ctx.arena.data = new_data;
            ctx.arena.capacity = new_capacity;
        }

        void *new = ctx.arena.data + ctx.arena.used;

        ctx.arena.used += aligned_size;
        ctx.arena.last_ptr = new;
        ctx.arena.last_size = size;

        return new;
    }

    // Reallocation of the most recent allocation.
    if (old == ctx.arena.last_ptr) {
        // Pretend the old allocation is gone.
        ctx.arena.used = (char *) old - ctx.arena.data;

        // Keep the old data for the size that we do share in common.
        size_t copy_size = size < ctx.arena.last_size ? size : ctx.arena.last_size;
        ctx.arena.used += copy_size;

        // Allocate for the remaining size.
        size_t remaining_size = size > copy_size ? size - copy_size : 0;
        if (remaining_size) {
            _arena_realloc(NULL, remaining_size);
        }

        ctx.arena.last_ptr = old;
        ctx.arena.last_size = size;

        return old;
    }

    // Reallocation of an older allocation: abandon it and make a new one.
    return _arena_realloc(NULL, size);
}

void _load_shader(void) {
    char *vertex_shader_src =
        "#version 410 core\n"
        "in vec2 a_position;\n"
        "in vec2 a_uv;\n"
        "uniform vec2 u_position;\n"
        "uniform vec2 u_size;\n"
        "uniform vec2 u_viewport;\n"
        "out vec2 v_uv;\n"
        "void main() {\n"
        "    vec2 pos = a_position * u_size;\n"
        "    float ndc_x = (pos.x / u_viewport.x) * 2 - 1;\n"
        "    float ndc_y = (pos.y / u_viewport.y) * -2 + 1;\n"
        "    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);\n"
        "    v_uv = a_uv;\n"
        "}\n";
    char *fragment_shader_src =
        "#version 410 core\n"
        "in vec2 v_uv;\n"
        "uniform vec4 u_color;\n"
        "out vec4 frag_color;\n"
        "void main() {\n"
        "    frag_color = u_color;\n"
        "}\n";

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

    GLuint program = glCreateProgram();
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

    ctx.renderer.shader = program;
    ctx.renderer.uniforms.position = glGetUniformLocation(program, "u_position");
    ctx.renderer.uniforms.size = glGetUniformLocation(program, "u_size");
    ctx.renderer.uniforms.color = glGetUniformLocation(program, "u_color");
    ctx.renderer.uniforms.viewport = glGetUniformLocation(program, "u_viewport");
}

void _prepare_quad(void) {
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

    glGenVertexArrays(1, &ctx.renderer.vao);
    glBindVertexArray(ctx.renderer.vao);

    glGenBuffers(1, &ctx.renderer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.renderer.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof (data), data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void *) (2 * sizeof (float)));
    glEnableVertexAttribArray(1);
}

void _unload_shader(void) {
    glDeleteProgram(ctx.renderer.shader);
}

void nui_init(void) {
    ctx.arena.data = NULL;
    ctx.arena.used = 0;
    ctx.arena.capacity = 0;
    ctx.memory.malloc = malloc;
    ctx.memory.free = free;
    ctx.root.parent = NULL;
    ctx.root.children = NULL;
    ctx.root.children_count = 0;
    _load_shader();
    _prepare_quad();
}

void nui_fini(void) {
    ctx.memory.free(ctx.arena.data);
    _unload_shader();
}

void nui_frame(void) {
    ctx.arena.used = 0;
    ctx.root.children = NULL;
    ctx.root.children_count = 0;
}

void nui_viewport(int width, int height) {
    ctx.root.w = width;
    ctx.root.h = height;
}

void nui_element_begin(struct nui_element *el) {
    assert(el);

    el->parent = ctx.current;
    ctx.current = el;

    struct nui_element *target = (el->parent ? el->parent : &ctx.root);

    target->children = _arena_realloc(target->children, sizeof target->children_count + 1);
    target->children[target->children_count] = el;
    target->children_count++;
}

void nui_element_end(void) {
    assert(ctx.current);

    ctx.current = ctx.current->parent;
}

void nui_fixed_width(int width) {
    ctx.current->w = width;
}

void nui_fixed_height(int height) {
    ctx.current->h = height;
}

void nui_fixed(int width, int height) {
    nui_fixed_width(width);
    nui_fixed_height(height);
}

void nui_background_color(uint32_t color) {
    ctx.current->style.background_color = color;
    ctx.current->style.flags |= NUI_STYLE_FLAG_BACKGROUND_COLOR;
}

void nui_update(void) {
}

void _nui_render_element(const struct nui_element *el) {
    if (el->style.flags & NUI_STYLE_FLAG_BACKGROUND_COLOR) {
        uint32_t color = el->style.background_color;
        float r = ((color >> 24) & 0xFF) / 255.0f;
        float g = ((color >> 16) & 0xFF) / 255.0f;
        float b = ((color >> 8) & 0xFF) / 255.0f;
        float a = (color & 0xFF) / 255.0f;

        glUniform4f(ctx.renderer.uniforms.color, r, g, b, a);
    } else {
        glUniform4f(ctx.renderer.uniforms.color, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    glUniform2f(ctx.renderer.uniforms.position, (float) el->x, (float) el->y);
    glUniform2f(ctx.renderer.uniforms.size, (float) el->w, (float) el->h);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    for (size_t i = 0; i < el->children_count; i++) {
        _nui_render_element(el->children[i]);
    }
}

void nui_render(void) {
    glViewport(0, 0, ctx.root.w, ctx.root.h);
    glUseProgram(ctx.renderer.shader);
    glBindVertexArray(ctx.renderer.vao);
    glUniform2f(ctx.renderer.uniforms.viewport, (float) ctx.root.w, (float) ctx.root.h);

    _nui_render_element(&ctx.root);
}
