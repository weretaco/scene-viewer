INCLUDE_PATH = includes

CFLAGS = -std=c++17 -O2 -I$(INCLUDE_PATH)
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SceneViewer: sceneviewer.cpp jsonloader.h jsonloader.cpp eventloader.h eventloader.cpp graphics-pipeline.h OrbitCamera.h OrbitCamera.cpp attribute.h vertexcolor.h vertextexture.h types.h utils.h rg_Window.h rg_WindowGLFW.h rg_WindowGLFW.cpp rg_WindowNativeLinux.h rg_WindowNativeLinux.cpp rg_WindowManager.h
	rm -f SceneViewer
	g++ $(CFLAGS) -o SceneViewer sceneviewer.cpp jsonloader.cpp eventloader.cpp OrbitCamera.cpp rg_WindowGLFW.cpp rg_WindowNativeLinux.cpp $(LDFLAGS)

LambertUtil: lambert-util.cpp
	rm -f LambertUtil
	g++ $(CFLAGS) -o LambertUtil lambert-util.cpp $(LDFLAGS)

.PHONY: shaders clean

shaders:
	bash compile.sh

clean:
	rm -f SceneViewer

# Useful so I don't forget all the CLI settings
run:
	./SceneViewer --scene scenes/env-cube.s72 --physical-device "AMD Radeon RX 6950 XT (RADV NAVI21)" --drawing-size 1600 1200 --culling none
