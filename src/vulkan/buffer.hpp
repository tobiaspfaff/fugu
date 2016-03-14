#pragma once
#include "vulkan/vkmain.hpp"

struct GpuInfo;

class VulkanBuffer
{
public:
	VulkanBuffer(VkDevice device, const GpuInfo& gpu, size_t size, VkBufferUsageFlags usage);
	void upload(void* ptr);

	VkDevice device;
	VkBuffer buffer;
	VkDeviceMemory mem;
	size_t size, physSize;
};

template<class T>
class UniformBuffer : public VulkanBuffer
{
	UniformBuffer(VkDevice device, const GpuInfo& gpu);
	void upload(const T& data);
};

template<class T>
class VertexBuffer : public VulkanBuffer
{
	VertexBuffer(VkDevice device, const GpuInfo& gpu, int numElements);
	//void upload(const T& data);
};


// ----------------------------------------------------
// IMPLEMENTATION
// ----------------------------------------------------

template<class T>
UniformBuffer<T>::UniformBuffer(VkDevice device, const GpuInfo& gpu) :
	VulkanBuffer(device, gpu, sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
{
}

template<class T>
void UniformBuffer<T>::upload(const T& data)
{
	upload((void*)&data);
}

template<class T>
VertexBuffer<T>::VertexBuffer(VkDevice device, const GpuInfo& gpu, int numElements) :
	VulkanBuffer(device, gpu, numElements*sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
{
}
