#ifndef _UTILS_H
#define _UTILS_H

#include <stdexcept>

#include <vulkan/vk_enum_string_helper.h>

inline void vkCheckResult(VkResult result, std::string errorMsg) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(errorMsg + std::string(string_VkResult(result)) + " [" + std::to_string(result) + "]");
    }
}

#endif // _UTILS_H