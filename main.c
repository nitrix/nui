#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "nui.h"
#include <stdio.h>
#include <stdlib.h>

static void _centerize(GLFWwindow *window) {
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        if (mode) {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            int xpos = (mode->width - width) / 2;
            int ypos = (mode->height - height) / 2;
            glfwSetWindowPos(window, xpos, ypos);
        }
    }
}

static void _on_framebuffer_size(GLFWwindow *window, int fb_width, int fb_height) {
    (void) window;
    nui_viewport(fb_width, fb_height);
}

static void _debug(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *param) {
    (void) length;
    (void) param;

    // Ignore notifications about buffers hints (e.g. GL_STREAM_DRAW) that wont be honored
    // and instead will use a different kind of memory as source for buffer object operations.
    if (id == 131185) return;

    // Ignore notifications about program/shader performance due to recompilation.
    if (id == 131218) return;

    // Ignore notifications about multisample storage allocatinos (the renderer does it on resizes by the ui).
    if (id == 131169) return;

    fprintf(stderr, "[OpenGL debug] source: %d, type: %d, id: %d, severity: %d, message: %s\n", source, type, id, severity, message);
}

int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "NUI Example", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        fprintf(stderr, "Failed to initialize OpenGL\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    {
        int context_flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
        bool has_debug_bit = context_flags & GL_CONTEXT_FLAG_DEBUG_BIT;

        if (has_debug_bit) {
            if (GLAD_GL_KHR_debug) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(_debug, NULL);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
            } else if (GLAD_GL_ARB_debug_output) {
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
                glDebugMessageCallbackARB(_debug, NULL);
                glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
            }
        }
    }

    _centerize(window);
    glfwShowWindow(window);

    glfwSetFramebufferSizeCallback(window, _on_framebuffer_size);

    nui_init();

    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    nui_viewport(fb_width, fb_height);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        nui_frame();

        NUI {
            nui_fixed(200, 200);
            nui_background_color(0xFF0000FF);
            nui_padding(10, 10, 10, 10);

            NUI {
                nui_fixed(100, 100);
                nui_background_color(0x00FF00FF);
            }
        }

        nui_update();
        nui_render();

        //// Example NUI usage
        // nui_window_begin(&ctx, "Hello, NUI!", 50, 50, 300, 200);
        // nui_text(&ctx, "This is a simple NUI example.");
        // nui_button(&ctx, "Click Me");
        // nui_window_end(&ctx);
        // nui_end_frame(&ctx);

        glfwSwapBuffers(window);
    }

    nui_fini();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
