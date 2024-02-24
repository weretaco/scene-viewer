// references
// https://docs.vulkan.org/tutorial/latest/00_Introduction.html
// https://www.mbsoftworks.sk/tutorials/opengl4/026-camera-pt3-orbit-camera/
// https://www.braynzarsoft.net/viewtutorial/q16390-34-aabb-cpu-side-frustum-culling
// https://www.saschawillems.de/blog/2017/09/16/headless-vulkan-examples/

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "jsonloader.h"
#include "eventloader.h"
#include "rg_WindowManager.h"
#include "OrbitCamera.h"

#include "VertexColor.h"
#include "VertexTexture.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// probably create a base vertex class and children, or a Vertex abstract class
// the one thing all Vertex classes must have are positions
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 color;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        //attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        attributeDescriptions[2].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct Attribute {
    std::string name;
    std::string src;
    uint32_t offset;
    uint32_t stride;
    std::string format;

    void print() {
        std::cout << "Attribute Name: " << name << std::endl;
        std::cout << "    Src: " << src << std::endl;
        std::cout << "    Offset: " << offset << std::endl;
        std::cout << "    Stride: " << stride << std::endl;
        std::cout << "    Format: " << format << std::endl;
    }
};

struct Indices {
    std::string src;
    uint32_t offset;
    std::string format;
};

struct Node {
    std::string name;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    std::vector<uint16_t> children;
    std::optional<uint16_t> camera;
    std::optional<uint16_t> mesh;

    void print() {
        std::cout << "Name: " << name << std::endl;

        std::cout << "Translation: " << glm::to_string(translation) << std::endl;
        std::cout << "Rotation: " << glm::to_string(rotation) << std::endl;
        std::cout << "Scale: " << glm::to_string(scale) << std::endl;

        std::cout << "Children: ";
        for (uint16_t child : children) {
            std:: cout << child << " ";
        }
        std::cout << std::endl;

        if (camera.has_value()) {
            std::cout << "Camera: " << camera.value() << std::endl;
        }
        
        if (mesh.has_value()) {
            std::cout << "Mesh: " << mesh.value() << std::endl;
        }
    }
};

struct Mesh {
    std::string name;
    std::string topology;
    uint32_t vertexCount;
    std::string src;
    uint32_t stride;
    Indices indicesData;
    std::vector<Attribute> attributes;
    std::optional<uint32_t> material;

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::array<glm::vec3, 2> aabb; // define min point and max point for AABB

    void cleanupBuffers(VkDevice device) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }

    void print() {
        std::cout << "Name: " << name << std::endl;
        std::cout << "Topology: " << topology << std::endl;
        std::cout << "Vertex Count: " << vertexCount << std::endl;
        std::cout << "Src: " << src << std::endl;
        std::cout << "Stride: " << stride << std::endl;

        if (indicesData.src != "") {
            std::cout << "indices.Src: " << indicesData.src << std::endl;
            std::cout << "indices.Offset: " << indicesData.offset << std::endl;
            std::cout << "indices.Format: " << indicesData.format << std::endl;
        }

        for (Attribute& attr : attributes) {
            attr.print();
        }
    }
};

// assume perspective camera
struct Camera {
    std::string name;
    float vfov;
    float aspect;
    float near;
    float far;
    glm::mat4 viewMat;

    void print() {
        std::cout << "Name: " << name << std::endl;
        std::cout << "Aspect: " << aspect << std::endl;
        std::cout << "Vfov: " << vfov << std::endl;
        std::cout << "Near: " << near << std::endl;
        std::cout << "Far: " << far << std::endl;
        std::cout << "View Mat: " << glm::to_string(viewMat) << std::endl;
    }
};

struct Driver {
    std::string name;
    uint16_t node;
    std::string channel;
    std::vector<float> times;
    std::vector<float> values;
    std::string interpolation;
    uint16_t animIndex;

    void print() {
        std::cout << "Name: " << name << std::endl;

        std::cout << "Node: " << node << std::endl;
        std::cout << "Channel: " << channel << std::endl;
        std::cout << "Interpolation: " << interpolation << std::endl;

        std::cout << "Times: ";
        for (float time : times) {
            std::cout << time << " ";
        }
        std::cout << std::endl;

        std::cout << "Values: ";
        for (float value : values) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
};

struct Animation {
    std::chrono::high_resolution_clock::time_point startTime;
    uint16_t curFrameIndex;
};

struct Texture {
    std::string src;
    enum class Type {
        Tex2D,
        TexCube
    };
    enum class Format {
        linear,
        rgbe
    };
    Type type;
    Format format;
};

template <typename T>
struct MaterialProp {
    enum class Type {
        VALUE,
        SRC
    };
    std::variant<
        T,
        std::string
    > value;
    Type type;
};

struct PbrProps {
    MaterialProp<glm::vec3> albedo;
    MaterialProp<float> roughness;
    MaterialProp<float> metalness;
};

struct LambertianProps {
    MaterialProp<glm::vec3> baseColor;
};

struct Material {
    std::string name;
    std::optional<Texture> normalMap;
    std::optional<Texture> displacementMap;

    enum class Type {
        PBR,
        LAMBERTIAN,
        MIRROR,
        ENVIRONMENT,
        SIMPLE
    };
    std::variant<
        PbrProps,
        LambertianProps,
        std::monostate
    > value;
    Type type;
};

struct Environment {
    std::string name;
    Texture radiance;
};

struct Scene {
    std::vector<Node> nodes;
    std::vector<Mesh> meshes;
    std::vector<Camera> cameras;
    std::vector<Driver> drivers;
    std::vector<Animation> anims;
    std::vector<Material> materials;
    std::vector<uint16_t> roots;

    // maps indices of JSON nodes to the index of the corresponding struct in one of the arrays of the Scene object
    // EXAMPLE: If the first mesh is at index 5 in the JSON array, then typeIndices[5] == 0.
    //          Similary, if the first node is at index 3, then typeIndices[3] == 0 as well
    std::vector<uint16_t> typeIndices;

