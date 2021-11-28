#include "RenderGraph.h"
#include "HashUtils.h"

#include <algorithm>

namespace scapes::visual
{
	/*
	 */
	RenderGraph *RenderGraph::create(foundation::render::Device *device, foundation::game::World *world)
	{
		return new RenderGraphImpl(device, world);
	}

	void RenderGraph::destroy(RenderGraph *RenderGraph)
	{
		delete RenderGraph;
	}

	/*
	 */
	void *ParameterAllocator::allocate(size_t size)
	{
		return malloc(size);
	}

	void ParameterAllocator::deallocate(void *memory)
	{
		free(memory);
	}

	/*
	 */
	RenderGraphImpl::RenderGraphImpl(foundation::render::Device *device, foundation::game::World *world)
		: device(device), world(world)
	{

	}

	RenderGraphImpl::~RenderGraphImpl()
	{
		shutdown();

		removeAllParameterGroups();
		removeAllParameters();
		removeAllTextures();
		removeAllRenderPasses();
	}

	/*
	 */
	void RenderGraphImpl::init(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		bool should_invalidate = false;

		for (auto &[hash, group] : parameter_group_lookup)
		{
			bool result = flushParameterGroup(group);
			should_invalidate = should_invalidate || result;
		}

		for (auto &[hash, texture] : texture_lookup)
		{
			bool result = flushTextureResource(texture);
			should_invalidate = should_invalidate || result;
		}

		for (IRenderPass *pass : passes)
			pass->init(this);

		if (should_invalidate)
			for (IRenderPass *pass : passes)
				pass->invalidate();

	}

	void RenderGraphImpl::shutdown()
	{
		for (IRenderPass *pass : passes)
			pass->shutdown();

		for (auto &[hash, group] : parameter_group_lookup)
			invalidateParameterGroup(group);

		for (auto &[hash, texture] : texture_lookup)
			invalidateTextureResource(texture);
	}

	/*
	 */
	void RenderGraphImpl::resize(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		for (auto &[hash, texture] : texture_lookup)
		{
			invalidateTextureResource(texture);
			flushTextureResource(texture);
		}

		for (IRenderPass *pass : passes)
			pass->invalidate();
	}

	void RenderGraphImpl::render(foundation::render::CommandBuffer *command_buffer)
	{
		bool should_invalidate = false;

		for (auto &[hash, group] : parameter_group_lookup)
		{
			bool result = flushParameterGroup(group);
			should_invalidate = should_invalidate || result;
		}

		for (auto &[hash, texture] : texture_lookup)
		{
			bool result = flushTextureResource(texture);
			should_invalidate = should_invalidate || result;
		}

		if (should_invalidate)
			for (IRenderPass *pass : passes)
				pass->invalidate();

		for (IRenderPass *pass : passes)
			pass->render(command_buffer);
	}

	/*
	 */
	bool RenderGraphImpl::load(const foundation::io::URI &uri)
	{
		// TODO: implement
		return false;
	}

	bool RenderGraphImpl::save(const foundation::io::URI &uri)
	{
		// TODO: implement
		return false;
	}

	/*
	 */
	bool RenderGraphImpl::deserialize(const foundation::json::Document &document)
	{
		// TODO: implement
		return false;
	}

	foundation::json::Document RenderGraphImpl::serialize()
	{
		// TODO: implement
		return foundation::json::Document();
	}

	/*
	 */
	void RenderGraphImpl::addParameterGroup(const char *name)
	{
		ParameterGroup *group = new ParameterGroup();

		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		parameter_group_lookup.insert({hash, group});
	}

	bool RenderGraphImpl::removeParameterGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = parameter_group_lookup.find(hash);
		if (it == parameter_group_lookup.end())
			return false;

		ParameterGroup *group = it->second;
		parameter_group_lookup.erase(it);

		destroyParameterGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllParameterGroups()
	{
		for (auto &[hash, group] : parameter_group_lookup)
			destroyParameterGroup(group);

		parameter_group_lookup.clear();
	}

