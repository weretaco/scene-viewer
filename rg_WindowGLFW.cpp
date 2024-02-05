#include "rg_WindowGLFW.h"

const char* const* rg_WindowGLFW::getRequiredExtensions(uint32_t *count) {
    return glfwGetRequiredInstanceExtensions(count);
}

void rg_WindowGLFW::init() {
    glfwInit();
}

void rg_WindowGLFW::cleanup() {
    glfwTerminate();
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