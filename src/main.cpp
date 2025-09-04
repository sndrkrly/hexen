#include <iostream>
#include "gfx/window.h"

int main () {
    window _window;
    _window.create("asd", 800, 600);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!_window.should_close()) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        _window.swap_buffers();
        _window.poll_events();
    }

    _window.cleanup();
    return 0;
}