	foundation::render::BindSet *RenderGraphImpl::getParameterGroupBindings(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = parameter_group_lookup.find(hash);
		if (it == parameter_group_lookup.end())
			return false;

		const ParameterGroup *group = it->second;
		return group->bindings;
	}

	/*
	 */
	bool RenderGraphImpl::addParameter(const char *group_name, const char *parameter_name, size_t size)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto it = parameter_group_lookup.find(group_hash);
		if (it == parameter_group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		ParameterGroup *group = it->second;

		Parameter *parameter = new Parameter();
		parameter->group = group;
		parameter->size = size;
		parameter->memory = parameter_allocator.allocate(size);

		group->parameters.push_back(parameter);
		parameter_lookup.insert({parameter_hash, parameter});

		invalidateParameterGroup(group);

		return true;
	}

	bool RenderGraphImpl::removeParameter(const char *group_name, const char *parameter_name)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = parameter_group_lookup.find(group_hash);
		if (group_it == parameter_group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		auto parameter_it = parameter_lookup.find(parameter_hash);
		if (parameter_it == parameter_lookup.end())
			return false;

		ParameterGroup *group = group_it->second;
		Parameter *parameter = parameter_it->second;

		parameter_lookup.erase(parameter_hash);

		auto it = std::find(group->parameters.begin(), group->parameters.end(), parameter);
		assert(it != group->parameters.end());

		group->parameters.erase(it);

		destroyParameter(parameter);
		invalidateParameterGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllParameters()
	{
		for (auto &[hash, parameter] : parameter_lookup)
			destroyParameter(parameter);

		parameter_lookup.clear();

		for (auto &[hash, group] : parameter_group_lookup)
		{
			group->parameters.clear();
			invalidateParameterGroup(group);
		}
	}

	/*
	 */
	size_t RenderGraphImpl::getParameterSize(const char *group_name, const char *parameter_name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = parameter_lookup.find(hash);
		if (it == parameter_lookup.end())
			return 0;

		Parameter *parameter = it->second;
		return parameter->size;
	}

	const void *RenderGraphImpl::getParameterValue(const char *group_name, const char *parameter_name, size_t offset) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = parameter_lookup.find(hash);
		if (it == parameter_lookup.end())
			return nullptr;

		Parameter *parameter = it->second;
		const uint8_t *data = reinterpret_cast<const uint8_t *>(parameter->memory);

		if (offset >= parameter->size)
			return nullptr;

		return data + offset;
	}

	bool RenderGraphImpl::setParameterValue(const char *group_name, const char *parameter_name, size_t dst_offset, size_t src_size, const void *src_data)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = parameter_group_lookup.find(group_hash);
		if (group_it == parameter_group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		auto parameter_it = parameter_lookup.find(parameter_hash);
		if (parameter_it == parameter_lookup.end())
			return false;

		ParameterGroup *group = group_it->second;
		Parameter *parameter = parameter_it->second;
		uint8_t *dst_data = reinterpret_cast<uint8_t *>(parameter->memory);

		if (dst_offset >= parameter->size)
			return false;

		if ((parameter->size - dst_offset) < src_size)
			return false;

		memcpy(dst_data + dst_offset, src_data, src_size);

		group->dirty = true;

		return true;
	}

	/*
	 */
	void RenderGraphImpl::addTextureRenderBuffer(const char *name, foundation::render::Format format, uint32_t downscale)
	{
		TextureResource *texture = new TextureResource();

		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		texture->type = TextureType::RENDER_BUFFER;
		texture->render_buffer_format = format;
		texture->render_buffer_downscale = std::max<uint32_t>(1, downscale);

		texture_lookup.insert({hash, texture});
	}

	void RenderGraphImpl::addTextureResource(const char *name, TextureHandle handle)
	{
		TextureResource *texture = new TextureResource();

		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		texture->type = TextureType::RESOURCE;
		texture->resource = handle;

		texture_lookup.insert({hash, texture});
	}

