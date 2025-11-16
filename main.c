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

int main(int argc, char **argv) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
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

    _centerize(window);
    glfwShowWindow(window);

    struct nui_context ctx;
    nui_context_init(&ctx);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // nui_begin_frame(&ctx);
        //// Example NUI usage
        // nui_window_begin(&ctx, "Hello, NUI!", 50, 50, 300, 200);
        // nui_text(&ctx, "This is a simple NUI example.");
        // nui_button(&ctx, "Click Me");
        // nui_window_end(&ctx);
        // nui_end_frame(&ctx);

        glfwSwapBuffers(window);
    }

    nui_context_fini(&ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
