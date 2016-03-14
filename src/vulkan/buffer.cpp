#include "vulkan/buffer.hpp"
#include "vulkan/vkutil.hpp"
#include "vulkan/instance.hpp"
using namespace std;

VulkanBuffer::VulkanBuffer(VkDevice device, const GpuInfo& gpu, size_t size, 
	VkBufferUsageFlags usage) :
	device(device), size(size)
{
	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.usage = usage;
	info.size = size;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.flags = 0;
	vkAssert(vkCreateBuffer(device, &info, nullptr, &buffer), "create buffer");

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, buffer, &memReqs);
	physSize = memReqs.size;

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.memoryTypeIndex = 0;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = memoryTypeFromProps(gpu.memoryProps,
		memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAssert(vkAllocateMemory(device, &allocInfo, nullptr, &mem), "alloc mem");

	vkAssert(vkBindBufferMemory(device, buffer, mem, 0), "bind mem");
}

void VulkanBuffer::upload(void* data)
{
	uint8_t *pData;
	vkAssert(vkMapMemory(device, mem, 0, physSize, 0, (void**)&pData), "map mem");
	memcpy(pData, data, size);
	vkUnmapMemory(device, mem);
}
