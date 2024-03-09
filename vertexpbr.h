#ifndef _VERTEX_PBR_H
#define _VERTEX_PBR_H

#include <array>
#include <fstream>
#include <vector>

#include "glm/glm.hpp"

#include <vulkan/vulkan.h>

#include "types.h"
#include "utils.h"
#include "attribute.h"

struct VertexPBR {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 texCoord;
    glm::vec4 color;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(VertexPBR);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexPBR, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexPBR, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexPBR, tangent);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(VertexPBR, texCoord);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(VertexPBR, color);

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

        VkDescriptorSetLayoutBinding albedoSamplerLayoutBinding{};
        albedoSamplerLayoutBinding.binding = 1;
        albedoSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        albedoSamplerLayoutBinding.descriptorCount = 1;
        albedoSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        albedoSamplerLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding normalSamplerLayoutBinding{};
        normalSamplerLayoutBinding.binding = 2;
        normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalSamplerLayoutBinding.descriptorCount = 1;
        normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        normalSamplerLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding metallicSamplerLayoutBinding{};
        metallicSamplerLayoutBinding.binding = 3;
        metallicSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        metallicSamplerLayoutBinding.descriptorCount = 1;
        metallicSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        metallicSamplerLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding roughnessSamplerLayoutBinding{};
        roughnessSamplerLayoutBinding.binding = 4;
        roughnessSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        roughnessSamplerLayoutBinding.descriptorCount = 1;
        roughnessSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        roughnessSamplerLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding cubemapSamplerLayoutBinding{};
        cubemapSamplerLayoutBinding.binding = 5;
        cubemapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cubemapSamplerLayoutBinding.descriptorCount = 1;
        cubemapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        cubemapSamplerLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 6> bindings = { uboLayoutBinding, albedoSamplerLayoutBinding, normalSamplerLayoutBinding, metallicSamplerLayoutBinding, roughnessSamplerLayoutBinding, cubemapSamplerLayoutBinding };
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
        std::array<VkDescriptorPoolSize, 6> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = descriptorCount;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = descriptorCount;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = descriptorCount;
        poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[3].descriptorCount = descriptorCount;
        poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[4].descriptorCount = descriptorCount;
        poolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[5].descriptorCount = descriptorCount;

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
                                    std::vector<VkBuffer>& uniformBuffers,
                                    VkImageView& albedoTexImageView,
                                    VkSampler& albedoTexSampler,
                                    VkImageView& normalTexImageView,
                                    VkSampler& normalTexSampler,
                                    VkImageView& metallicTexImageView,
                                    VkSampler& metallicTexSampler,
                                    VkImageView& roughnessTexImageView,
                                    VkSampler& roughnessTexSampler,
                                    VkImageView& cubemapTexImageView,
                                    VkSampler& cubemapTexSampler) {
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

            VkDescriptorImageInfo albedoImageInfo{};
            albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            albedoImageInfo.imageView = albedoTexImageView;
            albedoImageInfo.sampler = albedoTexSampler;

            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalImageInfo.imageView = normalTexImageView;
            normalImageInfo.sampler = normalTexSampler;

            VkDescriptorImageInfo metallicImageInfo{};
            metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallicImageInfo.imageView = metallicTexImageView;
            metallicImageInfo.sampler = metallicTexSampler;

            VkDescriptorImageInfo roughnessImageInfo{};
            roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            roughnessImageInfo.imageView = roughnessTexImageView;
            roughnessImageInfo.sampler = roughnessTexSampler;

            VkDescriptorImageInfo cubemapImageInfo{};
            cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            cubemapImageInfo.imageView = cubemapTexImageView;
            cubemapImageInfo.sampler = cubemapTexSampler;

            std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr;
            descriptorWrites[0].pTexelBufferView = nullptr;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = nullptr;
            descriptorWrites[1].pImageInfo = &albedoImageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = nullptr;
            descriptorWrites[2].pImageInfo = &normalImageInfo;
            descriptorWrites[2].pTexelBufferView = nullptr;

            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = descriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pBufferInfo = nullptr;
            descriptorWrites[3].pImageInfo = &metallicImageInfo;
            descriptorWrites[3].pTexelBufferView = nullptr;

            descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet = descriptorSets[i];
            descriptorWrites[4].dstBinding = 4;
            descriptorWrites[4].dstArrayElement = 0;
            descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[4].descriptorCount = 1;
            descriptorWrites[4].pBufferInfo = nullptr;
            descriptorWrites[4].pImageInfo = &roughnessImageInfo;
            descriptorWrites[4].pTexelBufferView = nullptr;

            descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[5].dstSet = descriptorSets[i];
            descriptorWrites[5].dstBinding = 5;
            descriptorWrites[5].dstArrayElement = 0;
            descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[5].descriptorCount = 1;
            descriptorWrites[5].pBufferInfo = nullptr;
            descriptorWrites[5].pImageInfo = &cubemapImageInfo;
            descriptorWrites[5].pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(), 0, nullptr);
        }
    }

    static VertexPBR readFromFile(std::ifstream& vertexData, const std::vector<Attribute>& attributes) {
        VertexPBR v;

        for (Attribute attr : attributes) {
            std::size_t size = getFormatSize(attr.format);

            if (attr.name == "POSITION") {
                vertexData.read(reinterpret_cast<char*>(&v.pos), size);
            } else if (attr.name == "NORMAL") {
                vertexData.read(reinterpret_cast<char*>(&v.normal), size);
            } else if (attr.name == "TANGENT") {
                vertexData.read(reinterpret_cast<char*>(&v.tangent), size);
            } else if (attr.name == "TEXCOORD") {
                vertexData.read(reinterpret_cast<char*>(&v.texCoord), size);
            } else if (attr.name == "COLOR") {
                uint32_t color;

                vertexData.read(reinterpret_cast<char*>(&color), size);

                // the leftmost channel is alpha, so ignoring that since we're just doing rgb colors
                v.color.r = static_cast<float>((color >> 0) & 0xff) / 255.f;
                v.color.g = static_cast<float>((color >> 8) & 0xff) / 255.f;
                v.color.b = static_cast<float>((color >> 16) & 0xff) / 255.f;
                v.color.a = static_cast<float>((color >> 24) & 0xff) / 255.f;
            }
            else {
                std::cout << "Unexpected attribute name: " << attr.name << std::endl;
            }
        }

        return v;
    }

    static size_t getFormatSize(std::string format) {
        if (format == "R32G32_SFLOAT") {
            return 8;
        } else if (format == "R32G32B32_SFLOAT") {
            return 12;
        } else if (format == "R32G32B32A32_SFLOAT") {
            return 16;
        } else if (format == "R8G8B8A8_UNORM") {
            return 4;
        } else {
            std::cout << "Unexpected input format: " << format << std::endl;
            return 0;
        }
    }
};

#endif // _VERTEX_PBR_H