    void print() {
        std::cout << "Scene:" << std::endl;
        
        std::cout << std::endl << "ROOTS:" << std::endl;
        for (uint16_t& root : roots) {
            std::cout << root << " ";
        }
        std::cout << std::endl;

        std::cout << std::endl << "CAMERAS:" << std::endl;
        for (Camera& camera : cameras) {
            camera.print();
        }
        std::cout << std::endl;

        std::cout << std::endl << "NODES:" << std::endl;
        for (Node& node : nodes) {
            node.print();
        }
        std::cout << std::endl;

        std::cout << std::endl << "MESHES:" << std::endl;
        for (Mesh& mesh : meshes) {
            mesh.print();
        }
        std::cout << std::endl;

        std::cout << std::endl << "DRIVERS:" << std::endl;
        for (Driver& driver : drivers) {
            driver.print();
        }
        std::cout << std::endl;
    }
};

// the vec4 culling planes represent the plane equation: Ax+By+Cz+D = 0, where the vec4 is (A, B, C, D)
struct Frustum {
    glm::vec4 nearPlane;
    glm::vec4 farPlane;
    glm::vec4 leftPlane;
    glm::vec4 rightPlane;
    glm::vec4 topPlane;
    glm::vec4 bottomPlane;
};

struct CLIArguments {
    std::string sceneFile = "";
    std::string physicalDeviceName = "";
    bool listPhysicalDevices = false;
    std::string cameraName = "";
    int width = 0;
    int height = 0;
    std::string eventsFile = "";
    bool headless = false;
    std::string culling = "none";
};

struct GraphicsPipeline {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    void destroy(VkDevice device) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
};

// forward declarations, implementations at the end of this file
void glfwScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

class HelloTriangleApplication {
public:
    void run(int argc, char* argv[]) {
        processCLIArgs(argc, argv);

        if (args.headless) {
            loadHeadlessEvents();
        } else if (!args.listPhysicalDevices) {
            initWindow();
        }

        initVulkan();
        mainLoop();
        cleanup();
    }

    void mouseScrollCallback(float yOffset) {
        if (curCamera == 0) {
            orbitCamera.changeZoom(-yOffset);
        }
    }

    void keyCallback(int key, int action) {
        if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
            curCamera++;

            if (curCamera > scene.cameras.size()) {
                curCamera = 1;
            }

            std::cout << "Changing to scene camera " << (curCamera - 1) << std::endl;
        }
    }

private:
    CLIArguments args;

    std::vector<EventLoader::Event> headlessEvents;
    uint32_t headlessImageIndex = 0;

    rg_Window* window;

    bool mousePressed = false;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    std::vector<VkDeviceMemory> swapChainImagesMemory;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;

    GraphicsPipeline colorPipeline;
    GraphicsPipeline texturePipeline;

    VkCommandPool commandPool;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    VkSampler textureSampler;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    bool framebufferResized = false;
    uint32_t currentFrame = 0;

    UniformBufferObject ubo{};

    Scene scene;
    OrbitCamera orbitCamera;
    uint32_t curCamera = 0; // 0 is the user-controlled orbit camera, values greater than 0 are scene cameras
    Frustum frustum;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open " + filename + "!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    static inline void vkCheckResult(VkResult result, std::string errorMsg) {
        if (result != VK_SUCCESS) {
            throw std::runtime_error(errorMsg + std::string(string_VkResult(result)) + " [" + std::to_string(result) + "]");
        }
    }

