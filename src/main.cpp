// vulkay.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <algorithm>
#include <vector>
#include <thread>
#include "vulkan/instance.hpp"
#include "vulkan/shader.hpp"
#include "platform/window.hpp"

using namespace std;

int main()
{
	const char* appName = "Fugu Vulkan Example";
	Window wnd(appName, 640, 480);
	VulkanInstance inst(appName, &wnd);
	
	Shader vert(inst.device, "simple.vert");
	Shader frag(inst.device, "simple.frag");
	DescriptorSet desc(inst.device);
	desc.add(0, DescriptorSet::UniformBuffer, DescriptorSet::Vertex);
	desc.create();
	

	this_thread::sleep_for(2s);
	return 0;
}

