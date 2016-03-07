#pragma once
#include <vector>
#include <string>
#include "vulkan/vkmain.hpp"

class Window;

struct GpuInfo 
{
	VkPhysicalDevice physDevice;
	std::vector<VkQueueFamilyProperties> queueProps;
	VkPhysicalDeviceMemoryProperties memoryProps;
	VkPhysicalDeviceProperties gpuProps;
};

class VulkanInstance
{
public:
	VulkanInstance(const std::string& appName, Window* wnd);
	void enumerateDevices();
	void createDevice(GpuInfo* gpu);

	std::string appName;
	std::vector<VkLayerProperties> instanceLayerProps;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkFormat format;
	VkQueue queue;
	int queueFamilyIndex = -1;
	std::vector<GpuInfo> gpus;
	GpuInfo* gpu = nullptr;
	VkDevice device;
	Window* wnd;
};