    void processCLIArgs(int argc, char* argv[]) {
        args = {
            .sceneFile = ""
        };

        std::cout << "CLI ARGS:" << std::endl;
        for (int i=0; i<argc; i++) {
            std::string arg(argv[i]);

            if (arg.rfind("--", 0) == 0) {
                if (arg == "--scene") {
                    handleArgScene(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--physical-device") {
                    handleArgPhysicalDevice(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--list-physical-devices") {
                    handleArgListPhysicalDevices(std::array<std::string, 1>{ argv[i] });
                } else if (arg == "--camera") {
                    handleArgCamera(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--drawing-size") {
                    handleArgDrawingSize(std::array<std::string, 3>{ argv[i], argv[i+1], argv[i+2] });
                } else if (arg == "--culling") {
                    handleArgCulling(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else if (arg == "--headless") {
                    handleArgHeadless(std::array<std::string, 2>{ argv[i], argv[i+1] });
                } else{
                    std::cout << std::endl << "Unknown command-line argument: " << arg << std::endl;
                }
            }
        }
    }

    void handleArgScene(const std::array<std::string, 2> &arr) {
        args.sceneFile = arr[1];
    }

    void handleArgPhysicalDevice(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        args.physicalDeviceName = arr[1];
    }

    void handleArgListPhysicalDevices(const std::array<std::string, 1> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        args.listPhysicalDevices = true;
    }

    void handleArgCamera(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        args.cameraName = arr[1];
    }

    void handleArgDrawingSize(const std::array<std::string, 3> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;

        try {
            args.width = stoi(arr[1]);
            args.height = stoi(arr[2]);
        } catch (const std::invalid_argument& e) {
            throw std::invalid_argument("At least one of the arguments for --drawing-size is invalud: " + arr[1] + " " + arr[2]);
        }
    }

    void handleArgCulling(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        std::cout << "culling mode: " << arr[1] << std::endl;

        args.culling = arr[1];

        if (args.culling != "frustum" && args.culling != "none") {
            throw std::runtime_error("Unexpected culling mode: " + args.culling + " (must be \"none\" or \"frustum\")");
        }
    }

    void handleArgHeadless(const std::array<std::string, 2> &arr) {
        std::cout << std::endl << "Handling " << arr[0] << std::endl;
        std::cout << "event file: " << arr[1] << std::endl << std::endl;

        if (args.width == 0 && args.height == 0) {
            throw std::invalid_argument("--drawing-size must also be specified when using headless mode");
        }

        args.eventsFile = arr[1];

        if (args.eventsFile != "") {
            args.headless = true;
        }
    }

    void initWindow() {
        rg_WindowManager::init();

        int windowWidth, windowHeight;

        if (args.width == 0 && args.height == 0) {
            windowWidth = WIDTH;
            windowHeight = HEIGHT;
        } else {
            windowWidth = static_cast<int>(args.width);
            windowHeight = static_cast<int>(args.height);
        }

        window = rg_WindowManager::createWindow(windowWidth, windowHeight, "Vulkan");
        if (args.cameraName == "") {
            curCamera = 0;
            orbitCamera = OrbitCamera(windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 80.0f);
        }

        glfwSetWindowUserPointer((GLFWwindow*)(window->getNativeWindowHandle()), this);

        glfwSetScrollCallback((GLFWwindow*)(window->getNativeWindowHandle()), glfwScrollCallback);
        glfwSetKeyCallback((GLFWwindow*)(window->getNativeWindowHandle()), glfwKeyCallback);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();

        if (args.listPhysicalDevices) {
            enumeratePhysicalDevices();

            // Quit the app after listing the devices to let the user pick one
            exit(EXIT_SUCCESS);
        }

        if (!args.headless) {
            createSurface();
        }

        pickPhysicalDevice();
        createLogicalDevice();

        if (args.headless) {
            createHeadlessSwapChain(3);
        } else {
            createSwapChain();
        }

        createImageViews();

        createCommandPool();

        loadSceneGraph();

        createRenderPass();
        createDepthResources();
        createFramebuffers();

        createDescriptorSetLayout();

        createUniformBuffers();

        createGraphicsPipeline(colorPipeline, "color");
        //createGraphicsPipeline(texturePipeline, "texture");
        //createTextureImage();
        //createTextureImageView();
        //createTextureSampler();

        createDescriptorPool();
        createDescriptorSets();

        createCommandBuffers();

        createSyncObjects();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers required, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> requiredExtensions = getRequiredExtensions();
        std::vector<VkExtensionProperties> availableExtensions = getAvailableExtensions();

        uint32_t i = 1;
        std::cout << "Required extensions: (" << requiredExtensions.size() << " total)" << std::endl;
        for (const char* ext : requiredExtensions) {
            std::cout << i << ": " << ext << std::endl;
            i++;
        }
        std::cout << std::endl;

        /*
        i = 1;
        std::cout << "Available extensions: (" << availableExtensions.size() << " total)" << std::endl;
        for (const VkExtensionProperties& ext : availableExtensions) {
            std::cout << i << ": " << ext.extensionName << std::endl;
            i++;
        }
        std::cout << std::endl;
        */

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        vkCheckResult(
            vkCreateInstance(&createInfo, nullptr, &instance),
            "failed to create instance");
    }

    std::vector<VkExtensionProperties> getAvailableExtensions() {
        uint32_t extensionCount = 0;

        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);

        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        return extensions;
    }

    std::vector<const char*> getRequiredExtensions() {
        std::vector<const char*> extensions = {};

        if (!args.headless) {
            extensions = rg_WindowManager::getRequiredExtensions();
        }

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        vkCheckResult(
            CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger),
            "failed to set up debug messenger");
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    void createSurface() {
        vkCheckResult(
            rg_WindowManager::createWindowSurface(instance, window, &surface),
            "failed to create window surface");
    }

    void enumeratePhysicalDevices() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::cout << "Available devices:" << std::endl;

        for (const VkPhysicalDevice& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            std::cout << deviceProperties.deviceName << std::endl;
        }
        std::cout << std::endl;

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        if (args.physicalDeviceName == "") {
            for (const VkPhysicalDevice& device : devices) {
                if (isDeviceSuitable(device)) {
                    physicalDevice = device;
                    break;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }
        } else {
            for (const VkPhysicalDevice& device : devices) {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);

                if (args.physicalDeviceName == deviceProperties.deviceName) {
                    if (!isDeviceSuitable(device)) {
                        throw std::runtime_error("The GPU named \"" + args.physicalDeviceName + "\" does not support the required features!");
                    }

                    physicalDevice = device;
                    break;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("No GPU named \"" + args.physicalDeviceName + "\" was found!");
            }
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        QueueFamilyIndices indices = findQueueFamilies(device);

        if (args.headless) {
            return indices.graphicsFamily.has_value();
        }

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;

                if (args.headless) {
                    break;
                }
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            // TODO: Perhaps add logic to prefer a device that supports both graphics and presentation
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const VkExtensionProperties& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount > 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount > 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies;

        if (args.headless) {
            uniqueQueueFamilies = {
                indices.graphicsFamily.value()
            };
        } else {
            uniqueQueueFamilies = {
                indices.graphicsFamily.value(),
                indices.presentFamily.value()
            };
        }

        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        // our only extensions is the swapchain extension, whichw e don't need in headless mode
        if (!args.headless) {
            createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        vkCheckResult(
            vkCreateDevice(physicalDevice, &createInfo, nullptr, &device),
            "failed to create logical device");

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

        if (!args.headless) {
            vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
        }

        std::cout << "Graphics queue index: " << indices.graphicsFamily.value() << std::endl;

        if (!args.headless) {
            std::cout << "Present queue index: " << indices.presentFamily.value() << std::endl;
        }

        std::cout << std::endl;
    }

    void createHeadlessSwapChain(uint32_t imageCount) {
        swapChainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

        swapChainExtent = {
            static_cast<uint32_t>(args.width),
            static_cast<uint32_t>(args.height)
        };

        swapChainImages.resize(imageCount);
        swapChainImagesMemory.resize(imageCount);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createImage(swapChainExtent.width, swapChainExtent.height, swapChainImageFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                swapChainImages[i], swapChainImagesMemory[i]);
        }
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        vkCheckResult(
            vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain),
            "failed to create swap chain");

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            window->getFramebufferSize(&width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (args.headless) {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        } else {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        std::vector<VkSubpassDependency> dependencies;

        if (args.headless) {
            dependencies.push_back({});
            dependencies.back().srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies.back().dstSubpass = 0;
			dependencies.back().srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies.back().dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies.back().srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies.back().dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies.back().dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies.push_back({});
			dependencies.back().srcSubpass = 0;
			dependencies.back().dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies.back().srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies.back().dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies.back().srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies.back().dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies.back().dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        } else {
            dependencies.push_back({});
            dependencies.back().srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies.back().dstSubpass = 0;
            dependencies.back().srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies.back().srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies.back().dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies.back().dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        vkCheckResult(
            vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass),
            "failed to create render pass");
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        /*
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        */

        //std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());;
        layoutInfo.pBindings = bindings.data();

        vkCheckResult(
            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout),
            "failed to create descriptor set layou");
    }

    void createGraphicsPipeline(GraphicsPipeline& pipeline, std::string shader) {
        std::vector<char> vertShaderCode = readFile("shaders/" + shader + "-vert.spv");
        std::vector<char> fragShaderCode = readFile("shaders/" + shader + "-frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        std::vector<VkVertexInputBindingDescription> bindingDescriptions = Vertex::getBindingDescriptions();
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());;
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPushConstantRange range = {};
        range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        range.offset = 0;
        range.size = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &range;

        vkCheckResult(
            vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline.pipelineLayout),
            "failed to create pipeline layout");

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipeline.pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        vkCheckResult(
            vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline),
            "failed to created graphics pipeline");

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        vkCheckResult(
            vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
            "failed to create shader module");

        return shaderModule;
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            vkCheckResult(
                vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]),
                "failed to create framebuffer");
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        vkCheckResult(
            vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool),
            "failed to create command pool");
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    /*
    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight,
            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage, textureImageMemory);

        transitionImageLayout(textureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copyBufferToImage(stagingBuffer, textureImage,
            static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        transitionImageLayout(textureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    */

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        vkCheckResult(
            vkCreateImage(device, &imageInfo, nullptr, &image),
            "failed to create image");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        vkCheckResult(
            vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory),
            "failed to allocate image memory");

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(commandBuffer,
            buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;

        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        vkCheckResult(
            vkCreateImageView(device, &viewInfo, nullptr, &imageView),
            "failed to create image view");

        return imageView;
    }

    void createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 9.0f;