	void RenderGraphImpl::addTextureSwapChain(const char *name, foundation::render::SwapChain *swap_chain)
	{
		TextureResource *texture = new TextureResource();

		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		texture->type = TextureType::SWAP_CHAIN;
		texture->swap_chain = swap_chain;

		texture_lookup.insert({hash, texture});
	}

	/*
	 */
	bool RenderGraphImpl::removeTexture(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return false;

		TextureResource *texture = it->second;
		destroyTextureResource(texture);

		texture_lookup.erase(hash);
		return true;
	}

	void RenderGraphImpl::removeAllTextures()
	{
		for (auto &[hash, texture] : texture_lookup)
			destroyTextureResource(texture);

		texture_lookup.clear();
	}

	/*
	 */
	RenderGraph::TextureType RenderGraphImpl::getTextureType(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return TextureType::INVALID;

		TextureResource *texture = it->second;
		return texture->type;
	}

	foundation::render::Texture *RenderGraphImpl::getTexture(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return nullptr;

		TextureResource *texture = it->second;
		if (texture->type == TextureType::RENDER_BUFFER)
			return texture->render_buffer;


		if (texture->type == TextureType::RESOURCE)
			return texture->resource->gpu_data;

		return nullptr;
	}

	/*
	 */
	foundation::render::Texture *RenderGraphImpl::getTextureRenderBuffer(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return nullptr;

		TextureResource *texture = it->second;
		if (texture->type != TextureType::RENDER_BUFFER)
			return nullptr;

		return texture->render_buffer;
	}

	foundation::render::Format RenderGraphImpl::getTextureRenderBufferFormat(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return foundation::render::Format::UNDEFINED;

		TextureResource *texture = it->second;
		if (texture->type != TextureType::RENDER_BUFFER)
			return foundation::render::Format::UNDEFINED;

		return texture->render_buffer_format;
	}

	uint32_t RenderGraphImpl::getTextureRenderBufferDownscale(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return 0;

		TextureResource *texture = it->second;
		if (texture->type != TextureType::RENDER_BUFFER)
			return 0;

		return texture->render_buffer_downscale;
	}

	/*
	 */
	TextureHandle RenderGraphImpl::getTextureResource(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return TextureHandle();

		TextureResource *texture = it->second;
		if (texture->type != TextureType::RESOURCE)
			return TextureHandle();

		return texture->resource;
	}

	bool RenderGraphImpl::setTextureResource(const char *name, TextureHandle handle)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return false;

		TextureResource *texture = it->second;
		if (texture->type != TextureType::RESOURCE)
			return false;

