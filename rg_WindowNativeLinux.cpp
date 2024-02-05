#include "rg_WindowNativeLinux.h"

#include <cassert>
#include <iostream>
#include <string>

Display* rg_WindowNativeLinux::dpy = nullptr;
Atom rg_WindowNativeLinux::wmDeleteMessage = 0;

const std::vector<const char*> rg_WindowNativeLinux::requiredExtensions = {
    "VK_KHR_surface",
    "VK_KHR_xlib_surface"
    //"VK_KHR_xcb_surface"
};

void rg_WindowNativeLinux::init() {
    XInitThreads();

    dpy = XOpenDisplay(NULL);
    assert(dpy);

    wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
}

void rg_WindowNativeLinux::cleanup() {
}

rg_WindowNativeLinux::rg_WindowNativeLinux(int width, int height, std::string title)
: rg_Window(width, height, title) {
    int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
    int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

    window = XCreateSimpleWindow(
                dpy,
                DefaultRootWindow(dpy),
                0,
                0,
                width,
                height,
                5,
                blackColor,
                blackColor);
    _windowShouldClose = false;

    XSelectInput(dpy, window,
        StructureNotifyMask | ExposureMask);
    XMapWindow(dpy, window);
    XSetWMProtocols(dpy, window, &rg_WindowNativeLinux::wmDeleteMessage, 1);

    // Wait for the MapNotify event

    std::cout << "Entering native linux event loop" << std::endl;
    for(;;) {
        XEvent e;
        XNextEvent(dpy, &e);
        std::cout << "event type: " << e.type << std::endl;

        if (e.type == MapNotify) {
            std::cout << "MapNotify" << std::endl;
            break;
        }
    }

    XFlush(dpy);
}

const char* const* rg_WindowNativeLinux::getRequiredExtensions(uint32_t *count) {
    *count = requiredExtensions.size();

    return requiredExtensions.data();
}

void rg_WindowNativeLinux::pollEvents() {
    // Process all pending events
    while (XPending(dpy) > 0) {
        waitEvents();
    }
}

void rg_WindowNativeLinux::waitEvents() {
    XEvent e;
    XNextEvent(dpy, &e);

    if (e.type == ClientMessage && e.xclient.data.l[0] == rg_WindowNativeLinux::wmDeleteMessage) {
        XDestroyWindow(dpy,e.xclient.window);
        XFlush(dpy);

        _windowShouldClose = true;
    }
}

bool rg_WindowNativeLinux::windowShouldClose() {
    return _windowShouldClose;
}

void rg_WindowNativeLinux::getFramebufferSize(int* width, int* height) {
    int x, y;
    unsigned int uwidth, uheight, borderWidth, depth;

    XGetGeometry(dpy, window, &DefaultRootWindow(dpy), &x, &y, &uwidth, 
                &uheight, &borderWidth, &depth);

    *width = static_cast<int>(uwidth);
    *height = static_cast<int>(uheight);
}

void* rg_WindowNativeLinux::getNativeWindowHandle() {
    return (void*)window;
}

void rg_WindowNativeLinux::destroyNativeWindow() {
}

VkResult rg_WindowNativeLinux::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
    VkXlibSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = dpy;
    createInfo.window = window;

    return vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, surface);
}