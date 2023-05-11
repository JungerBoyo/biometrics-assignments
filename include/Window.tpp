#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

template<>
void bm::Window::setKeyCallback
(void(*key_callback)(GLFWwindow*, int, int, int, int));

template<>
void bm::Window::setMousePositionCallback
(void(*mouse_position_callback)(GLFWwindow*, double, double));

template<>
void bm::Window::setMouseButtonCallback
(void(*mouse_button_callback)(GLFWwindow*, int, int, int));

template<>
void bm::Window::setMouseScrollCallback
(void(*mouse_scroll_callback)(GLFWwindow*, double, double));