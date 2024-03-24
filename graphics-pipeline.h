#ifndef _GRAPHICS_PIPELINE_H
#define _GRAPHICS_PIPELINE_H

#include <vector>

#include <vulkan/vulkan.h>

template <typename V>
class GraphicsPipeline {
public:
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    void destroy(VkDevice device) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
};

#endif // _GRAPHICS_PIPELINE_H