#pragma once

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class window {
    private:
        GLFWwindow *_window = nullptr;

    public:
        void create(const char *title, int width, int height);
        void cleanup();

        void swap_buffers();
        void poll_events();

        bool should_close() const;

        GLFWwindow *get_window() const { return _window; }
};
