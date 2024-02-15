#include <array>
#include <iostream>
#include <string>

// ./Launcher --scene scene.s72 --camera name --drawing-size w h --culling none|frustum|... --headless events

class Launcher {
public:
    void processCLIArgs(int argc, char* argv[]) {
        std::cout << "CLI ARGS:" << std::endl;
        for (int i=0; i<argc; i++) {
            std::string arg(argv[i]);

            if (arg.rfind("--", 0) == 0) {
                if (arg == "--scene") {
                    handleArgScene(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--camera") {
                    handleArgCamera(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--drawing-size") {
                    handleArgDrawingSize(std::array<std::string, 3>{ argv[i], argv[i+1], argv[i+2] });
                } else if (arg == "--culling") {
                    handleArgCulling(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--headless") {
                    handleArgHeadless(std::array<std::string, 2>{ argv[i], argv[i+1] });
                }
            }
        }
    }

private:
    void handleArgScene(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        std::cout << "scene: " << arr[1] << std::endl;
    }

    void handleArgCamera(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        std::cout << "name: " << arr[1] << std::endl;
    }

    void handleArgDrawingSize(const std::array<std::string, 3> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        std::cout << "w: " << arr[1] << std::endl;
        std::cout << "h: " << arr[2] << std::endl;
    }

    void handleArgCulling(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        std::cout << "culling mode: " << arr[1] << std::endl;
    }

    void handleArgHeadless(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        std::cout << "event file: " << arr[1] << std::endl;
    }
};

int main(int argc, char* argv[]) {
    Launcher launcher;

    launcher.processCLIArgs(argc, argv);

    return EXIT_SUCCESS;
}