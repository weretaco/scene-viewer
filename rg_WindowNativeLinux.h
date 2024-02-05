#ifndef _RG_WINDOW_NATIVE_LINUX_H
#define _RG_WINDOW_NATIVE_LINUX_H

#include <vector>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include "rg_Window.h"

class rg_WindowNativeLinux : public rg_Window {
    public:

        static void init();
        static void cleanup();

        static const char* const* getRequiredExtensions(uint32_t *count);

        rg_WindowNativeLinux(int width, int height, std::string title);

        void pollEvents() override;
        void waitEvents() override;
        bool windowShouldClose() override;
        void getFramebufferSize(int* width, int* height) override;
        void* getNativeWindowHandle() override;
        void destroyNativeWindow() override;
        VkResult createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) override;

    private:
        Window window;
        bool _windowShouldClose;

        static Display* dpy;
        static Atom wmDeleteMessage;

        static const std::vector<const char*> requiredExtensions;
};

#endif // _RG_WINDOW_NATIVE_LINUX_H