#pragma once
#include <vector>
#include <string>
#include "vulkan/vkmain.hpp"

class Window;

struct BufferView {
	VkImage image;
	VkImageView view;
};

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
	void createCommandBuffer();
	void createSwapChain();
	void createDepthBuffer();

	const VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;

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
	VkCommandPool cmdPool;
	VkCommandBuffer cmd;
	VkSwapchainKHR swapChain;
	std::vector<BufferView> swapImages;
	int curSwap = 0;

	VkImage depthImage;
	VkDeviceMemory depthMem;
	VkImageView depthView;
};