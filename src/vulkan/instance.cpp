#include "vulkan/instance.hpp"
#include "platform/window.hpp"
#include "vulkan/vkutil.hpp"
#include <algorithm>
using namespace std;

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
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkPipelineStageFlags srcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(cmd, srcStages, destStages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VulkanInstance::VulkanInstance(const string& appName, Window* wnd) :
	appName(appName), wnd(wnd)
{
	enumerateDevices();
	createDevice(&gpus[0]);
	createCommandBuffer();
	createSwapChain();
	createDepthBuffer();
}

void VulkanInstance::enumerateDevices() 
{
	// enumerate layer properties
	VkResult res = VK_INCOMPLETE;
	while (res == VK_INCOMPLETE)
	{
		// This is a loop, as layerCount could change between those two function calls
		uint32_t layerCount;
		vkAssert(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), "enum instance layers");
		instanceLayerProps.resize(layerCount);
		res = vkEnumerateInstanceLayerProperties(&layerCount, instanceLayerProps.data());
	}
	vkAssert(res, "enum instance layers");

	// create instance
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "Fugu";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

#ifdef _WIN32
	vector<const char*> instanceExtNames{ VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
#else
	fatalError("missing: some Linux thingies");
#endif

	vector<const char*> instanceLayers{};

	VkInstanceCreateInfo instInfo = {};
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pNext = nullptr;
	instInfo.flags = 0;
	instInfo.pApplicationInfo = &appInfo;
	instInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
	instInfo.ppEnabledLayerNames = instanceLayers.empty() ? nullptr : instanceLayers.data();
	instInfo.enabledExtensionCount = (uint32_t)instanceExtNames.size();
	instInfo.ppEnabledExtensionNames = instanceExtNames.data();
	vkAssert(vkCreateInstance(&instInfo, nullptr, &instance), "create instance");

	// enumerate devices
	uint32_t gpuCount;
	vkAssert(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr), "enumerate physical devices");
	if (gpuCount < 1)
		fatalError("No GPU found");
	vector<VkPhysicalDevice> physicalDevices(gpuCount);
	vkAssert(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()), "enumerate physical devices");

	for (auto& phys : physicalDevices)
	{
		GpuInfo gpu;
		gpu.physDevice = phys;
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(phys, &queueCount, nullptr);
		if (queueCount < 1)
			fatalError("No queue present");

		gpu.queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(phys, &queueCount, gpu.queueProps.data());

		vkGetPhysicalDeviceMemoryProperties(phys, &gpu.memoryProps);
		vkGetPhysicalDeviceProperties(phys, &gpu.gpuProps);
		gpus.push_back(gpu);
	}
}

void VulkanInstance::createDevice(GpuInfo* gpu)
{
	this->gpu = gpu;
	
	// Construct the surface description:
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.hinstance = static_cast<HINSTANCE>(wnd->getInstance());
	createInfo.hwnd = static_cast<HWND>(wnd->getHandle());
	vkAssert(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface), "create surface");
#endif // _WIN32

	// Iterate over each queue to learn whether it supports presenting:
	for (int i = 0; i < gpu->queueProps.size(); i++) {
		VkBool32 supports = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(gpu->physDevice, i, surface, &supports);
		if (supports && (gpu->queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			queueFamilyIndex = i;
	}
	if (queueFamilyIndex < 0)
		fatalError("Can't find a queue for graphics+presenting");

	// Get the list of VkFormats that are supported:
	uint32_t formatCount;
	vkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->physDevice, surface, &formatCount, nullptr), "get surface formats");
	vector<VkSurfaceFormatKHR> formats(formatCount);
	vkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->physDevice, surface, &formatCount, formats.data()), "get surface formats");
	if (formats.empty())
		fatalError("No formats reported");

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	format = formats[0].format;
	if (format == VK_FORMAT_UNDEFINED)
		format = VK_FORMAT_B8G8R8A8_UNORM;

	VkDeviceQueueCreateInfo queueInfo = {};
	float queue_priorities[1] = { 0.0 };
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext = nullptr;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = queue_priorities;
	queueInfo.queueFamilyIndex = queueFamilyIndex;

	vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vector<const char*> deviceLayers;

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = nullptr;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledLayerCount = (uint32_t)deviceLayers.size();
	deviceInfo.ppEnabledLayerNames = deviceLayers.empty() ? nullptr : deviceLayers.data();
	deviceInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();
	deviceInfo.pEnabledFeatures = nullptr;
	vkAssert(vkCreateDevice(gpu->physDevice, &deviceInfo, nullptr, &device), "create device");

	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
}

void VulkanInstance::createCommandBuffer()
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

void VulkanInstance::createSwapChain()
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
		swapChainExtent.width = wnd->width;
		swapChainExtent.height = wnd->height;
	}
	else {
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

void VulkanInstance::createDepthBuffer() {
	VkImageCreateInfo imageInfo = {};

	const VkFormat depthFormat = VK_FORMAT_D16_UNORM;

	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(gpu->physDevice, depthFormat, &props);
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) 
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	else
		fatalError("Depth format unsupported");

	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = nullptr;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = depthFormat;
	imageInfo.extent.width = wnd->width;
	imageInfo.extent.height = wnd->height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = numSamples;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.flags = 0;

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlloc.pNext = nullptr;
	memAlloc.allocationSize = 0;
	memAlloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.image = VK_NULL_HANDLE;
	viewInfo.format = depthFormat;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.flags = 0;

	if (depthFormat == VK_FORMAT_D16_UNORM_S8_UINT ||
		depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
		depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) 
	{
		viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkMemoryRequirements memReqs; 

	// Create image
	vkAssert(vkCreateImage(device, &imageInfo, nullptr, &depthImage), "create depth");
	vkGetImageMemoryRequirements(device, depthImage, &memReqs);

	memAlloc.allocationSize = memReqs.size;

	// Use the memory properties to determine the type of memory required
	memAlloc.memoryTypeIndex = memoryTypeFromProps(gpu->memoryProps, memReqs.memoryTypeBits, 0); // 0: No requirements

	// Allocate memory
	vkAssert(vkAllocateMemory(device, &memAlloc, nullptr, &depthMem), "alloc mem");

	// Bind memory
	vkAssert(vkBindImageMemory(device, depthImage, depthMem, 0), "bind mem");
	
	// Set the image layout to depth stencil optimal
	queueImageLayout(cmd, depthImage, viewInfo.subresourceRange.aspectMask,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// Create image view
	viewInfo.image = depthImage;
	vkAssert(vkCreateImageView(device, &viewInfo, nullptr, &depthView), "depth view");
}