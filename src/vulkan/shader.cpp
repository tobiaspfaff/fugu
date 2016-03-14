#include "vulkan/shader.hpp"
#include <fstream>
#include <vector>
using namespace std;

Shader::Shader(VkDevice device, const string& name) :
	name(name)
{
	// read binary blob
	ifstream ifs("shader/" + name + ".spv", ios::binary | ios::ate);
	if (!ifs.is_open())
		fatalError("Can't open shader " + name);
	auto pos = ifs.tellg();
	vector<char> buffer(pos);
	ifs.seekg(0, ios::beg);
	ifs.read(buffer.data(), pos);
	ifs.close();

	// create shader
	VkShaderModuleCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.codeSize = pos;
	info.pCode = reinterpret_cast<uint32_t*>(buffer.data());
	
	vkAssert(vkCreateShaderModule(device, &info, nullptr, &module), "create shader");
}

void DescriptorSet::add(int idx, Type type, ShaderType shaderType)
{
	VkDescriptorSetLayoutBinding info;
	info.binding = idx;
	if (type == Type::Sampler)
		info.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	else if (type == Type::UniformBuffer)
		info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	info.descriptorCount = 1;
	info.stageFlags = 0;
	if (shaderType & ShaderType::Vertex)
		info.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
	if (shaderType & ShaderType::Fragment)
		info.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	info.pImmutableSamplers = nullptr;
	bindings.push_back(info);
}

void DescriptorSet::create()
{
	VkDescriptorSetLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.bindingCount = (uint32_t)bindings.size();
	info.pBindings = bindings.data();
	vkAssert(vkCreateDescriptorSetLayout(device, &info, nullptr, &layout), "create set layout");
		
	VkPipelineLayoutCreateInfo plInfo = {};
	plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plInfo.pNext = nullptr;
	plInfo.pushConstantRangeCount = 0;
	plInfo.pPushConstantRanges = nullptr;
	plInfo.setLayoutCount = 1;
	plInfo.pSetLayouts = &layout;

	vkAssert(vkCreatePipelineLayout(device, &plInfo, nullptr, &pipelineLayout), "create pipeline layout");
}