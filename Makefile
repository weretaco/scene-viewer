STB_INCLUDE_PATH = /home/dportnoy/Dev/realtime-rendering/stb

CFLAGS = -std=c++17 -O2 -I$(STB_INCLUDE_PATH)
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SceneViewer: sceneviewer.cpp jsonloader.cpp rg_Window.h rg_WindowGLFW.h rg_WindowGLFW.cpp rg_WindowNativeLinux.h rg_WindowNativeLinux.cpp rg_WindowManager.h
	rm -f SceneViewer
	g++ $(CFLAGS) -o SceneViewer sceneviewer.cpp jsonloader.cpp rg_WindowGLFW.cpp rg_WindowNativeLinux.cpp $(LDFLAGS)

.PHONY: shaders clean

shaders:
	bash compile.sh

clean:
	rm -f SceneViewer
