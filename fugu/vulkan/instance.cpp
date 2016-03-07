#include "vulkan/instance.hpp"
#include "platform/window.hpp"
using namespace std;

VulkanInstance::VulkanInstance(const string& appName, Window* wnd) :
	appName(appName), wnd(wnd)
{
	enumerateDevices();
	createDevice(&gpus[0]);
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

