#ifndef _MATERIAL_H
#define _MATERIAL_H

#include <iostream>
#include <string>
#include <variant>

#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include <glm/gtx/io.hpp>

#include "jsonloader.h"
#include "types.h"

template <typename T>
struct Texture {
    enum class SrcType {
        File,
        Value
    };
    enum class Type {
        Tex2D,
        TexCube
    };
    enum class Format {
        Linear,
        RGBE
    };
    SrcType srcType;
    Type type;
    Format format;
    std::variant<
        T,
        std::string
    > src;

    VulkanSampledImage vkImage;

    void print() {
        std::cout << std::endl << "Texture Info" << std::endl;
        std::cout << "Type: " << static_cast<typename std::underlying_type<Type>::type>(type) << std::endl;
        std::cout << "Format: " << static_cast<typename std::underlying_type<Format>::type>(format) << std::endl;

        if (srcType == SrcType::File) {
            std::cout << "Src: " << std::get<std::string>(src) << std::endl;
        } else {
            std::cout << "Src: " << std::get<T>(src) << std::endl;
        }
    }

    void cleanup(VkDevice device) {
        vkImage.cleanup(device);
    }
};

struct PbrProps {
    Texture<glm::vec3> albedo;
    Texture<float> roughness;
    Texture<float> metalness;
};

struct LambertianProps {
    Texture<glm::vec3> albedo;
};

struct Material {
    std::string name;

    std::vector<VkDescriptorSet> descriptorSets;

    Texture<glm::vec3> normalMap;
    Texture<float> displacementMap;

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

    void print() {
        std::cout << std::endl << "Material Name: " << name << std::endl;
        normalMap.print();
        displacementMap.print();

        if (type == Type::LAMBERTIAN) {
            std::get<LambertianProps>(value).albedo.print();
        } else if (type == Type::PBR) {
            std::get<PbrProps>(value).albedo.print();
            std::get<PbrProps>(value).roughness.print();
            std::get<PbrProps>(value).metalness.print();
        }
    }

    void cleanup(VkDevice device) {
        normalMap.cleanup(device);
        displacementMap.cleanup(device);

        if (type == Type::LAMBERTIAN) {
            std::get<LambertianProps>(value).albedo.cleanup(device);
        } else if (type == Type::PBR) {
            std::get<PbrProps>(value).albedo.cleanup(device);
            std::get<PbrProps>(value).metalness.cleanup(device);
            std::get<PbrProps>(value).roughness.cleanup(device);
        }
    }
};

template <class T>
T parseValueFromJson(JsonLoader::JsonNode& json) {
}

template <>
glm::vec3 parseValueFromJson(JsonLoader::JsonNode& json) {
    if (json.type == JsonLoader::JsonNode::Type::ARRAY) {
        std::vector<JsonLoader::JsonNode*>& jsonArray = *std::get<std::vector<JsonLoader::JsonNode*>*>(json.value);

        return {
            std::get<float>(jsonArray[0]->value),
            std::get<float>(jsonArray[1]->value),
            std::get<float>(jsonArray[2]->value)
        };
    } else {
        throw std::runtime_error("Trying to parse a vec3 from a non-ARRAY json node");
    }

    return {};
}

template <>
float parseValueFromJson(JsonLoader::JsonNode& json) {
    if (json.type == JsonLoader::JsonNode::Type::NUMBER) {
        return std::get<float>(json.value);
    } else {
        throw std::runtime_error("Trying to parse a float from a non-NUMBER json node");
    }
}

template <typename T>
Texture<T> parseTextureFromJson(std::map<std::string, JsonLoader::JsonNode*>& json, std::string textureName, T defaultValue) {
    if (json.count(textureName) > 0) {
        // check if the json specifies a filepath or a constant value

        if (json[textureName]->type == JsonLoader::JsonNode::Type::OBJECT) {
            std::map<std::string, JsonLoader::JsonNode*>& jsonObj = *std::get<std::map<std::string, JsonLoader::JsonNode*>*>(json[textureName]->value);

            std::string type = jsonObj.count("type") > 0
                ? std::get<std::string>(jsonObj["type"]->value)
                : "";
            std::string format = jsonObj.count("format") > 0
                ? std::get<std::string>(jsonObj["format"]->value)
                : "";

            return Texture<T>{
                Texture<T>::SrcType::File,
                type == "cube"
                    ? Texture<T>::Type::TexCube
                    : Texture<T>::Type::Tex2D,
                format == "rgbe"
                    ? Texture<T>::Format::RGBE
                    : Texture<T>::Format::Linear,
                std::get<std::string>(jsonObj["src"]->value)
            };
        } else {
            return Texture<T>{
                Texture<T>::SrcType::Value,
                Texture<T>::Type::Tex2D,
                Texture<T>::Format::Linear,
                parseValueFromJson<T>(*json[textureName])
            };
        }
    } else {
        return Texture<T>{
            Texture<T>::SrcType::Value,
            Texture<T>::Type::Tex2D,
            Texture<T>::Format::Linear,
            defaultValue
        };
    }
}

#endif // _MATERIAL_H