#include "window.h"

void window::create(const char *title, int width, int height)
{
    if (!glfwInit()) { std::cout << "failed to initialize glfw\n"; }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    _window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!_window) { glfwTerminate(); std::cout << "window creation failed\n"; }

    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    const GLenum err = glewInit();
    if (err != GLEW_OK) { std::cout << "glew init failed\n"; }

    int _framebuffer_width = 0, _framebuffer_height = 0;
    glfwGetFramebufferSize(_window, &_framebuffer_width, &_framebuffer_height);
    glViewport(0, 0, _framebuffer_width, _framebuffer_height);
}

void window::cleanup() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

void window::swap_buffers() {
    glfwSwapBuffers(_window);
}

void window::poll_events() {
    glfwPollEvents();
}

bool window::should_close() const {
    return _window ? glfwWindowShouldClose(_window) : true;
}