        vkCheckResult(
            vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler),
            "failed to create texture sampler");
    }

    void loadHeadlessEvents() {
        EventLoader eventLoader(args.eventsFile);

        headlessEvents = eventLoader.parseEvents();

        eventLoader.close();
    }

    void loadSceneGraph() {
        JsonLoader sceneLoader(args.sceneFile);

        std::cout << "LOADING JSON..." << std::endl << std::endl;
        JsonLoader::JsonNode* sceneJson = sceneLoader.parseJson();

        sceneLoader.close();

        std::cout << "CONSTRUCTING SCENE..." << std::endl;
        constructSceneFromJson(scene, sceneJson);

        //scene.print();
        std::cout << std::endl << "SHOWING SCENE CAMERAS" << std::endl;
        size_t i = 0;
        for (const Camera& cam : scene.cameras) {
            std:: cout << cam.name << std::endl;

            if (cam.name == args.cameraName) {
                std::cout << "Setting camera to " << cam.name << std::endl;

                curCamera = i+1; // curCamera == 0 means OrbitCamera
            }

            i++;
        }
        std::cout << std::endl;

        // abort if the named camera wasn't found
        if (args.cameraName != "" && curCamera == 0) {
            throw std::runtime_error("No camera named \"" + args.cameraName + "\" was found in the scene");
        }

        for (Mesh& mesh : scene.meshes) {
            loadVertices(mesh);
            createVertexBuffer(mesh);
            createIndexBuffer(mesh);
            mesh.aabb = getAABB(mesh);
        }
    }

    void constructSceneFromJson(Scene& scene, JsonLoader::JsonNode* json) {
        if (json->type != JsonLoader::JsonNode::Type::ARRAY) {
            throw std::runtime_error("The root of the scene json should be an array");
        }

        std::vector<JsonLoader::JsonNode*>& nodes = *std::get<std::vector<JsonLoader::JsonNode*>*>(json->value);

        for (JsonLoader::JsonNode* n : nodes) {
            JsonLoader::JsonNode& node = *n;

            if (node.type == JsonLoader::JsonNode::Type::STRING && std::get<std::string>(node.value) == "s72-v1") {
                // This will be the first element of the array, ignore it

                scene.typeIndices.push_back(std::numeric_limits<uint16_t>::max());
            } else if (node.type == JsonLoader::JsonNode::Type::OBJECT) {
                std::map<std::string, JsonLoader::JsonNode*>& obj = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(node.value);

                std::string sceneType = std::get<std::string>(obj["type"]->value);

                if (sceneType == "SCENE") {
                    scene.typeIndices.push_back(std::numeric_limits<uint16_t>::max());

                    std::vector<JsonLoader::JsonNode*>& roots = *std::get<std::vector<JsonLoader::JsonNode*>*>(obj["roots"]->value);

                    for (JsonLoader::JsonNode* root : roots) {
                        scene.roots.push_back(static_cast<uint16_t>(std::get<float>(root->value)));
                    }
                } else if (sceneType == "NODE") {
                    scene.typeIndices.push_back(scene.nodes.size());
                    scene.nodes.push_back({});

                    scene.nodes.back().name = std::get<std::string>(obj["name"]->value);

                    if (obj.count("translation") > 0) {
                        scene.nodes.back().translation = parseVec3(obj["translation"]);
                    } else {
                        scene.nodes.back().translation = glm::vec3(0, 0, 0);
                    }

                    if (obj.count("rotation") > 0) {
                        scene.nodes.back().rotation = parseQuat(obj["rotation"]);
                    } else {
                        scene.nodes.back().rotation = glm::quat(1, 0, 0, 0);
                    }

                    if (obj.count("scale") > 0) {
                        scene.nodes.back().scale = parseVec3(obj["scale"]);
                    } else {
                        scene.nodes.back().scale = glm::vec3(1, 1, 1);
                    }

                    if (obj.count("children") > 0) {
                        std::vector<JsonLoader::JsonNode*>& children = *std::get<std::vector<JsonLoader::JsonNode*>*>(obj["children"]->value);

                        for (JsonLoader::JsonNode* node : children) {
                            scene.nodes.back().children.push_back(static_cast<uint16_t>(std::get<float>(node->value)));
                        }
                    }

                    if (obj.count("camera") > 0) {
                        scene.nodes.back().camera = static_cast<uint16_t>(std::get<float>(obj["camera"]->value));
                    }

                    if (obj.count("mesh") > 0) {
                        scene.nodes.back().mesh = static_cast<uint16_t>(std::get<float>(obj["mesh"]->value));
                    }
                } else if (sceneType == "MESH") {
                    scene.typeIndices.push_back(scene.meshes.size());
                    scene.meshes.push_back({});

                    scene.meshes.back().name = std::get<std::string>(obj["name"]->value);
                    scene.meshes.back().topology = std::get<std::string>(obj["topology"]->value);
                    scene.meshes.back().vertexCount = static_cast<uint32_t>(std::get<float>(obj["count"]->value));

                    if (obj.count("indices") > 0) {
                        std::map<std::string, JsonLoader::JsonNode*>& indices = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(obj["indices"]->value);

                        scene.meshes.back().indicesData.src = std::get<std::string>(indices["src"]->value);
                        scene.meshes.back().indicesData.offset = static_cast<uint32_t>(std::get<float>(indices["offset"]->value));
                        scene.meshes.back().indicesData.format = std::get<std::string>(indices["format"]->value);
                    } else {
                        scene.meshes.back().indicesData = { "", 0, ""};
                    }

                    if (obj.count("material") > 0) {
                        scene.meshes.back().material = static_cast<uint32_t>(std::get<float>(obj["material"]->value));
                    }

                    std::map<std::string, JsonLoader::JsonNode*>& attributes = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(obj["attributes"]->value);

                    if (obj.count("material") > 0) {
                        scene.meshes.back().attributes.resize(5);
                    } else {
                        // this mesh uses the simple material
                        scene.meshes.back().attributes.resize(3);
                    }

                    for (const std::pair<const std::string, JsonLoader::JsonNode*>& attr : attributes) {
                        std::map<std::string, JsonLoader::JsonNode*>& attrVal = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(attr.second->value);

                        Attribute a = {
                            attr.first,
                            std::get<std::string>(attrVal["src"]->value),
                            static_cast<uint32_t>(std::get<float>(attrVal["offset"]->value)),
                            static_cast<uint32_t>(std::get<float>(attrVal["stride"]->value)),
                            std::get<std::string>(attrVal["format"]->value)
                        };

                        // hack to get attributes in the right order, should really sort by offset
                        if (obj.count("material") > 0) {
                            scene.meshes.back().attributes.resize(5);

                            if (a.name == "POSITION") {
                                scene.meshes.back().attributes[0] = a;
                            } else if (a.name == "NORMAL") {
                                scene.meshes.back().attributes[1] = a;
                            } else if (a.name == "TANGENT") {
                                scene.meshes.back().attributes[2] = a;
                            } else if (a.name == "TEXCOORD") {
                                scene.meshes.back().attributes[3] = a;
                            } else if (a.name == "COLOR") {
                                scene.meshes.back().attributes[4] = a;
                            }
                        } else {
                            // this mesh uses the simple material
                            scene.meshes.back().attributes.resize(3);

                            if (a.name == "POSITION") {
                                scene.meshes.back().attributes[0] = a;
                            } else if (a.name == "NORMAL") {
                                scene.meshes.back().attributes[1] = a;
                            } else if (a.name == "COLOR") {
                                scene.meshes.back().attributes[2] = a;
                            }
                        }
                    }

                    // Assume that the src and stride for all attributes are the same
                    scene.meshes.back().src = scene.meshes.back().attributes[0].src;
                    scene.meshes.back().stride = scene.meshes.back().attributes[0].stride;
                } else if (sceneType == "CAMERA") {
                    scene.typeIndices.push_back(scene.cameras.size());
                    scene.cameras.push_back({});

                    scene.cameras.back().name = std::get<std::string>(obj["name"]->value);

                    std::map<std::string, JsonLoader::JsonNode*>& perspective = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(obj["perspective"]->value);

                    scene.cameras.back().aspect = std::get<float>(perspective["aspect"]->value);
                    scene.cameras.back().vfov = std::get<float>(perspective["vfov"]->value);
                    scene.cameras.back().near = std::get<float>(perspective["near"]->value);
                    scene.cameras.back().far = std::get<float>(perspective["far"]->value);
                } else if (sceneType == "DRIVER") {
                    scene.typeIndices.push_back(scene.drivers.size());
                    scene.drivers.push_back({});

                    scene.drivers.back().name = std::get<std::string>(obj["name"]->value);
                    scene.drivers.back().node = static_cast<uint16_t>(std::get<float>(obj["node"]->value));
                    scene.drivers.back().channel = std::get<std::string>(obj["channel"]->value);

                    if (obj.count("interpolation") > 0) {
                        scene.drivers.back().interpolation = std::get<std::string>(obj["interpolation"]->value);
                    } else {
                        scene.drivers.back().interpolation = "LINEAR";
                    }

                    std::vector<JsonLoader::JsonNode*>& times = *std::get<std::vector<JsonLoader::JsonNode*>*>(obj["times"]->value);
                    for (JsonLoader::JsonNode* node : times) {
                        scene.drivers.back().times.push_back(std::get<float>(node->value));
                    }

                    std::vector<JsonLoader::JsonNode*>& values = *std::get<std::vector<JsonLoader::JsonNode*>*>(obj["values"]->value);
                    for (JsonLoader::JsonNode* node : values) {
                        scene.drivers.back().values.push_back(std::get<float>(node->value));
                    }

                    scene.drivers.back().animIndex = scene.anims.size();

                    // Also initialize the corresponding Animation objects

                    scene.anims.push_back({});
                    scene.anims.back().curFrameIndex = std::numeric_limits<uint16_t>::max();
                } else if (sceneType == "MATERIAL") {
                    scene.typeIndices.push_back(scene.materials.size());
                    scene.materials.push_back({});

                    scene.materials.back().name = std::get<std::string>(obj["name"]->value);

                    if (obj.count("normalMap") > 0) {
                        std::map<std::string, JsonLoader::JsonNode*>& jsonObj = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(obj["normalMap"]->value);

                        scene.materials.back().normalMap = {
                            src: std::get<std::string>(jsonObj["src"]->value),
                            type: Texture::Type::Tex2D,
                            format: Texture::Format::linear
                        };
                    } else {
                        scene.materials.back().normalMap = {
                            src: "",
                            type: Texture::Type::Tex2D,
                            format: Texture::Format::linear
                        };
                    }

                    if (obj.count("displacementMap") > 0) {
                        std::map<std::string, JsonLoader::JsonNode*>& jsonObj = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(obj["displacementMap"]->value);

                        scene.materials.back().displacementMap = {
                            src: std::get<std::string>(jsonObj["src"]->value),
                            type: Texture::Type::Tex2D,
                            format: Texture::Format::linear
                        };
                    } else {
                        scene.materials.back().displacementMap = {
                            src: "",
                            type: Texture::Type::Tex2D,
                            format: Texture::Format::linear
                        };
                    }
                } else if (sceneType == "ENVIRONMENT") {
                    Environment env; // not sure if I should one or more than one per scene
                }
            } else {
                std::cout << "UNEXPECTED JSON TYPE!" << std::endl;
            }
        }

        // Fix the indices

        for (size_t i = 0; i < scene.roots.size(); i++) {
            scene.roots[i] = scene.typeIndices[scene.roots[i]];
        }

        for (Node& sceneNode : scene.nodes) {
            for (size_t i = 0; i < sceneNode.children.size(); i++) {
                sceneNode.children[i] = scene.typeIndices[sceneNode.children[i]];
            }

            if (sceneNode.camera.has_value()) {
                sceneNode.camera = scene.typeIndices[sceneNode.camera.value()];
            }

            if (sceneNode.mesh.has_value()) {
                sceneNode.mesh = scene.typeIndices[sceneNode.mesh.value()];
            }
        }

        for (Driver& driver : scene.drivers) {
            driver.node = scene.typeIndices[driver.node];
        }

        // Generate view mats for cameras
        for (uint16_t root : scene.roots) {
            Node rootNode = scene.nodes[root];

            generateCameraViewMatsInNode(scene.nodes[root], glm::mat4(1), scene);
        }
    }

    void generateCameraViewMatsInNode(const Node& node, glm::mat4 transform, Scene& scene) {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0), node.scale);
        glm::mat4 rotMat = glm::toMat4(node.rotation);
        glm::mat4 transMat = glm::translate(glm::mat4(1.0), node.translation);

        transform *= transMat * rotMat * scaleMat;

        if (node.camera.has_value()) {
            scene.cameras[node.camera.value()].viewMat = glm::inverse(transform);
        }

        for (uint16_t child : node.children) {
            Node childNode = scene.nodes[child];

            generateCameraViewMatsInNode(childNode, transform, scene);
        }
    }

    void renderSceneGraph(VkCommandBuffer& commandBuffer, Scene& scene) {
        for (uint16_t root : scene.roots) {
            Node rootNode = scene.nodes[root];

            renderNode(commandBuffer, rootNode, glm::mat4(1), scene);
        }
    }

    void renderNode(VkCommandBuffer& commandBuffer, Node& node, glm::mat4 transform, Scene& scene) {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0), node.scale);
        glm::mat4 rotMat = glm::toMat4(node.rotation);
        glm::mat4 transMat = glm::translate(glm::mat4(1.0), node.translation);

        transform *= transMat * rotMat * scaleMat;

        if (node.mesh.has_value()) {
            const Mesh& mesh = scene.meshes[node.mesh.value()];
            const std::array<glm::vec3, 2>& aabb = mesh.aabb;

            glm::vec3 center(transform * glm::vec4((aabb[1] - aabb[0]) / 2.0f, 0.0f));
            glm::vec3 max(transform * glm::vec4(aabb[1], 0.0f));
            glm::vec3 halfExtent = max - center;

            glm::vec3 pos(transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            if (args.culling == "none" || frustumIntersectsAABB(frustum, center, halfExtent)) {
                renderMesh(commandBuffer, mesh, transform);
            }
        }

        for (uint16_t child : node.children) {
            Node childNode = scene.nodes[child];

            renderNode(commandBuffer, childNode, transform, scene);
        }
    }

    void renderMesh(VkCommandBuffer& commandBuffer, const Mesh& mesh, glm::mat4 transform) {
        vkCmdPushConstants(commandBuffer, colorPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);

        VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, colorPipeline.pipelineLayout,
            0, 1, &descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
    }

    glm::vec3 parseVec3(JsonLoader::JsonNode* node) {
        std::vector<JsonLoader::JsonNode*>& components = *std::get<std::vector<JsonLoader::JsonNode*>*>(node->value);

        return glm::vec3(
            std::get<float>(components[0]->value),
            std::get<float>(components[1]->value),
            std::get<float>(components[2]->value)
        );
    }

    glm::vec4 parseVec4(JsonLoader::JsonNode* node) {
        std::vector<JsonLoader::JsonNode*>& components = *std::get<std::vector<JsonLoader::JsonNode*>*>(node->value);

        return glm::vec4(
            std::get<float>(components[0]->value),
            std::get<float>(components[1]->value),
            std::get<float>(components[2]->value),
            std::get<float>(components[3]->value)
        );
    }

    glm::quat parseQuat(JsonLoader::JsonNode* node) {
        std::vector<JsonLoader::JsonNode*>& components = *std::get<std::vector<JsonLoader::JsonNode*>*>(node->value);

        return glm::quat(
            std::get<float>(components[3]->value),
            std::get<float>(components[0]->value),
            std::get<float>(components[1]->value),
            std::get<float>(components[2]->value)
        );
    }

    void loadVertices(Mesh& mesh) {
        // assume that all vertices for a model are in the same file

        mesh.vertices = {};
        mesh.indices = {};

        std::ifstream vertexData;
        vertexData.open("scenes/" + mesh.src, std::ios::binary | std::ios::in);

        if (vertexData.fail()) {
            throw std::runtime_error("Failed to load vertices from " + mesh.src + "!");
        }

        // TODO: Maybe I should read directly into a byte array that I can then copy to the vertex buffer in one go

        for (uint32_t i = 0; i < mesh.vertexCount; i++) {
            mesh.vertices.push_back({});
            mesh.indices.push_back(static_cast<uint16_t>(i));

            for (Attribute attr : mesh.attributes) {
                std::size_t size = getFormatSize(attr.format);

                if (attr.name == "POSITION") {
                    vertexData.read(reinterpret_cast<char*>(&mesh.vertices.back().pos), size);
                }
                else if (attr.name == "NORMAL") {
                    vertexData.read(reinterpret_cast<char*>(&mesh.vertices.back().normal), size);
                }
                else if (attr.name == "COLOR") {
                    uint32_t color;

                    vertexData.read(reinterpret_cast<char*>(&color), size);

                    // the leftmost channel is alpha, so ignoring that since we're just doing rgb colors
                    mesh.vertices.back().color.r = static_cast<float>((color >> 0) & 0xff) / 255.f;
                    mesh.vertices.back().color.g = static_cast<float>((color >> 8) & 0xff) / 255.f;
                    mesh.vertices.back().color.b = static_cast<float>((color >> 16) & 0xff) / 255.f;
                }
                else {
                    std::cout << "Unexpected attribute name: " << attr.name << std::endl;
                }
            }
        }

        vertexData.close();
    }

    size_t getFormatSize(std::string format) {
        if (format == "R32G32B32_SFLOAT") {
            return 12;
        }
        else if (format == "R8G8B8A8_UNORM") {
            return 4;
        }
        else {
            std::cout << "Unexpected input format: " << format << std::endl;
            return 0;
        }
    }

    void createVertexBuffer(Mesh& mesh) {
        VkDeviceSize bufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, mesh.vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            mesh.vertexBuffer, mesh.vertexBufferMemory);

        copyBuffer(stagingBuffer, mesh.vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer(Mesh& mesh) {
        VkDeviceSize bufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, mesh.indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            mesh.indexBuffer, mesh.indexBufferMemory);

        copyBuffer(stagingBuffer, mesh.indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    std::array<glm::vec3, 2> getAABB(const Mesh& mesh) {
        if (mesh.name == "Ground") {
            std::cout << std::endl << "GETTING AABB FOR " << mesh.name << std::endl;
        }

        std::array<glm::vec3, 2> aabb;

        aabb[0] = glm::vec3(std::numeric_limits<float>::max());
        aabb[1] = glm::vec3(std::numeric_limits<float>::min());

        for (const Vertex& vert : mesh.vertices) {
            // min vertex
            aabb[0].x = std::min(aabb[0].x, vert.pos.x);
            aabb[0].y = std::min(aabb[0].y, vert.pos.y);
            aabb[0].z = std::min(aabb[0].z, vert.pos.z);

            // max vertex
            aabb[1].x = std::max(aabb[1].x, vert.pos.x);
            aabb[1].y = std::max(aabb[1].y, vert.pos.y);
            aabb[1].z = std::max(aabb[1].z, vert.pos.z);
        }

        if (mesh.name == "Ground") {
            std::cout << "AABB min: " << glm::to_string(aabb[0]) << std::endl;
            std::cout << "AABB max: " << glm::to_string(aabb[1]) << std::endl;
            std::cout << std::endl;
        }

        return aabb;
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 1;

        vkCheckResult(
            vkCreateBuffer(device, &bufferInfo, nullptr, &buffer),
            "failed to create buffer");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        vkCheckResult(
            vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory),
            "failed to allocate buffer memory");

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find available memory type!");
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        //poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        //poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        vkCheckResult(
            vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool),
            "failed to create descriptor pool");
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        vkCheckResult(
            vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()),
            "failed to allocate descriptor sets");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            /*
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;
            */

            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr;
            descriptorWrites[0].pTexelBufferView = nullptr;

            /*
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = nullptr;
            descriptorWrites[1].pImageInfo = &imageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr;
            */

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(), 0, nullptr);
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        vkCheckResult(
            vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()),
            "failed to allocate command buffers");
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkCheckResult(
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]),
                "failed to create semaphore");

            vkCheckResult(
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]),
                "failed to create semaphore");

            vkCheckResult(
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]),
                "failed to create fence");
        }
    }

    void mainLoop() {
        if (args.headless) {
            std::cout << "Running headless events..." << std::endl << std::endl;
            for (EventLoader::Event ev : headlessEvents) {
                if (ev.type == EventLoader::Event::Type::AVAILABLE) {
                    drawFrameHeadless();
                } else if (ev.type == EventLoader::Event::Type::PLAY) {
                    // TODO: Change animate to allow passing in microseconds, and apply t and rate from the PLAY command
                    //animate(ev.timestamp);
                } else if (ev.type == EventLoader::Event::Type::SAVE) {
                    saveFrame(ev.args[0]);
                } else if (ev.type == EventLoader::Event::Type::MARK) {
                    std::cout << "MARK";

                    for (const std::string& str : ev.args) {
                        std::cout << " " << str;
                    }

                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;

            vkDeviceWaitIdle(device);
        } else {
            std::chrono::high_resolution_clock::time_point curTime;

            while (!window->windowShouldClose()) {
                window->pollEvents();
                handleEvents();

                curTime = std::chrono::high_resolution_clock::now();
                animate(curTime);

                drawFrame();
            }

            vkDeviceWaitIdle(device);
        }
    }

    void saveFrame(const std::string& filename) {
        const char* imagedata;

        VkImage dstImage;
        VkDeviceMemory dstImageMemory;

        createImage(swapChainExtent.width, swapChainExtent.height, swapChainImageFormat,
                    VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    dstImage, dstImageMemory);

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier1{};
        barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.image = dstImage;
        barrier1.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        barrier1.srcAccessMask = 0;
        barrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier1);

        VkImageCopy imageCopyRegion{};
        imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageCopyRegion.srcSubresource.layerCount = 1;
        imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageCopyRegion.dstSubresource.layerCount = 1;
        imageCopyRegion.extent.width = swapChainExtent.width;
        imageCopyRegion.extent.height = swapChainExtent.height;
        imageCopyRegion.extent.depth = 1;

        vkCmdCopyImage(
            commandBuffer,
            swapChainImages[headlessImageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageCopyRegion);

        VkImageMemoryBarrier barrier2{};
        barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.image = dstImage;
        barrier2.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier2);

        endSingleTimeCommands(commandBuffer);

        VkImageSubresource subResource{};
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSubresourceLayout subResourceLayout;

        vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

        vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&imagedata);
        imagedata += subResourceLayout.offset;

        std::ofstream file(filename, std::ios::out | std::ios::binary);

        // ppm header
        file << "P6\n" << swapChainExtent.width << "\n" << swapChainExtent.height << "\n" << 255 << "\n";

        std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
		const bool colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());

        for (uint32_t y = 0; y < swapChainExtent.height; y++) {
            unsigned int *row = (unsigned int*)imagedata;
            for (uint32_t x = 0; x < swapChainExtent.width; x++) {
                if (colorSwizzle) {
                    file.write((char*)row + 2, 1);
                    file.write((char*)row + 1, 1);
                    file.write((char*)row, 1);
                }
                else {
                    file.write((char*)row, 3);
                }
                row++;
            }
            imagedata += subResourceLayout.rowPitch;
        }
        file.close();

        vkUnmapMemory(device, dstImageMemory);
		vkFreeMemory(device, dstImageMemory, nullptr);
		vkDestroyImage(device, dstImage, nullptr);
    }

    void animate(std::chrono::high_resolution_clock::time_point curTime) {
        for (const Driver& driver : scene.drivers) {
            Animation& anim = scene.anims[driver.animIndex];
            Node& node = scene.nodes[driver.node];

            float elapsedTime;

            if (anim.curFrameIndex == std::numeric_limits<uint16_t>::max()) {
                // init the animiation time

                anim.curFrameIndex = 0;
                anim.startTime = curTime;
                elapsedTime = 0.0f;
            } else {
                elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(curTime - anim.startTime).count();

                // stop if we got to the end of the animation (could maybe change this to loop instead)
                while (anim.curFrameIndex < (driver.times.size() - 1) && elapsedTime >= driver.times[anim.curFrameIndex + 1]) {
                    anim.curFrameIndex++;
                }
            }

            size_t dataSize;

            if (driver.channel == "translation" || driver.channel == "scale") {
                dataSize = 3;
            } else if (driver.channel == "rotation") {
                dataSize = 4;
            } else {
                throw std::runtime_error("Unexpected animation channel");
            }

            std::vector<float> curValues, nextFrameValues;

            for (size_t i = 0; i < dataSize; i++) {
                curValues.push_back(driver.values[anim.curFrameIndex * dataSize + i]);
            }

            if (anim.curFrameIndex >= driver.times.size() - 1) {
                // update the values to the ones in the last frame and go to the next animation

                continue;
            }

            for (size_t i = 0; i < dataSize; i++) {
                nextFrameValues.push_back(driver.values[(anim.curFrameIndex + 1) * dataSize + i]);
            }

            float timeUntilNextFrame = driver.times[anim.curFrameIndex + 1] - driver.times[anim.curFrameIndex];
            float timeFraction = (elapsedTime - driver.times[anim.curFrameIndex]) / timeUntilNextFrame;

            if (driver.interpolation == "STEP") {
                glm::vec3 vecCurFrame(curValues[0], curValues[1], curValues[2]);

                if (driver.channel == "translation") {
                    node.translation = vecCurFrame;
                } else if (driver.channel == "scale") {
                    node.scale = vecCurFrame;
                }
            } else if (driver.interpolation == "LINEAR") {
                // currently assuming that channel == "translation" or channel == "scale"

                glm::vec3 vecCurFrame(curValues[0], curValues[1], curValues[2]);
                glm::vec3 vecNextFrame(nextFrameValues[0], nextFrameValues[1], nextFrameValues[2]);

                glm::vec3 interpolated = vecCurFrame + ((vecNextFrame - vecCurFrame) * timeFraction);

                if (driver.channel == "translation") {
                    node.translation = interpolated;
                } else if (driver.channel == "scale") {
                    node.scale = interpolated;
                }
            } else if (driver.interpolation == "SLERP") {
                // currently assuming that channel == "rotation"

                glm::quat vecCurFrame(curValues[3], curValues[0], curValues[1], curValues[2]);
                glm::quat vecNextFrame(nextFrameValues[3], nextFrameValues[0], nextFrameValues[1], nextFrameValues[2]);

                float angle = glm::acos(glm::dot(vecCurFrame, vecNextFrame));
                float denom = glm::sin(angle); 

                glm::quat interpolated = (vecCurFrame * glm::sin((1.0f - timeFraction) * angle) + vecNextFrame * glm::sin(timeFraction * angle)) / denom;

                node.rotation = interpolated;
            }
        }
    }

    void handleEvents() {
        int state = glfwGetMouseButton((GLFWwindow*)(window->getNativeWindowHandle()), GLFW_MOUSE_BUTTON_LEFT);

        if (state == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos((GLFWwindow*)(window->getNativeWindowHandle()), &xpos, &ypos);

            if (curCamera == 0) {
                if (mousePressed) {
                    orbitCamera.rotate(static_cast<float>(xpos), static_cast<float>(ypos));
                } else {
                    orbitCamera.startRotate(static_cast<float>(xpos), static_cast<float>(ypos));
                }
            }
            mousePressed = true;
        } else if (state == GLFW_RELEASE) {
            mousePressed = false;
        }
    }

    void drawFrameHeadless() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        headlessImageIndex++;

        headlessImageIndex %= swapChainFramebuffers.size();

        updateUniformBuffer(currentFrame);

        // Only reset the fence if we are submitting work
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], headlessImageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        vkCheckResult(
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]),
            "failed to submit draw command buffer");

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            std::cout << "OUT OF DATE" << std::endl;
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image");
        }

        updateUniformBuffer(currentFrame);

        // Only reset the fence if we are submitting work
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkCheckResult(
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]),
            "failed to submit draw command buffer");

        VkSwapchainKHR swapChains[] = { swapChain };

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void updateUniformBuffer(uint32_t currentFrame) {
        //static auto startTime = std::chrono::high_resolution_clock::now();

        //std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
        //float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        //ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = getViewFromCamera();
        ubo.proj = getProjFromCamera();

        //std::cout << "Projection: " << glm::to_string(ubo.proj) << std::endl;
        //std::cout << "View: " << glm::to_string(ubo.view) << std::endl;

        generateFrustum(ubo.proj * ubo.view, ubo.view);

        //std::cout << "Near Plane: " << glm::to_string(frustum.nearPlane) << std::endl;
        //std::cout << "Far Plane: " << glm::to_string(frustum.farPlane) << std::endl;

        memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }

    Camera getActiveCam() {
        if (curCamera == 0) {
            return {
                .name = "User Cam",
                .vfov = 0.119856f,
                .aspect = 1.77778f,
                .near = 0.1f,
                .far = 1000.0f
            };
        } else {
            return scene.cameras[curCamera - 1];
        }
    }

    glm::mat4 getViewFromCamera() {
        if (curCamera == 0) {
            return orbitCamera.getViewMatrix();
        } else {
            return scene.cameras[curCamera - 1].viewMat;
        }
    }

    glm::mat4 getProjFromCamera() {
        glm::mat4 proj;

        const Camera& cam = curCamera == 0 ?
            Camera {
                .name = "User Cam",
                .vfov = 0.119856f,
                .aspect = 1.77778f,
                .near = 0.1f,
                .far = 1000.0f
            } :
            scene.cameras[curCamera - 1];

        proj = glm::perspective(cam.vfov, cam.aspect, cam.near, cam.far);
        proj[1][1] *= -1;

        return proj;
    }

    void generateFrustum(const glm::mat4& vpMat, const glm::mat4& viewMat) {
        const Camera& cam = getActiveCam();
        glm::mat4 viewInverse = glm::affineInverse(viewMat);

        glm::vec3 right = glm::vec3(viewInverse[0]);
        glm::vec3 up = glm::vec3(viewInverse[1]);
        glm::vec3 forward = -glm::vec3(viewInverse[2]);
        glm::vec3 eye = glm::vec3(viewInverse[3]);
        float near = cam.near;
        float far = cam.far;
        float half_v = far * tanf(cam.vfov * 0.5f);
        float half_h = half_v * cam.aspect;

        frustum.nearPlane = generatePlane(eye + (forward * near), forward);
        frustum.farPlane = generatePlane(eye + (forward * far), forward * -1.0f);
        frustum.rightPlane =    generatePlane(eye, glm::cross(up, forward * far + right * half_h));
        frustum.leftPlane =     generatePlane(eye, glm::cross(forward * far - right * half_h, up));
        frustum.topPlane =      generatePlane(eye, glm::cross(right, forward * far + up * half_v));
        frustum.bottomPlane =   generatePlane(eye, glm::cross(forward * far - up * half_v, right));
    }

    glm::vec4 generatePlane(glm::vec3 p, glm::vec3 normal) {
        normal = glm::normalize(normal);
        float dist = glm::dot(normal, p);

        return glm::vec4(normal, dist);
    }

    float getSignedDistanceToPlane(const glm::vec4& plane, const glm::vec3& position) {
        return glm::dot(glm::vec3(plane), position) - plane.w;
    }

    bool planeIntersectsAABB(const glm::vec4& plane, const glm::vec3& center, const glm::vec3& extents) {
        glm::vec3 planNormal(plane);

        float r = glm::dot(extents, planNormal);

        return -r <= getSignedDistanceToPlane(plane, center);
    }

    bool frustumIntersectsAABB(const Frustum& frustum, glm::vec3 center, glm::vec3 extents) {
        if (!planeIntersectsAABB(frustum.nearPlane, center, extents)) {
            return false;
        }
        if (!planeIntersectsAABB(frustum.farPlane, center, extents)) {
            return false;
        }
        if (!planeIntersectsAABB(frustum.leftPlane, center, extents)) {
            return false;
        }
        if (!planeIntersectsAABB(frustum.rightPlane, center, extents)) {
            return false;
        }
        if (!planeIntersectsAABB(frustum.topPlane, center, extents)) {
            return false;
        }
        if (!planeIntersectsAABB(frustum.bottomPlane, center, extents)) {
            return false;
        }
        
        return true;
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        vkCheckResult(
            vkBeginCommandBuffer(commandBuffer, &beginInfo),
            "failed to begin recording command buffer");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, colorPipeline.pipeline);

        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        renderSceneGraph(commandBuffer, scene);

        vkCmdEndRenderPass(commandBuffer);

        vkCheckResult(
            vkEndCommandBuffer(commandBuffer),
            "failed to record command buffer");
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            window->getFramebufferSize(&width, &height);
            window->waitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createDepthResources();
        createFramebuffers();
    }

    void cleanupSwapChain() {
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);

        for (VkFramebuffer framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (VkImageView imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void cleanupHeadlessSwapChain() {
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);

        for (VkFramebuffer framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (VkImageView imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        for (VkImage image : swapChainImages) {
            vkDestroyImage(device, image, nullptr);
        }

        for (VkDeviceMemory imageMemory : swapChainImagesMemory) {
            vkFreeMemory(device, imageMemory, nullptr);
        }
    }

    void cleanup() {
        if (args.headless) {
            cleanupHeadlessSwapChain();
        } else {
            cleanupSwapChain();
        }

        //vkDestroySampler(device, textureSampler, nullptr);
        //vkDestroyImageView(device, textureImageView, nullptr);

        //vkDestroyImage(device, textureImage, nullptr);
        //vkFreeMemory(device, textureImageMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        colorPipeline.destroy(device);
        //texturePipeline.destroy(device);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyRenderPass(device, renderPass, nullptr);

        for (Mesh& mesh : scene.meshes) {
            mesh.cleanupBuffers(device);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);

        if (!args.headless) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);

        if (!args.headless) {
            rg_WindowManager::destroyWindow(window);
            rg_WindowManager::cleanup();
        }
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const VkLayerProperties& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }
};

void glfwScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    static_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window))->mouseScrollCallback(static_cast<float>(yOffset));
}

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    static_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window))->keyCallback(key, action);
}

int main(int argc, char* argv[]) {
    std::cout << std::boolalpha;

    HelloTriangleApplication app;

    try {
        app.run(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