		texture->resource = handle;
		return true;
	}

	/*
	 */
	foundation::render::SwapChain *RenderGraphImpl::getTextureSwapChain(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return nullptr;

		TextureResource *texture = it->second;
		if (texture->type != TextureType::SWAP_CHAIN)
			return nullptr;

		return texture->swap_chain;
	}

	bool RenderGraphImpl::setTextureSwapChain(const char *name, foundation::render::SwapChain *swap_chain)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = texture_lookup.find(hash);
		if (it == texture_lookup.end())
			return false;

		TextureResource *texture = it->second;
		if (texture->type != TextureType::SWAP_CHAIN)
			return false;

		texture->swap_chain = swap_chain;
		return true;
	}

	/*
	 */
	void RenderGraphImpl::addRenderPass(IRenderPass *pass)
	{
		// TODO: how to init new render pass?
		passes.push_back(pass);
	}

	bool RenderGraphImpl::addRenderPass(IRenderPass *pass, size_t index)
	{
		if (index >= passes.size())
			return false;

		// TODO: how to init new render pass?
		passes.insert(passes.begin() + index, pass);

		return true;
	}

	bool RenderGraphImpl::removeRenderPass(size_t index)
	{
		if (index >= passes.size())
			return false;

		IRenderPass *pass = passes[index];
		pass->shutdown();

		passes.erase(passes.begin() + index);
		return true;
	}

	void RenderGraphImpl::removeAllRenderPasses()
	{
		for (IRenderPass *pass : passes)
			pass->shutdown();

		passes.clear();
	}

	/*
	 */
	bool RenderGraphImpl::registerRenderPassType(const char *name, PFN_createRenderPass function)
	{
		// TODO:
		return false;
	}

	/*
	 */
	void RenderGraphImpl::destroyParameterGroup(RenderGraphImpl::ParameterGroup *group)
	{
		assert(group);

		invalidateParameterGroup(group);

		for (Parameter *parameter : group->parameters)
			parameter->group = nullptr;

		delete group;
	}

	void RenderGraphImpl::invalidateParameterGroup(RenderGraphImpl::ParameterGroup *group)
	{
		assert(group);

		// TODO: make sure UBO + BindSet recreated only if
		// current UBO size is not enough to fit all parameters
		device->destroyUniformBuffer(group->gpu_resource);
		device->destroyBindSet(group->bindings);

		group->gpu_resource = nullptr;
		group->bindings = nullptr;

		group->dirty = true;
	}

	bool RenderGraphImpl::flushParameterGroup(RenderGraphImpl::ParameterGroup *group)
	{
		assert(group);

		if (!group->dirty)
			return true;

		bool should_invalidate = false;

		uint32_t current_offset = 0;
		uint32_t ubo_size = 0;
		constexpr uint32_t alignment = 16;

		for (Parameter *parameter : group->parameters)
		{
			uint32_t padding = alignment - current_offset % alignment;
			if (current_offset > 0 && current_offset + parameter->size > alignment)
				ubo_size += padding;

			ubo_size += static_cast<uint32_t>(parameter->size);
			current_offset = (current_offset + parameter->size) % alignment;
		}

		if (group->gpu_resource_size < ubo_size)
		{
			device->destroyUniformBuffer(group->gpu_resource);
			device->destroyBindSet(group->bindings);

			group->gpu_resource_size = ubo_size;
			group->gpu_resource = device->createUniformBuffer(foundation::render::BufferType::DYNAMIC, ubo_size);

			should_invalidate = true;
		}

		uint8_t *ubo_data = reinterpret_cast<uint8_t *>(device->map(group->gpu_resource));

		current_offset = 0;
		for (Parameter *parameter : group->parameters)
		{
			size_t padding = alignment - current_offset % alignment;
			if (current_offset > 0 && current_offset + parameter->size > alignment)
				ubo_data += padding;

			memcpy(ubo_data, parameter->memory, parameter->size);

			ubo_data += parameter->size;
			current_offset = (current_offset + parameter->size) % alignment;
		}

		device->unmap(group->gpu_resource);

		if (group->bindings == nullptr)
		{
			group->bindings = device->createBindSet();
			device->bindUniformBuffer(group->bindings, 0, group->gpu_resource);

			should_invalidate = true;
		}

		group->dirty = false;
		return should_invalidate;
	}

	/*
	 */
	void RenderGraphImpl::destroyParameter(RenderGraphImpl::Parameter *parameter)
	{
		assert(parameter);

		parameter_allocator.deallocate(parameter->memory);
		delete parameter;
	}

	/*
	 */
	void RenderGraphImpl::destroyTextureResource(RenderGraphImpl::TextureResource *texture)
	{
		assert(texture);

		invalidateTextureResource(texture);

		delete texture;
	}

	void RenderGraphImpl::invalidateTextureResource(RenderGraphImpl::TextureResource *texture)
	{
		assert(texture);

		device->destroyTexture(texture->render_buffer);
		texture->render_buffer = nullptr;
	}

	bool RenderGraphImpl::flushTextureResource(RenderGraphImpl::TextureResource *texture)
	{
		assert(texture);

		if (texture->type != TextureType::RENDER_BUFFER)
			return false;

		if (texture->render_buffer)
			return false;

		foundation::render::Format format = texture->render_buffer_format;
		uint32_t texture_width = std::max<uint32_t>(1, width / texture->render_buffer_downscale);
		uint32_t texture_height = std::max<uint32_t>(1, height / texture->render_buffer_downscale);

		texture->render_buffer = device->createTexture2D(texture_width, texture_height, 1, format);
		return true;
	}
}
