#include "vulkan/vkutil.hpp"

uint32_t memoryTypeFromProps(const VkPhysicalDeviceMemoryProperties& props, uint32_t typeBits, VkFlags requirements_mask) 
{
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < 32; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((props.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
				return i;
			}
		}
		typeBits >>= 1;
	}
	fatalError("mem type from props");
	return -1;
}

