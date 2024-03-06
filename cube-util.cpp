#include <cstdio>
#include <cstdlib>

#include <array>
#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/matrix_inverse.hpp"

struct CLIArguments {
    std::string srcCubemap = "";
    std::string dstCubemap = "";
};

struct PlaneIntersection {
    glm::vec3 p;
    float t;
    bool valid;
};

class CubeUtil {
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
    uint32_t numLayers = 6;
    uint32_t layerSize;
    unsigned char* lucImg;
    int width, height, channels;
    int edgeSamples = 16; // edge samples for the generated cubemap

    std::array<std::array<glm::vec3, 2>, 6> vecNormalUp;

    void processCLIArgs(int argc, char* argv[]) {
        args = {};
        args.srcCubemap = "";
        args.dstCubemap = "";

        if (argc != 4 || std::string(argv[2]) != "--lambertian") {
            throw std::invalid_argument("Expected usaged: cube {inFile} --lambertian {outFile}");
        }

        handleArgSrc(std::array<std::string, 1>{ argv[1] });
        handleArgDst(std::array<std::string, 1>{ argv[3] });
    }

    void handleArgSrc(const std::array<std::string, 1> &arr) {
        args.srcCubemap = arr[0];
    }

    void handleArgDst(const std::array<std::string, 1> &arr) {
        args.dstCubemap = arr[0];
    }

    void loadCubemap() {
        std::cout << "Loading cubemap from " << args.srcCubemap << std::endl;

        img = stbi_load(args.srcCubemap.c_str(), &width, &height, &channels, 0);
        if (img == nullptr) {
            throw std::runtime_error("Error loading the image: " + args.srcCubemap);
        }
        std::cout << "Loaded image with a width of " << width << "px, a height of " << height << "px and " << channels << " channels" << std::endl;

        // allocate memory for the LUC
        lucImg = (stbi_uc*)malloc(edgeSamples * edgeSamples * channels * numLayers);
    }

    void processCubemap() {
        std::cout << "Processing cubemap" << std::endl;

        size_t imageSize = width * height * channels;
        layerSize = static_cast<uint32_t>(imageSize) / numLayers;
        uint32_t lucLayerSize = static_cast<uint32_t>(edgeSamples * edgeSamples * channels);

        // the six images are in this order in the file:
        // right, left, up, down, front, back
        // they should be in this order on the GPU:
        // front, back, up, down, right, left

        std::array<uint32_t, 6> imageOffsets = {
            layerSize * 4,
            layerSize * 5,
            layerSize * 2,
            layerSize * 3,
            layerSize * 0,
            layerSize * 1
        };

        std::array<uint32_t, 6> lucOffsets = {
            lucLayerSize * 4,
            lucLayerSize * 5,
            lucLayerSize * 2,
            lucLayerSize * 3,
            lucLayerSize * 0,
            lucLayerSize * 1
        };

        vecNormalUp = {{
            {
                glm::vec3( 0.0f,  0.0f,  1.0f),
                glm::vec3( 0.0f,  1.0f,  0.0f)
            }, {
                glm::vec3( 0.0f,  0.0f, -1.0f),
                glm::vec3( 0.0f,  1.0f,  0.0f)
            }, {
                glm::vec3( 0.0f,  1.0f,  0.0f),
                glm::vec3( 0.0f,  0.0f, -1.0f)
            }, {
                glm::vec3( 0.0f, -1.0f,  0.0f),
                glm::vec3( 0.0f,  0.0f,  1.0f)
            }, {
                glm::vec3( 1.0f,  0.0f,  0.0f),
                glm::vec3( 0.0f,  1.0f,  0.0f)
            }, {
                glm::vec3(-1.0f,  0.0f,  0.0f),
                glm::vec3( 0.0f,  1.0f,  0.0f)
            }
        }};

        for (uint32_t i = 0; i < numLayers; i++) {
            convolveFace(img + imageOffsets[i], lucImg + lucOffsets[i], vecNormalUp[i][0], vecNormalUp[i][1]);
        }
    }

    void saveCubemap() {
        std::cout << "Saving cubemap to " << args.dstCubemap << std::endl;

        //stbi_write_png(args.dstCubemap.c_str(), width, height, channels, img, width * channels);
        stbi_write_png(args.dstCubemap.c_str(), edgeSamples, edgeSamples * numLayers, channels, lucImg, edgeSamples * channels);

        stbi_image_free(img);
    }

    void convolveFace(unsigned char* imgLayer, unsigned char* lucLayer, const glm::vec3& normal, const glm::vec3& up) {
        glm::vec3 right = glm::normalize(glm::cross(up, normal));

        float faceWidth = 2.f; // -1.0 to 1.0
        float pixelSize = faceWidth / edgeSamples;

        for (int i = 0; i < edgeSamples; i++) {
            for (int j = 0; j < edgeSamples; j++) {
                float upComp = -1.f + (pixelSize / 2) + (i * pixelSize);
                float rightComp = -1.f + (pixelSize / 2) + (j * pixelSize);

                glm::vec3 pos = normal + upComp * (-up) + rightComp * right;

                unsigned char *p = getPixelMem(lucLayer, edgeSamples, channels, j, i);

                glm::vec3 irradiance = getIrradiance(pos);

                *p = (uint8_t)(irradiance.r * 255);
                *(p + 1) = (uint8_t)(irradiance.g * 255);
                *(p + 2) = (uint8_t)(irradiance.b * 255);
                *(p + 3) = 255;
            }
        }
    }

