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

		removeAllGroups();
		removeAllGroupParameters();
		removeAllGroupTextures();
		removeAllRenderBuffers();
		removeAllRenderPasses();

		render_pass_type_lookup.clear();
	}

	/*
	 */
	void RenderGraphImpl::init(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		bool should_invalidate = false;

		for (auto &[hash, group] : group_lookup)
		{
			bool result = flushGroup(group);
			should_invalidate = should_invalidate || result;
		}

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			bool result = flushRenderBuffer(render_buffer);
			should_invalidate = should_invalidate || result;
		}

		for (IRenderPass *pass : passes)
			pass->init();

		if (should_invalidate)
			for (IRenderPass *pass : passes)
				pass->invalidate();

	}

	void RenderGraphImpl::shutdown()
	{
		for (IRenderPass *pass : passes)
			pass->shutdown();

		for (auto &[hash, group] : group_lookup)
			invalidateGroup(group);

		for (auto &[hash, render_buffer] : render_buffer_lookup)
			invalidateRenderBuffer(render_buffer);

		invalidateFrameBufferCache();
	}

	/*
	 */
	void RenderGraphImpl::resize(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			invalidateRenderBuffer(render_buffer);
			flushRenderBuffer(render_buffer);
		}

		invalidateFrameBufferCache();

		for (IRenderPass *pass : passes)
			pass->invalidate();
	}

	void RenderGraphImpl::render(foundation::render::CommandBuffer *command_buffer)
	{
		bool should_invalidate = false;

		for (auto &[hash, group] : group_lookup)
		{
			bool result = flushGroup(group);
			should_invalidate = should_invalidate || result;
		}

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			bool result = flushRenderBuffer(render_buffer);
			should_invalidate = should_invalidate || result;
		}

		if (should_invalidate)
		{
			scapes::foundation::Log::message("Invalidation before render\n");
			for (IRenderPass *pass : passes)
				pass->invalidate();
		}

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
	bool RenderGraphImpl::addGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		if (group_lookup.find(hash) != group_lookup.end())
			return false;

		Group *group = new Group();
		group_lookup.insert({hash, group});

		return true;
	}

	bool RenderGraphImpl::removeGroup(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = group_lookup.find(hash);
		if (it == group_lookup.end())
			return false;

		Group *group = it->second;
		group_lookup.erase(it);

		destroyGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllGroups()
	{
		for (auto &[hash, group] : group_lookup)
			destroyGroup(group);

		group_lookup.clear();
	}

	foundation::render::BindSet *RenderGraphImpl::getGroupBindings(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = group_lookup.find(hash);
		if (it == group_lookup.end())
			return false;

		const Group *group = it->second;
		return group->bindings;
	}

	/*
	 */
	bool RenderGraphImpl::addGroupParameter(const char *group_name, const char *parameter_name, size_t size)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto it = group_lookup.find(group_hash);
		if (it == group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		if (group_parameter_lookup.find(parameter_hash) != group_parameter_lookup.end())
			return false;

		Group *group = it->second;

		GroupParameter *parameter = new GroupParameter();
		parameter->group = group;
		parameter->size = size;
		parameter->memory = parameter_allocator.allocate(size);

		group->parameters.push_back(parameter);
		group_parameter_lookup.insert({parameter_hash, parameter});

		invalidateGroup(group);

		return true;
	}

	bool RenderGraphImpl::removeGroupParameter(const char *group_name, const char *parameter_name)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		auto parameter_it = group_parameter_lookup.find(parameter_hash);
		if (parameter_it == group_parameter_lookup.end())
			return false;

		Group *group = group_it->second;
		GroupParameter *parameter = parameter_it->second;

		group_parameter_lookup.erase(parameter_hash);

		auto it = std::find(group->parameters.begin(), group->parameters.end(), parameter);
		assert(it != group->parameters.end());

		group->parameters.erase(it);

		destroyGroupParameter(parameter);
		invalidateGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllGroupParameters()
	{
		for (auto &[hash, parameter] : group_parameter_lookup)
			destroyGroupParameter(parameter);

		group_parameter_lookup.clear();

		for (auto &[hash, group] : group_lookup)
		{
			group->parameters.clear();
			invalidateGroup(group);
		}
	}

	/*
	 */
	bool RenderGraphImpl::addGroupTexture(const char *group_name, const char *texture_name)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto it = group_lookup.find(group_hash);
		if (it == group_lookup.end())
			return false;

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		if (group_texture_lookup.find(texture_hash) != group_texture_lookup.end())
			return false;

		Group *group = it->second;

		GroupTexture *texture = new GroupTexture();
		texture->group = group;

		group->textures.push_back(texture);
		group_texture_lookup.insert({texture_hash, texture});

		invalidateGroup(group);

		return true;
	}

	bool RenderGraphImpl::removeGroupTexture(const char *group_name, const char *texture_name)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		auto texture_it = group_texture_lookup.find(texture_hash);
		if (texture_it == group_texture_lookup.end())
			return false;

		Group *group = group_it->second;
		GroupTexture *texture = texture_it->second;

		group_texture_lookup.erase(texture_hash);

		auto it = std::find(group->textures.begin(), group->textures.end(), texture);
		assert(it != group->textures.end());

		group->textures.erase(it);

		destroyGroupTexture(texture);
		invalidateGroup(group);

		return true;
	}

	void RenderGraphImpl::removeAllGroupTextures()
	{
		for (auto &[hash, texture] : group_texture_lookup)
			destroyGroupTexture(texture);

		group_texture_lookup.clear();

		for (auto &[hash, group] : group_lookup)
		{
			group->textures.clear();
			invalidateGroup(group);
		}
	}

	/*
	 */
	TextureHandle RenderGraphImpl::getGroupTexture(const char *group_name, const char *texture_name) const
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return TextureHandle();

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		auto texture_it = group_texture_lookup.find(texture_hash);
		if (texture_it == group_texture_lookup.end())
			return TextureHandle();

		GroupTexture *texture = texture_it->second;

		return texture->texture;
	}

	bool RenderGraphImpl::setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t texture_hash = group_hash;
		common::HashUtils::combine(texture_hash, std::string_view(texture_name));

		auto texture_it = group_texture_lookup.find(texture_hash);
		if (texture_it == group_texture_lookup.end())
			return false;

		GroupTexture *texture = texture_it->second;

		texture->texture = handle;
		return true;
	}

	/*
	 */
	bool RenderGraphImpl::addRenderBuffer(const char *name, foundation::render::Format format, uint32_t downscale)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		if (render_buffer_lookup.find(hash) != render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer = new RenderBuffer();

		render_buffer->format = format;
		render_buffer->downscale = std::max<uint32_t>(1, downscale);
		render_buffer->texture = nullptr;
		render_buffer->bindings = nullptr;

		render_buffer_lookup.insert({hash, render_buffer});
		return true;
	}

	bool RenderGraphImpl::removeRenderBuffer(const char *name)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer = it->second;
		destroyRenderBuffer(render_buffer);

		render_buffer_lookup.erase(hash);
		return true;
	}

	void RenderGraphImpl::removeAllRenderBuffers()
	{
		for (auto &[hash, render_buffer] : render_buffer_lookup)
			destroyRenderBuffer(render_buffer);

		render_buffer_lookup.clear();
	}

	bool RenderGraphImpl::swapRenderBuffers(const char *name0, const char *name1)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name0));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer0 = it->second;

		hash = 0;
		common::HashUtils::combine(hash, std::string_view(name1));

		it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer1 = it->second;

		if (render_buffer0->format != render_buffer1->format)
			return false;

		if (render_buffer0->downscale != render_buffer1->downscale)
			return false;

		std::swap(render_buffer0->texture, render_buffer1->texture);
		std::swap(render_buffer0->bindings, render_buffer1->bindings);

		return true;
	}

	/*
	 */
	foundation::render::Texture *RenderGraphImpl::getRenderBufferTexture(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return nullptr;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->texture;
	}

	foundation::render::BindSet *RenderGraphImpl::getRenderBufferBindings(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return nullptr;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->bindings;
	}

	foundation::render::Format RenderGraphImpl::getRenderBufferFormat(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return foundation::render::Format::UNDEFINED;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->format;
	}

	uint32_t RenderGraphImpl::getRenderBufferDownscale(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return 0;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->downscale;
	}

	/*
	 */
	foundation::render::FrameBuffer *RenderGraphImpl::fetchFrameBuffer(uint32_t num_attachments, const char *render_buffer_names[])
	{
		uint64_t hash = 0;

		foundation::render::FrameBufferAttachment attachments[32];

		for (uint32_t i = 0; i < num_attachments; ++i)
		{
			foundation::render::FrameBufferAttachment &attachment = attachments[i];
			attachment.texture = getRenderBufferTexture(render_buffer_names[i]);
			attachment.base_layer = 0;
			attachment.base_mip = 0;
			attachment.num_layers = 1;

			common::HashUtils::combine(hash, attachment.texture);
			common::HashUtils::combine(hash, attachment.base_layer);
			common::HashUtils::combine(hash, attachment.base_mip);
			common::HashUtils::combine(hash, attachment.num_layers);
		}

		auto it = framebuffer_cache.find(hash);
		if (it != framebuffer_cache.end())
			return it->second;

		foundation::render::FrameBuffer *framebuffer = device->createFrameBuffer(num_attachments, attachments);
		framebuffer_cache.insert({hash, framebuffer});

		return framebuffer;
	}

	/*
	 */
	bool RenderGraphImpl::removeRenderPass(size_t index)
	{
		if (index >= passes.size())
			return false;

		IRenderPass *pass = passes[index];
		pass->shutdown();
		delete pass;

		passes.erase(passes.begin() + index);
		return true;
	}

	bool RenderGraphImpl::removeRenderPass(IRenderPass *pass)
	{
		auto it = std::find(passes.begin(), passes.end(), pass);
		if (it == passes.end())
			return false;

		size_t index = std::distance(passes.begin(), it);
		return removeRenderPass(index);
	}

	void RenderGraphImpl::removeAllRenderPasses()
	{
		for (IRenderPass *pass : passes)
		{
			pass->shutdown();
			delete pass;
		}

		passes.clear();
	}

	/*
	 */
	size_t RenderGraphImpl::getGroupParameterSize(const char *group_name, const char *parameter_name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = group_parameter_lookup.find(hash);
		if (it == group_parameter_lookup.end())
			return 0;

		GroupParameter *parameter = it->second;
		return parameter->size;
	}

	const void *RenderGraphImpl::getGroupParameter(const char *group_name, const char *parameter_name, size_t offset) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(group_name));
		common::HashUtils::combine(hash, std::string_view(parameter_name));

		auto it = group_parameter_lookup.find(hash);
		if (it == group_parameter_lookup.end())
			return nullptr;

		GroupParameter *parameter = it->second;
		const uint8_t *data = reinterpret_cast<const uint8_t *>(parameter->memory);

		if (offset >= parameter->size)
			return nullptr;

		return data + offset;
	}

	bool RenderGraphImpl::setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_offset, size_t src_size, const void *src_data)
	{
		uint64_t group_hash = 0;
		common::HashUtils::combine(group_hash, std::string_view(group_name));

		auto group_it = group_lookup.find(group_hash);
		if (group_it == group_lookup.end())
			return false;

		uint64_t parameter_hash = group_hash;
		common::HashUtils::combine(parameter_hash, std::string_view(parameter_name));

		auto parameter_it = group_parameter_lookup.find(parameter_hash);
		if (parameter_it == group_parameter_lookup.end())
			return false;

		Group *group = group_it->second;
		GroupParameter *parameter = parameter_it->second;
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
	bool RenderGraphImpl::registerRenderPass(const char *name, PFN_createRenderPass function)
	{
		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		auto it = render_pass_type_lookup.find(render_pass_hash);
		if (it != render_pass_type_lookup.end())
			return false;

		render_pass_type_lookup.insert({render_pass_hash, function});
		return true;
	}

	IRenderPass *RenderGraphImpl::createRenderPass(const char *name)
	{
		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		auto it = render_pass_type_lookup.find(render_pass_hash);
		if (it == render_pass_type_lookup.end())
			return nullptr;

		PFN_createRenderPass function = it->second;
		assert(function);

		IRenderPass *render_pass = function(this);
		assert(render_pass);

		passes.push_back(render_pass);
		return render_pass;
	}

	/*
	 */
	void RenderGraphImpl::destroyGroup(RenderGraphImpl::Group *group)
	{
		assert(group);

		invalidateGroup(group);

		for (GroupParameter *parameter : group->parameters)
			parameter->group = nullptr;

		for (GroupTexture *texture : group->textures)
			texture->group = nullptr;

		delete group;
	}

	void RenderGraphImpl::invalidateGroup(RenderGraphImpl::Group *group)
	{
		assert(group);

		// TODO: make sure UBO + BindSet recreated only if
		// current UBO size is not enough to fit all parameters
		device->destroyUniformBuffer(group->buffer);
		device->destroyBindSet(group->bindings);

		group->buffer = nullptr;
		group->buffer_size = 0;
		group->bindings = nullptr;

		group->dirty = true;
	}

	bool RenderGraphImpl::flushGroup(RenderGraphImpl::Group *group)
	{
		assert(group);

		if (!group->dirty)
			return false;

		bool should_invalidate = false;

		uint32_t current_offset = 0;
		uint32_t ubo_size = 0;
		constexpr uint32_t alignment = 16;

		for (GroupParameter *parameter : group->parameters)
		{
			uint32_t padding = alignment - current_offset % alignment;
			if (current_offset > 0 && current_offset + parameter->size > alignment)
				ubo_size += padding;

			ubo_size += static_cast<uint32_t>(parameter->size);
			current_offset = (current_offset + parameter->size) % alignment;
		}

		if (group->buffer_size < ubo_size)
		{
			device->destroyUniformBuffer(group->buffer);
			device->destroyBindSet(group->bindings);

			group->buffer_size = ubo_size;
			group->buffer = device->createUniformBuffer(foundation::render::BufferType::DYNAMIC, ubo_size);

			should_invalidate = true;
		}

		if (group->buffer_size > 0)
		{
			uint8_t *ubo_data = reinterpret_cast<uint8_t *>(device->map(group->buffer));

			current_offset = 0;
			for (GroupParameter *parameter : group->parameters)
			{
				size_t padding = alignment - current_offset % alignment;
				if (current_offset > 0 && current_offset + parameter->size > alignment)
					ubo_data += padding;

				memcpy(ubo_data, parameter->memory, parameter->size);

				ubo_data += parameter->size;
				current_offset = (current_offset + parameter->size) % alignment;
			}

			device->unmap(group->buffer);
		}

		if (group->bindings == nullptr)
		{
			group->bindings = device->createBindSet();
			should_invalidate = true;
		}

		uint32_t binding = 0;
		if (group->buffer)
			device->bindUniformBuffer(group->bindings, binding++, group->buffer);

		for (GroupTexture *texture : group->textures)
			device->bindTexture(group->bindings, binding++, texture->texture->gpu_data);

		group->dirty = false;
		return should_invalidate;
	}

	/*
	 */
	void RenderGraphImpl::destroyGroupParameter(RenderGraphImpl::GroupParameter *parameter)
	{
		assert(parameter);

		parameter_allocator.deallocate(parameter->memory);
		delete parameter;
	}

	void RenderGraphImpl::destroyGroupTexture(RenderGraphImpl::GroupTexture *texture)
	{
		assert(texture);

		delete texture;
	}

	/*
	 */
	void RenderGraphImpl::destroyRenderBuffer(RenderGraphImpl::RenderBuffer *buffer)
	{
		assert(buffer);

		invalidateRenderBuffer(buffer);

		delete buffer;
	}

	void RenderGraphImpl::invalidateRenderBuffer(RenderGraphImpl::RenderBuffer *buffer)
	{
		assert(buffer);

		device->destroyTexture(buffer->texture);
		buffer->texture = nullptr;

		device->destroyBindSet(buffer->bindings);
		buffer->bindings = nullptr;
	}

	bool RenderGraphImpl::flushRenderBuffer(RenderGraphImpl::RenderBuffer *texture)
	{
		assert(texture);

		if (texture->texture)
			return false;

		assert(texture->bindings == nullptr);

		foundation::render::Format format = texture->format;
		uint32_t texture_width = std::max<uint32_t>(1, width / texture->downscale);
		uint32_t texture_height = std::max<uint32_t>(1, height / texture->downscale);

		texture->texture = device->createTexture2D(texture_width, texture_height, 1, format);
		texture->bindings = device->createBindSet();

		device->bindTexture(texture->bindings, 0, texture->texture);
		return true;
	}

	/*
	 */
	void RenderGraphImpl::invalidateFrameBufferCache()
	{
		for (auto &[hash, framebuffer] : framebuffer_cache)
			device->destroyFrameBuffer(framebuffer);

		framebuffer_cache.clear();
	}
}
