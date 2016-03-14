#pragma once
#include "vulkan/vkmain.hpp"

uint32_t memoryTypeFromProps(const VkPhysicalDeviceMemoryProperties& props, 
	uint32_t typeBits, VkFlags requirements_mask);

