CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SceneViewer: main.cpp
	rm -f SceneViewer
	g++ $(CFLAGS) -o SceneViewer main.cpp $(LDFLAGS)

.PHONY: test shaders clean

test: SceneViewer
	./SceneViewer

clean:
	rm -f SceneViewer
