#ifndef _RG_WINDOW_GLFW_H
#define _RG_WINDOW_GLFW_H

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "rg_Window.h"

class rg_WindowGLFW : public rg_Window {
    public:

        static void init();
        static void cleanup();

        static const char* const* getRequiredExtensions(uint32_t *count);

        rg_WindowGLFW(int width, int height, std::string title);

        void pollEvents() override;
        void waitEvents() override;
        bool windowShouldClose() override;
        void getFramebufferSize(int* width, int* height) override;
        void* getNativeWindowHandle() override;
        void destroyNativeWindow() override;
        VkResult createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) override;

    private:
        GLFWwindow* window;
};

#endif // _RG_WINDOW_GLFW_H