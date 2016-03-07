#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#define APP_NAME_STR_LEN 80
#else
#include <unistd.h>
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <string>
#include "util/util.hpp"

inline void vkAssert(VkResult res, const std::string& msg)
{
	if (res != VK_SUCCESS)
		fatalError(msg);
}
