// vulkay.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <algorithm>
#include <vector>
#include <thread>
#include "vulkan/instance.hpp"
#include "platform/window.hpp"

using namespace std;

struct BufferView {
	VkImage image;
	VkImageView view;
};

struct VulkanInfo {
	vector<VkLayerProperties> instanceLayerProps;
	VkInstance instance;
	vector<GpuInfo> gpus;
	GpuInfo* gpu = nullptr;
	int width;
	int height;
	VkSurfaceKHR surface;
	int queueFamilyIndex = -1;
	VkFormat format;
	VkDevice device;
	VkQueue queue;
	VkCommandPool cmdPool;
	VkCommandBuffer cmd;
	VkSwapchainKHR swapChain;
	std::vector<BufferView> swapImages;
	int curSwap = 0;

	static VulkanInfo createInstance(const char* appName);
	void createDevice(GpuInfo* gpu);
	void createCommandBuffer();
	void createSwapChain();
protected:
	VulkanInfo() {}
};

void queueImageLayout(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspectMask, 
	VkImageLayout oldLayout, VkImageLayout newLayout) 
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask =	VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		barrier.dstAccessMask =	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		barrier.dstAccessMask =	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkPipelineStageFlags srcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(cmd, srcStages, destStages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanInfo::createCommandBuffer()
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.pNext = nullptr;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkAssert(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool), "create pool");

	VkCommandBufferAllocateInfo cmdInfo = {};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdInfo.pNext = nullptr;
	cmdInfo.commandPool = cmdPool;
	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdInfo.commandBufferCount = 1;
	vkAssert(vkAllocateCommandBuffers(device, &cmdInfo, &cmd), "create command buffer");

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;
	cmdBufInfo.flags = 0;
	cmdBufInfo.pInheritanceInfo = nullptr;
	vkAssert(vkBeginCommandBuffer(cmd, &cmdBufInfo), "begin command buffer");
}

void VulkanInfo::createSwapChain() 
{
	// Init swap chain
	VkSurfaceCapabilitiesKHR caps;
	uint32_t presentModeCount;
	vkAssert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->physDevice, surface, &caps), "get caps");
	vkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->physDevice, surface, &presentModeCount, nullptr), "get modes");
	vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->physDevice, surface, &presentModeCount, presentModes.data()), "get modes");

	// width and height are either both -1, or both not -1.
	VkExtent2D swapChainExtent;
	if (caps.currentExtent.width == (uint32_t)-1) 
	{
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapChainExtent.width = width;
		swapChainExtent.height = height;
	} else {
		// If the surface size is defined, the swap chain size must match
		swapChainExtent = caps.currentExtent;
	}

	// If mailbox mode is available, use it, as is the lowest-latency non-
	// tearing mode.  If not, try IMMEDIATE which will usually be available,
	// and is fastest (though it tears).  If not, fall back to FIFO which is
	// always available.
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end())
		swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	else if (find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end())
		swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	// Determine the number of VkImages to use in the swap chain (we desire to
	// own only 1 image at a time, besides the images being displayed and
	// queued for display):
	uint32_t swapChainImages = caps.minImageCount + 1;
	if (caps.maxImageCount > 0)
		swapChainImages = min(swapChainImages, caps.maxImageCount);

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else
		preTransform = caps.currentTransform;
	
	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.pNext = nullptr;
	swapChainInfo.surface = surface;
	swapChainInfo.minImageCount = swapChainImages;
	swapChainInfo.imageFormat = format;
	swapChainInfo.imageExtent.width = swapChainExtent.width;
	swapChainInfo.imageExtent.height = swapChainExtent.height;
	swapChainInfo.preTransform = preTransform;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.presentMode = swapchainPresentMode;
	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;
	swapChainInfo.clipped = true;
	swapChainInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainInfo.queueFamilyIndexCount = 0;
	swapChainInfo.pQueueFamilyIndices = nullptr;
	vkAssert(vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &swapChain), "create swapchain");
	
	uint32_t imageCnt;
	vkAssert(vkGetSwapchainImagesKHR(device, swapChain, &imageCnt, nullptr), "get images");
	vector<VkImage> images(imageCnt);
	vkAssert(vkGetSwapchainImagesKHR(device, swapChain, &imageCnt, images.data()), "get images");
	
	for (uint32_t i = 0; i < imageCnt; i++)
	{
		queueImageLayout(cmd, images[i], VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		BufferView buf;
		buf.image = images[i];
		VkImageViewCreateInfo imageViewInfo = {};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.pNext = nullptr;
		imageViewInfo.format = format;
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.flags = 0;
		imageViewInfo.image = images[i];
		vkAssert(vkCreateImageView(device, &imageViewInfo, nullptr, &buf.view), "create view");
		
		swapImages.push_back(buf);
	}
}

int main()
{
	const char* appName = "Fugu Vulkan Example";
	Window wnd(appName, 640, 480);
	VulkanInstance inst(appName, &wnd);
	//info.createCommandBuffer();
	
	this_thread::sleep_for(2s);
	return 0;
}