    glm::vec3 getIrradiance(glm::vec3 localPos) {
        const float PI = glm::pi<float>();

        glm::vec3 normal = normalize(localPos);
        glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
        glm::vec3 right = glm::normalize(glm::cross(up, normal));
        up = glm::normalize(glm::cross(normal, right));

        glm::vec3 irradiance = glm::vec3(0.0);

        float sampleDelta = 0.025f;
        float nrSamples = 0.0f;

        for (float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta) {
            for (float theta = 0.0f; theta < 0.5f * PI; theta += sampleDelta) {

                // spherical => cartesian (in tangent space)
                glm::vec3 tangentSample = glm::vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));

                // tangent => world
                // already normalized
                glm::vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

                irradiance += sampleCubemap(sampleVec) * cos(theta) * sin(theta);
                nrSamples++;
            }
        }

        irradiance =  irradiance * (PI / nrSamples);

        return irradiance;
    }

    // get RGB from cubemap (TODO; Convert from rgbe)
    glm::vec3 sampleCubemap(glm::vec3 dir) {
        PlaneIntersection intersect;

        intersect = getXPlaneIntersection(dir, 1.0f);
        if (intersect.valid) {
            glm::ivec2 coords = getPixCoordsFromNormPosition(-intersect.p.z, intersect.p.y, width);
            return getPixelColor(img, width, channels, coords.x, coords.y);
        }

        intersect = getXPlaneIntersection(dir, -1.0f);
        if (intersect.valid) {
            glm::ivec2 coords = getPixCoordsFromNormPosition(intersect.p.z, intersect.p.y, width);
            return getPixelColor(img + layerSize, width, channels, coords.x, coords.y);
        }

        intersect = getYPlaneIntersection(dir, 1.0f);
        if (intersect.valid) {
            glm::ivec2 coords = getPixCoordsFromNormPosition(intersect.p.x, -intersect.p.z, width);
            return getPixelColor(img + layerSize * 2, width, channels, coords.x, coords.y);
        }

        intersect = getYPlaneIntersection(dir, -1.0f);
        if (intersect.valid) {
            glm::ivec2 coords = getPixCoordsFromNormPosition(intersect.p.x, intersect.p.z, width);
            return getPixelColor(img + layerSize * 3, width, channels, coords.x, coords.y);
        }

        intersect = getZPlaneIntersection(dir, 1.0f);
        if (intersect.valid) {
            glm::ivec2 coords = getPixCoordsFromNormPosition(intersect.p.x, intersect.p.y, width);
            return getPixelColor(img + layerSize * 4, width, channels, coords.x, coords.y);
        }

        intersect = getZPlaneIntersection(dir, -1.0f);
        if (intersect.valid) {
            glm::ivec2 coords = getPixCoordsFromNormPosition(-intersect.p.x, intersect.p.y, edgeSamples);
            return getPixelColor(img + layerSize * 5, width, channels, coords.x, coords.y);
        }

        throw std::runtime_error("selection ray failed to intersect cubemap");
        return glm::vec3(0.f, 0.f, 0.f);
    }

    /*
     * Equation of line in all these intersection tests is p = p0 + t * dir
     * p0 is the origin, so we get p = t * dir
     * Then, depending on the plane we check against, we know one of the x, y, z values in p, and can solve for t
     * e.g., for x-plane, pX = t * dirX and pX = 1.0f, so t = 1.0f / dirX
     * Then, solve for pY and pZ by plugging in t
     */

    // checks intersection with plane where x is some constant value
    PlaneIntersection getXPlaneIntersection(glm::vec3 dir, float xValue) {
        float t = xValue / dir.x;
        glm::vec3 p = glm::vec3(xValue, t * dir.y, t * dir.z);

        return {
            p,
            t,
            t > 0.0f && -1.0f < p.y && p.y < 1.0f && -1.0f < p.z && p.z < 1.0f
        };
    }

    // checks intersection with plane where y is some constant value
    PlaneIntersection getYPlaneIntersection(glm::vec3 dir, float yValue) {
        float t = yValue / dir.y;
        glm::vec3 p = glm::vec3(t * dir.x, yValue, t * dir.z);

        return {
            p,
            t,
            t > 0.0f && -1.0f < p.x && p.x < 1.0f && -1.0f < p.z && p.z < 1.0f
        };
    }

    // checks intersection with plane where z is some constant value
    PlaneIntersection getZPlaneIntersection(glm::vec3 dir, float zValue) {
        float t = zValue / dir.z;
        glm::vec3 p = glm::vec3(t * dir.x, t * dir.y, zValue);

        return {
            p,
            t,
            t > 0.0f && -1.0f < p.x && p.x < 1.0f && -1.0f < p.y && p.y < 1.0f
        };
    }

    unsigned char* getPixelMem(unsigned char* img, int width, int channels, int x, int y) {
        size_t pixelOffset = x + y * width;

        return img + (pixelOffset * channels);
    }

    glm::vec3 getPixelColor(unsigned char* img, int width, int channels, int x, int y) {
        unsigned char* mem = getPixelMem(img, width, channels, x, y);

        float r = static_cast<float>(*mem) / 255.f;
        float g = static_cast<float>(*(mem + 1)) / 255.f;
        float b = static_cast<float>(*(mem + 2)) / 255.f;

        return glm::vec3(r, g, b);
    }

    glm::ivec2 getPixCoordsFromNormPosition(float x, float y, int width) {
        float xFraction = (x - -1.f) / 2.f;
        float yFraction = (y - -1.f) / 2.f;

        return glm::ivec2((int)(width * xFraction), (int)(width * yFraction));
    }
};

int main(int argc, char* argv[]) {
    std::cout << std::boolalpha;

    CubeUtil app;

    try {
        app.run(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
