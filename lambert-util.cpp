#include <cstdio>
#include <cstdlib>

#include <array>
#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

struct CLIArguments {
    std::string srcCubemap = "";
    std::string dstCubemap = "";
};

class LambertUtil {
public:
    void run(int argc, char* argv[]) {
        processCLIArgs(argc, argv);

        loadCubemap();
        processCubemap();
        saveCubemap();
    }

private:
    CLIArguments args;
    unsigned char* img;
    int width, height, channels;

    void processCLIArgs(int argc, char* argv[]) {
        args = {};
        args.srcCubemap = "";
        args.dstCubemap = "";

        std::cout << "CLI ARGS:" << std::endl;
        for (int i=0; i<argc; i++) {
            std::string arg(argv[i]);

            if (arg.rfind("--", 0) == 0) {
                if (arg == "--src") {
                    handleArgSrc(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--dst") {
                    handleArgDst(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else{
                    std::cout << std::endl << "Unknown command-line argument: " << arg << std::endl;
                }
            }
        }
    }

    void handleArgSrc(const std::array<std::string, 2> &arr) {
        args.srcCubemap = arr[1];
    }

    void handleArgDst(const std::array<std::string, 2> &arr) {
        args.dstCubemap = arr[1];
    }

    void loadCubemap() {
        std::cout << "Loading cubemap from " << args.srcCubemap << std::endl;

        img = stbi_load(args.srcCubemap.c_str(), &width, &height, &channels, 0);
        if (img == nullptr) {
            throw std::runtime_error("Error loading the image: " + args.srcCubemap);
        }
        std::cout << "Loaded image with a width of " << width << "px, a height of " << height << "px and " << channels << " channels" << std::endl;
    }

    void processCubemap() {
        std::cout << "Processing cubemap" << std::endl;

        size_t img_size = width * height * channels;

        for (unsigned char *p = img; p != img + img_size; p += channels) {
            uint8_t c = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
            *p = c;
            *(p + 1) = c;
            *(p + 2) = c;
        }
    }

    void saveCubemap() {
        std::cout << "Saving cubemap to " << args.dstCubemap << std::endl;

        stbi_write_png(args.dstCubemap.c_str(), width, height, channels, img, width * channels);

        stbi_image_free(img);
    }
};

int main(int argc, char* argv[]) {
    std::cout << std::boolalpha;

    LambertUtil app;

    try {
        app.run(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
