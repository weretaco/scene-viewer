// References:
// https://learnopengl.com/PBR/IBL/Specular-IBL

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
    enum class Mode {
        LAMBERTIAN,
        GGX,
        BRDF // will create a brdf lookup table
    };
    Mode mode;

    void run(int argc, char* argv[]) {
        processCLIArgs(argc, argv);

        if (mode == Mode::LAMBERTIAN) {
            loadCubemap();
            processLambertian();
            saveCubemap();
        } else if (mode == Mode::GGX) {
            processGGXCubemap();
        } else {
            generateBRDFLUT();
        }
    }

private:
    CLIArguments args;

    unsigned char* img;
    uint32_t numLayers = 6;
    uint32_t layerSize;
    unsigned char* dstImg;
    int width, height, channels;
    int edgeSamples = 16; // edge samples for the generated cubemap

    // the first vector is the normal, the second is the up vector
    std::array<std::array<glm::vec3, 2>, 6> vecNormalUp = {{
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

    void processCLIArgs(int argc, char* argv[]) {
        args = {};
        args.srcCubemap = "";
        args.dstCubemap = "";

        std::string modeArg(argv[2]);

        if (argc != 4 || (modeArg != "--lambertian" && modeArg != "--ggx" && modeArg != "--brdf")) {
            throw std::invalid_argument("Expected usaged: cube {inFile} --(lambertian|ggx) {outFile}");
        }

        handleMode(modeArg);
        handleArgSrc(std::array<std::string, 1>{ argv[1] });
        handleArgDst(std::array<std::string, 1>{ argv[3] });
    }

    void handleMode(std::string modeArg) {
        if (modeArg == "--lambertian") {
            mode = Mode::LAMBERTIAN;
        } else if (modeArg == "--ggx") {
            mode = Mode::GGX;
        } else if (modeArg == "--brdf") {
            mode = Mode::BRDF;
        } else {
            throw std::runtime_error("Unsuppoerted mode: " + modeArg);
        }
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
        dstImg = (stbi_uc*)malloc(edgeSamples * edgeSamples * channels * numLayers);
    }

    void processGGXCubemap() {
        std::cout << "Loading cubemap from " << args.srcCubemap << std::endl;

        img = stbi_load(args.srcCubemap.c_str(), &width, &height, &channels, 0);
        if (img == nullptr) {
            throw std::runtime_error("Error loading the image: " + args.srcCubemap);
        }
        std::cout << "Loaded image with a width of " << width << "px, a height of " << height << "px and " << channels << " channels" << std::endl;
    
        size_t imageWidth = width;

        uint32_t NUM_MIP_LEVELS = 5;

        std::string fileBase = args.dstCubemap.substr(0, args.dstCubemap.find("."));
        std::string fileExt = args.dstCubemap.substr(args.dstCubemap.find(".") + 1);

        for (int i=0; i<NUM_MIP_LEVELS; i++) {
            std::cout << "Creating mip level " << i << " (with image width " << imageWidth << ")" << std::endl;
            size_t imageSize = imageWidth * imageWidth * channels * numLayers;

            dstImg = (stbi_uc*)malloc(imageSize);

            std::string dstFilename = fileBase + "." + std::to_string(i) + "." + fileExt;

            std::cout << "Saving to " << dstFilename << std::endl;

            processGGXMipLevel(img, width, dstImg, imageWidth, i);

            stbi_write_png(dstFilename.c_str(), imageWidth, imageWidth * numLayers, channels, dstImg, imageWidth * channels);

            stbi_image_free(dstImg);

            imageWidth = floor(((float)imageWidth) / 2.f);
        }

        glm::vec3 N( 0.0f,  0.0f,  1.0f);
    }

    void processGGXMipLevel(unsigned char* srcImg, uint32_t srcWidth, unsigned char* dstImg, uint32_t dstWidth, uint32_t roughness) {
        uint32_t srcLayerSize = srcWidth * srcWidth * channels;
        uint32_t dstLayerSize = dstWidth * dstWidth * channels;

        // the six images are in this order in the file:
        // right, left, up, down, front, back
        // they should be in this order on the GPU:
        // front, back, up, down, right, left

        std::array<uint32_t, 6> srcImageOffsets = {
            srcLayerSize * 4,
            srcLayerSize * 5,
            srcLayerSize * 2,
            srcLayerSize * 3,
            srcLayerSize * 0,
            srcLayerSize * 1
        };

        std::array<uint32_t, 6> dstImageOffsets = {
            dstLayerSize * 4,
            dstLayerSize * 5,
            dstLayerSize * 2,
            dstLayerSize * 3,
            dstLayerSize * 0,
            dstLayerSize * 1
        };

        for (uint32_t i = 0; i < numLayers; i++) {
            computeGGXFace(srcImg + srcImageOffsets[i], dstImg + dstImageOffsets[i], srcWidth, dstWidth, vecNormalUp[i][0], vecNormalUp[i][1], roughness);
        }
    }

    void computeGGXFace(unsigned char* srcImgLayer, unsigned char* dstImgLayer, uint32_t srcWidth, uint32_t dstWidth, const glm::vec3& normal, const glm::vec3& up, uint32_t roughness) {
        glm::vec3 right = glm::normalize(glm::cross(up, normal));

        float faceWidth = 2.f; // -1.0 to 1.0
        float pixelSize = faceWidth / dstWidth;

        for (int i = 0; i < dstWidth; i++) {
            for (int j = 0; j < dstWidth; j++) {
                float upComp = -1.f + (pixelSize / 2) + (i * pixelSize);
                float rightComp = -1.f + (pixelSize / 2) + (j * pixelSize);

                glm::vec3 pos = normal + upComp * (-up) + rightComp * right;

                // get a pointer to the current pixel of the destination image
                unsigned char *p = getPixelMem(dstImgLayer, dstWidth, channels, j, i);

                glm::vec3 N = glm::normalize(pos);
                glm::vec3 R = N;
                glm::vec3 V = R;

                const int SAMPLE_COUNT = 1024u;
                float totalWeight = 0.0;   
                glm::vec3 color = glm::vec3(0.0);

                for (int i = 0; i < SAMPLE_COUNT; i++) {
                    glm::vec2 Xi = hammersley(i, SAMPLE_COUNT);;
                    glm::vec3 H = importanceSampleGGX(Xi, N, roughness);
                    glm::vec3 L = glm::normalize(2.0f * glm::dot(V, H) * H - V);

                    float NdotL = std::max(glm::dot(N, L), 0.0f);
                    if (NdotL > 0.0f) {
                        color += sampleCubemap(L) * NdotL;
                        totalWeight += NdotL;
                    }
                }

                *p = (uint8_t)(color.r * 255);
                *(p + 1) = (uint8_t)(color.g * 255);
                *(p + 2) = (uint8_t)(color.b * 255);
                *(p + 3) = 255;
            }
        }
    }

    glm::vec3 importanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness) {
        float a = roughness*roughness;

        float phi = 2.0f * glm::pi<float>() * Xi.x;
        float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a*a - 1.0f) * Xi.y));
        float sinTheta = sqrt(1.0f - cosTheta*cosTheta);

        glm::vec3 H(
            cos(phi) * sinTheta,
            sin(phi) * sinTheta,
            cosTheta);

        glm::vec3 up = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
        glm::vec3 tangent   = glm::normalize(cross(up, N));
        glm::vec3 bitangent = glm::cross(N, tangent);

        glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
        return glm::normalize(sampleVec);
    }

    glm::vec2 hammersley(uint i, uint N) {
        return glm::vec2(float(i) / float(N), vanDerCorput(i, 2u));
    }

    float vanDerCorput(uint n, uint base) {
        float invBase = 1.0 / float(base);
        float denom   = 1.0;
        float result  = 0.0;

        for(uint i = 0u; i < 32u; ++i) {
            if (n > 0u) {
                denom = std::fmod((float) n, 2.0f);
                result += denom * invBase;
                invBase = invBase / 2.0;
                n = uint(float(n) / 2.0);
            }
        }

        return result;
    }

    void generateBRDFLUT() {
        // Reusing the dstCubemap var here, but it's just a texture, not a cubemap

        int samples = 256;

        std::cout << "Generating BRDF LUT and writing it to " << args.dstCubemap << std::endl;

        size_t imageWidth = 128;
        int channels = 4;
        size_t imageSize = imageWidth * imageWidth * channels;

        unsigned char* dstImg = (stbi_uc*)malloc(imageSize);

        for (int y = 0; y < imageWidth; y++) {
		    for (int x = 0; x < imageWidth; x++) {
			    float NoV = (y + 0.5f) * (1.0f / imageWidth);
			    float roughness = (x + 0.5f) * (1.0f / imageWidth);

                unsigned char *p = getPixelMem(dstImg, imageWidth, channels, x, y);

                glm::vec2 val = integrateBRDF(NoV, roughness, samples);

                *p = (uint8_t)(val.r * 255);
                *(p + 1) = (uint8_t)(val.g * 255);
                *(p + 2) = 0;
                *(p + 3) = 255;
            }
        }

        stbi_write_png(args.dstCubemap.c_str(), imageWidth, imageWidth, channels, dstImg, imageWidth * channels);

        stbi_image_free(dstImg);
    }

    glm::vec2 integrateBRDF(float NdotV, float roughness, unsigned int samples) {
        glm::vec3 V(
	        sqrt(1.0 - NdotV * NdotV),
	        0.0,
	        NdotV);

        glm::vec3 N(0.0, 0.0, 1.0);

        float A = 0.0;
	    float B = 0.0;

        for (int i = 0; i < samples; i++) {
            glm::vec2 Xi = hammersley(i, samples);
            glm::vec3 H = importanceSampleGGX(Xi, N, roughness);
            glm::vec3 L = normalize(2.0f * dot(V, H) * H - V);

            float NoL = glm::max(L.z, 0.0f);
            float NoH = glm::max(H.z, 0.0f);
            float VoH = glm::max(dot(V, H), 0.0f);
            float NoV = glm::max(dot(N, V), 0.0f);

            if (NoL > 0.0f) {
                float G = geometrySmith(roughness, NoV, NoL);

                float G_Vis = (G * VoH) / (NoH * NoV);
                float Fc = pow(1.0f - VoH, 5.0f);

                A += (1.0f - Fc) * G_Vis;
                B += Fc * G_Vis;
            }
        }

        return glm::vec2(A, B) / (float) samples;
    }

    float geometrySmith(float roughness, float NoV, float NoL) {
        float ggx1 = geometrySchlickGGX(NoL, roughness);
        float ggx2 = geometrySchlickGGX(NoV, roughness);

        return ggx1 * ggx2;
    }

    float geometrySchlickGGX(float NdotV, float roughness) {
        float a = roughness;
        float k = (a * a) / 2.0f;

        float nom = NdotV;
        float denom = NdotV * (1.0f - k) + k;

        return nom / denom;
    }

    void processLambertian() {
        std::cout << "Processing lambertian cubemap" << std::endl;

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

        for (uint32_t i = 0; i < numLayers; i++) {
            convolveFace(img + imageOffsets[i], dstImg + lucOffsets[i], vecNormalUp[i][0], vecNormalUp[i][1]);
        }
    }

    void saveCubemap() {
        std::cout << "Saving cubemap to " << args.dstCubemap << std::endl;

        //stbi_write_png(args.dstCubemap.c_str(), width, height, channels, img, width * channels);
        stbi_write_png(args.dstCubemap.c_str(), edgeSamples, edgeSamples * numLayers, channels, dstImg, edgeSamples * channels);

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

        throw std::runtime_error(std::string("selection ray failed to intersect cubemap") + glm::to_string(dir));
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

        //std::cout << "X" << std::endl;
        //std::cout << "Check p: " << glm::to_string(p) << std::endl;
        //std::cout << "Check t: " << t << std::endl;

        return {
            p,
            t,
            t > 0.0f && -1.00005f < p.y && p.y < 1.00005f && -1.00005f < p.z && p.z < 1.00005f
        };
    }

    // checks intersection with plane where y is some constant value
    PlaneIntersection getYPlaneIntersection(glm::vec3 dir, float yValue) {
        float t = yValue / dir.y;
        glm::vec3 p = glm::vec3(t * dir.x, yValue, t * dir.z);

        //std::cout << "Y" << std::endl;
        //std::cout << "Check p: " << glm::to_string(p) << std::endl;
        //std::cout << "Check t: " << t << std::endl;

        return {
            p,
            t,
            t > 0.0f && -1.00005f < p.x && p.x < 1.00005f && -1.00005f < p.z && p.z < 1.00005f
        };
    }

    // checks intersection with plane where z is some constant value
    PlaneIntersection getZPlaneIntersection(glm::vec3 dir, float zValue) {
        float t = zValue / dir.z;
        glm::vec3 p = glm::vec3(t * dir.x, t * dir.y, zValue);

        return {
            p,
            t,
            t > 0.0f && -1.00005f < p.x && p.x < 1.00005f && -1.00005f < p.y && p.y < 1.00005f
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
