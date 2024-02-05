#ifndef _RG_WINDOW_MANAGER_H
#define _RG_WINDOW_MANAGER_H

#include <string>

//#include "rg_WindowGLFW.h"
#include "rg_WindowNativeLinux.h"

#include "rg_Window.h"

class rg_WindowManager {
    public:
        static void init() {

#ifdef _glfw3_h_
            rg_WindowGLFW::init();
#endif

#ifdef _X11_XLIB_H_
            rg_WindowNativeLinux::init();
#endif
        }

        static rg_Window* createWindow(int width, int height, std::string title) {
#ifdef _glfw3_h_
            return new rg_WindowGLFW(width, height, title);
#endif

#ifdef _X11_XLIB_H_
            return new rg_WindowNativeLinux(width, height, title);
#endif
        }

        static void destroyWindow(rg_Window* window) {
            window->destroyNativeWindow();
            delete window;
        }

        static void cleanup() {
#ifdef _glfw3_h_
            rg_WindowGLFW::cleanup();
#endif

#ifdef _X11_XLIB_H_
            rg_WindowNativeLinux::cleanup();
#endif
        }

        static std::vector<const char*> getRequiredExtensions() {
            uint32_t extensionCount = 0;
            const char* const* extensionsArray;

#ifdef _glfw3_h_
            extensionsArray = rg_WindowGLFW::getRequiredExtensions(&extensionCount);
#endif

#ifdef _X11_XLIB_H_
            extensionsArray = rg_WindowNativeLinux::getRequiredExtensions(&extensionCount);
#endif

            std::vector<const char*> extensions(extensionsArray, extensionsArray + extensionCount);
        
            return extensions;
        }

        static VkResult createWindowSurface(VkInstance instance, rg_Window* window, VkSurfaceKHR* surface) {
            return window->createWindowSurface(instance, surface);
        }

    private:
};

#endif // _RG_WINDOW_MANAGER_H