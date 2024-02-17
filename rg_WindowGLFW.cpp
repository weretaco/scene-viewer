#include "rg_WindowGLFW.h"

void rg_WindowGLFW::init() {
    glfwInit();
}

void rg_WindowGLFW::cleanup() {
    glfwTerminate();
}

rg_WindowGLFW::rg_WindowGLFW(int width, int height, std::string title)
: rg_Window(width, height, title) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

const char* const* rg_WindowGLFW::getRequiredExtensions(uint32_t *count) {
    return glfwGetRequiredInstanceExtensions(count);
}

void rg_WindowGLFW::pollEvents() {
    glfwPollEvents();
}

void rg_WindowGLFW::waitEvents() {
    glfwWaitEvents();
}

bool rg_WindowGLFW::windowShouldClose() {
    return glfwWindowShouldClose(window);
}

void rg_WindowGLFW::getFramebufferSize(int* width, int* height) {
    glfwGetFramebufferSize(window, width, height);
}

void* rg_WindowGLFW::getNativeWindowHandle() {
    return (void*)window;
}

void rg_WindowGLFW::destroyNativeWindow() {
    glfwDestroyWindow(window);
}

VkResult rg_WindowGLFW::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
    return glfwCreateWindowSurface(instance, window, nullptr, surface);
}