#ifndef _RG_WINDOW_H
#define _RG_WINDOW_H

#include <string>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

class rg_Window {
    public:

        static void init();
        static void cleanup();

        static const char* const* getRequiredExtensions(uint32_t *count) {
            return nullptr;
        }

        rg_Window(int width, int height, std::string title) {
        }

        virtual void pollEvents() = 0;
        virtual void waitEvents() = 0;
        virtual bool windowShouldClose() = 0;
        virtual void getFramebufferSize(int* width, int* height) = 0;

        virtual void* getNativeWindowHandle() = 0;
        virtual void destroyNativeWindow() = 0;

        virtual VkResult createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) = 0;

    private:
};

#endif // _RG_WINDOW_H