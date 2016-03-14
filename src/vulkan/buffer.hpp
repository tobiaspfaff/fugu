#pragma once
#include "vulkan/vkmain.hpp"

struct GpuInfo;

class UniformBuffer
{
public:
	UniformBuffer(VkDevice device, const GpuInfo& gpu, size_t size);
	void upload(void* data);
	
	VkDevice device;
	VkBuffer buffer;
	VkDeviceMemory mem;
	size_t size, physSize;
};

template<class T>
class TypedUniformBuffer : public UniformBuffer
{
	TypedUniformBuffer(VkDevice device, const GpuInfo& gpu);
	void upload(const T& data);
};