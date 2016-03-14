#pragma once
#include <string>
#include <vector>
#include "vulkan/vkmain.hpp"

class Shader 
{ 
public:
	Shader(VkDevice device, const std::string& name);

	std::string name;
	VkShaderModule module;
};

class DescriptorSet
{
public:
	enum Type { UniformBuffer = 1, Sampler };
	enum ShaderType { Vertex = 1, Fragment = 2, Both = 3 };
	DescriptorSet(VkDevice device) : device(device) {}
	
	void add(int idx, Type type, ShaderType shaderType);
	void create();

	VkDevice device;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout;
	VkPipelineLayout pipelineLayout;
};