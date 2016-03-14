#include "vulkan/shader.hpp"
#include "vulkan/buffer.hpp"
#include <fstream>
#include <vector>
#include <cassert>
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

void DescriptorSetLayout::add(int idx, Type type, ShaderType shaderType)
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

void DescriptorSetLayout::create()
{
	// Desc Set
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.bindingCount = (uint32_t)bindings.size();
	layoutInfo.pBindings = bindings.data();
	vkAssert(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout), "create set layout");
		
	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.pushConstantRangeCount = 0;
	pipelineInfo.pPushConstantRanges = nullptr;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &layout;
	vkAssert(vkCreatePipelineLayout(device, &pipelineInfo, nullptr, &pipelineLayout), "create pipeline layout");

	// Desc Pool
	vector<VkDescriptorPoolSize> typeCount(bindings.size());
	for (int i = 0; i < bindings.size(); i++) {
		typeCount[i].type = bindings[i].descriptorType;
		typeCount[i].descriptorCount = 1;
	}
	
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = (uint32_t)typeCount.size();
	poolInfo.pPoolSizes = typeCount.data();

	vkAssert(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool), "create desc pool");
}

unique_ptr<Binding> DescriptorSetLayout::createBinding()
{
	auto bnd = make_unique<Binding>();
	VkDescriptorSetAllocateInfo info[1];
	info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info[0].pNext = nullptr;
	info[0].descriptorPool = pool;
	info[0].descriptorSetCount = 1;
	info[0].pSetLayouts = &layout;
	vkAssert(vkAllocateDescriptorSets(device, info, &bnd->set), "alloc desc set");

	bnd->writes.resize(bindings.size());
	bnd->bindData.resize(bindings.size());
	for (int i = 0; i < bindings.size(); i++)
	{
		auto& el = bnd->writes[i];
		el = {};
		el.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		el.pNext = nullptr;
		el.dstSet = bnd->set;
		el.descriptorCount = 1;
		el.descriptorType = bindings[i].descriptorType;
		el.dstArrayElement = 0;
		el.dstBinding = i;
	}
	return bnd;
}

void Binding::apply() 
{
	vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void Binding::setBuffer(int idx, const VulkanBuffer& buffer)
{
	assert(idx < writes.size());
	
	bindData[idx].bufferInfo.offset = 0;
	bindData[idx].bufferInfo.buffer = buffer.buffer;
	bindData[idx].bufferInfo.range = buffer.size;
	writes[idx].pBufferInfo = &bindData[idx].bufferInfo;
}
