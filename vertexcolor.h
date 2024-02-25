#ifndef _VERTEX_COLOR_H
#define _VERTEX_COLOR_H

#include <vector>

#include "glm/glm.hpp"

#include <vulkan/vulkan.h>

#include "types.h"
#include "utils.h"
#include "attribute.h"

struct VertexColor {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 color;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(VertexColor);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexColor, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexColor, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        //attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        attributeDescriptions[2].offset = offsetof(VertexColor, color);

        return attributeDescriptions;
    }

    static void createDescriptorSetLayout(const VkDevice& device,
                                          VkDescriptorSetLayout& descriptorSetLayout) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());;
        layoutInfo.pBindings = bindings.data();

        vkCheckResult(
            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout),
            "failed to create descriptor set layou");
    }

    static void createDescriptorPool(const VkDevice& device,
                                     VkDescriptorPool& descriptorPool,
                                     uint32_t descriptorCount) {
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = descriptorCount;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = descriptorCount;

        vkCheckResult(
            vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool),
            "failed to create descriptor pool");
    }

    static void createDescriptorSets(const VkDevice& device,
                                    const VkDescriptorSetLayout& descriptorSetLayout,
                                    const VkDescriptorPool& descriptorPool,
                                    std::vector<VkDescriptorSet>& descriptorSets,
                                    uint32_t descriptorCount,
                                    /* pipeline-specific arguments */
                                    std::vector<VkBuffer>& uniformBuffers) {
        std::vector<VkDescriptorSetLayout> layouts(descriptorCount, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = descriptorCount;
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(descriptorCount);
        vkCheckResult(
            vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()),
            "failed to allocate descriptor sets");

        for (size_t i = 0; i < descriptorCount; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

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

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(), 0, nullptr);
        }
    }

    static VertexColor readFromFile(std::ifstream& vertexData, const std::vector<Attribute>& attributes) {
        VertexColor v;

        for (Attribute attr : attributes) {
            std::size_t size = getFormatSize(attr.format);

            if (attr.name == "POSITION") {
                vertexData.read(reinterpret_cast<char*>(&v.pos), size);
            }
            else if (attr.name == "NORMAL") {
                vertexData.read(reinterpret_cast<char*>(&v.normal), size);
            }
            else if (attr.name == "COLOR") {
                uint32_t color;

                vertexData.read(reinterpret_cast<char*>(&color), size);

                // the leftmost channel is alpha, so ignoring that since we're just doing rgb colors
                v.color.r = static_cast<float>((color >> 24) & 0xff) / 255.f;
                v.color.g = static_cast<float>((color >> 16) & 0xff) / 255.f;
                v.color.b = static_cast<float>((color >> 8) & 0xff) / 255.f;
                v.color.a = static_cast<float>((color >> 0) & 0xff) / 255.f;
            }
            else {
                std::cout << "Unexpected attribute name: " << attr.name << std::endl;
            }
        }

        return v;
    }

    static size_t getFormatSize(std::string format) {
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
};

#endif // _VERTEX_COLOR_H