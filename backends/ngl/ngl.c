#include "glad/glad.h"
#include <stdio.h>

static GLuint program;
static GLuint vao, vbo;
static struct {
    int width, height;
} viewport;
static struct {
    GLint position, size, color;
    GLint viewport;
} uniforms;

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
        "    vec2 pos = u_position + a_position * u_size;\n"
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

void ngl_before_render(int width, int height) {
    glViewport(0, 0, width, height);
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniform2f(uniforms.viewport, (float) width, (float) height);
}

void ngl_draw_rect(int x, int y, int w, int h, uint32_t color) {
    float r = ((color >> 24) & 0xFF) / 255.0f;
    float g = ((color >> 16) & 0xFF) / 255.0f;
    float b = ((color >> 8) & 0xFF) / 255.0f;
    float a = (color & 0xFF) / 255.0f;

    glUniform4f(uniforms.color, r, g, b, a);
    glUniform2f(uniforms.position, (float) x, (float) y);
    glUniform2f(uniforms.size, (float) w, (float) h);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
