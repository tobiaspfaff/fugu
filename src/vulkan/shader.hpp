#pragma once
#include <string>
#include <vector>
#include <memory>
#include "vulkan/vkmain.hpp"

class VulkanBuffer;

class Shader 
{ 
public:
	Shader(VkDevice device, const std::string& name);

	std::string name;
	VkShaderModule module;
};

class Binding 
{
public:
	void setBuffer(int idx, const VulkanBuffer& buffer);
	void apply();

	struct BindData {
		VkDescriptorBufferInfo bufferInfo;
		VkDescriptorImageInfo imageInfo;
	};

	VkDescriptorSet set;
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<BindData> bindData;
	VkDevice device;
};

class DescriptorSetLayout
{
public:
	enum Type { UniformBuffer = 1, Sampler };
	enum ShaderType { Vertex = 1, Fragment = 2, Both = 3 };
	DescriptorSetLayout(VkDevice device) : device(device) {}
	
	void add(int idx, Type type, ShaderType shaderType);
	void create();
	std::unique_ptr<Binding> createBinding();

	VkDevice device;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout;
	VkPipelineLayout pipelineLayout;
	VkDescriptorPool pool;
};