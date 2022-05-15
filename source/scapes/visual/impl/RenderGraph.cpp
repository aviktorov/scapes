#include "RenderGraph.h"
#include <HashUtils.h>

#include <scapes/foundation/io/FileSystem.h>

#include <algorithm>

namespace yaml = scapes::foundation::serde::yaml;
namespace math = scapes::foundation::math;

namespace scapes::visual::impl
{
	/*
	 */
	RenderGraph::RenderGraph(
		foundation::resources::ResourceManager *resource_manager,
		hardware::Device *device,
		shaders::Compiler *compiler,
		foundation::game::World *world,
		MeshHandle unit_quad
	)
		: resource_manager(resource_manager), device(device), compiler(compiler), world(world), unit_quad(unit_quad), gpu_bindings(resource_manager, device)
	{

	}

	RenderGraph::~RenderGraph()
	{
		shutdown();

		removeAllGroups();
		removeAllRenderBuffers();
		removeAllRenderPasses();

		pass_types_runtime.constructors.clear();
		pass_types_runtime.name_hashes.clear();
		pass_types_runtime.names.clear();
	}

	/*
	 */
	void RenderGraph::init(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		bool should_invalidate = false;

		bool result = gpu_bindings.flush();
		should_invalidate = should_invalidate || result;

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			bool result = flushRenderBuffer(render_buffer);
			should_invalidate = should_invalidate || result;
		}

		for (IRenderPass *pass : passes_runtime.passes)
			pass->init();

		if (should_invalidate)
			for (IRenderPass *pass : passes_runtime.passes)
				pass->invalidate();

	}

	void RenderGraph::shutdown()
	{
		for (IRenderPass *pass : passes_runtime.passes)
			pass->shutdown();

		gpu_bindings.invalidate();

		for (auto &[hash, render_buffer] : render_buffer_lookup)
			invalidateRenderBuffer(render_buffer);

		invalidateFrameBufferCache();
	}

	/*
	 */
	void RenderGraph::resize(uint32_t w, uint32_t h)
	{
		width = w;
		height = h;

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			invalidateRenderBuffer(render_buffer);
			flushRenderBuffer(render_buffer);
		}

		invalidateFrameBufferCache();

		for (IRenderPass *pass : passes_runtime.passes)
			pass->invalidate();
	}

	void RenderGraph::render(hardware::CommandBuffer command_buffer)
	{
		bool should_invalidate = false;

		bool result = gpu_bindings.flush();
		should_invalidate = should_invalidate || result;

		for (auto &[hash, render_buffer] : render_buffer_lookup)
		{
			bool result = flushRenderBuffer(render_buffer);
			should_invalidate = should_invalidate || result;
		}

		if (should_invalidate)
		{
			scapes::foundation::Log::message("Invalidation before render\n");
			for (IRenderPass *pass : passes_runtime.passes)
				pass->invalidate();
		}

		for (IRenderPass *pass : passes_runtime.passes)
			pass->render(command_buffer);
	}

	/*
	 */
	bool RenderGraph::load(const foundation::io::URI &uri)
	{
		foundation::io::FileSystem *file_system = resource_manager->getFileSystem();
		assert(file_system);

		size_t size = 0;
		uint8_t *data = reinterpret_cast<uint8_t *>(file_system->map(uri, size));

		if (!data)
		{
			foundation::Log::error("RenderGraph::load(): can't open \"%s\" file\n", uri);
			return false;
		}

		yaml::csubstr yaml(reinterpret_cast<const char *>(data), size);
		yaml::Tree &tree = yaml::parse(yaml);
		bool result = deserialize(tree);

		file_system->unmap(data);

		return result;
	}

	bool RenderGraph::save(const foundation::io::URI &uri)
	{
		foundation::io::FileSystem *file_system = resource_manager->getFileSystem();
		assert(file_system);

		foundation::io::Stream *file = file_system->open(uri, "wb");
		if (!file)
		{
			foundation::Log::error("RenderGraph::save(): can't open \"%s\" file\n", uri);
			return false;
		}

		const yaml::Tree &tree = serialize();
		yaml::csubstr output = yaml::emit(tree, tree.root_id(), yaml::substr{}, false);

		std::vector<char> data;
		data.resize(output.len);
		yaml::emit(tree, tree.root_id(), yaml::substr(data.data(), data.size()));

		size_t bytes_written = file->write(data.data(), sizeof(char), output.len);
		file_system->close(file);

		return bytes_written == output.len;
	}

	/*
	 */
	bool RenderGraph::deserialize(const yaml::Tree &tree)
	{
		// TODO: split this into multiple methods for the sake of readability
		const yaml::NodeRef stream = tree.rootref();
		if (!stream.is_root())
		{
			foundation::Log::error("RenderGraph::deserialize(): can't get root node\n");
			return false;
		}

		if (!stream.is_stream())
		{
			foundation::Log::error("RenderGraph::deserialize(): root node is not a stream\n");
			return false;
		}

		if (stream.is_doc())
		{
			foundation::Log::error("RenderGraph::deserialize(): root node must not be a document\n");
			return false;
		}

		if (!gpu_bindings.deserialize(stream))
		{
			foundation::Log::error("RenderGraph::deserialize(): can't deserialize parameter groups\n");
			return false;
		}

		for (const yaml::NodeRef document : stream.children())
		{
			if (!document.is_doc())
			{
				foundation::Log::error("RenderGraph::deserialize(): child node is not a document\n");
				return false;
			}

			for (const yaml::NodeRef child : document.children())
			{
				yaml::csubstr child_key = child.key();

				if (child_key.compare("RenderBuffers") == 0)
					deserializeRenderBuffers(child);

				else if (child_key.compare("RenderPass") == 0)
					deserializeRenderPass(child);
			}
		}

		return true;
	}

	yaml::Tree RenderGraph::serialize()
	{
		yaml::Tree tree;
		yaml::NodeRef root = tree.rootref();
		root |= yaml::STREAM;

		gpu_bindings.serialize(root);

		// serialize all render buffers
		if (render_buffer_lookup.size() > 0)
		{
			yaml::NodeRef child = root.append_child();
			child |= yaml::DOCMAP;

			child = child.append_child();
			child |= yaml::SEQ;

			child.set_key("RenderBuffers");

			for (auto &[hash, render_buffer] : render_buffer_lookup)
			{
				yaml::NodeRef render_buffer_node = child.append_child();
				render_buffer_node |= yaml::MAP;

				render_buffer_node["name"] << render_buffer->name.c_str();
				render_buffer_node["format"] << render_buffer->format;

				if (render_buffer->downscale != 1)
					render_buffer_node["downscale"] << render_buffer->downscale;
			}
		}

		// serialize all render passes
		for (size_t i = 0; i < passes_runtime.passes.size(); ++i)
		{
			yaml::NodeRef child = root.append_child();

			child.set_type(yaml::DOCMAP);

			child = child.append_child();
			child |= yaml::MAP;

			child.set_key("RenderPass");

			child["name"] << passes_runtime.names[i].c_str();
			child["type"] << passes_runtime.type_names[i].c_str();

			IRenderPass *pass = passes_runtime.passes[i];
			if (!pass->serialize(child))
				foundation::Log::error("Can't serialize render pass with type \"%s\"\n", passes_runtime.type_names[i].c_str());
		}

		return tree;
	}

	/*
	 */
	bool RenderGraph::addGroup(const char *name)
	{
		return gpu_bindings.addGroup(name);
	}

	bool RenderGraph::removeGroup(const char *name)
	{
		return gpu_bindings.removeGroup(name);
	}

	void RenderGraph::removeAllGroups()
	{
		gpu_bindings.clear();
	}

	hardware::BindSet RenderGraph::getGroupBindings(const char *name) const
	{
		return gpu_bindings.getGroupBindings(name);
	}

	/*
	 */
	bool RenderGraph::addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements)
	{
		return gpu_bindings.addGroupParameter(group_name, parameter_name, type, num_elements);
	}

	bool RenderGraph::addGroupParameter(const char *group_name, const char *parameter_name, size_t element_size, size_t num_elements)
	{
		return gpu_bindings.addGroupParameter(group_name, parameter_name, element_size, num_elements);
	}

	bool RenderGraph::removeGroupParameter(const char *group_name, const char *parameter_name)
	{
		return gpu_bindings.removeGroupParameter(group_name, parameter_name);
	}

	/*
	 */
	bool RenderGraph::addGroupTexture(const char *group_name, const char *texture_name)
	{
		return gpu_bindings.addGroupTexture(group_name, texture_name);
	}

	bool RenderGraph::removeGroupTexture(const char *group_name, const char *texture_name)
	{
		return gpu_bindings.removeGroupTexture(group_name, texture_name);
	}

	/*
	 */
	TextureHandle RenderGraph::getGroupTexture(const char *group_name, const char *texture_name) const
	{
		return gpu_bindings.getGroupTexture(group_name, texture_name);
	}

	bool RenderGraph::setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle)
	{
		return gpu_bindings.setGroupTexture(group_name, texture_name, handle);
	}

	/*
	 */
	bool RenderGraph::addRenderBuffer(const char *name, hardware::Format format, uint32_t downscale)
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		if (render_buffer_lookup.find(hash) != render_buffer_lookup.end())
			return false;

		RenderBuffer *render_buffer = new RenderBuffer();

		render_buffer->name = std::string(name);
		render_buffer->format = format;
		render_buffer->downscale = std::max<uint32_t>(1, downscale);
		render_buffer->texture = nullptr;
		render_buffer->bindings = nullptr;

		render_buffer_lookup.insert({hash, render_buffer});
		return true;
	}

	bool RenderGraph::removeRenderBuffer(const char *name)
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

	void RenderGraph::removeAllRenderBuffers()
	{
		for (auto &[hash, render_buffer] : render_buffer_lookup)
			destroyRenderBuffer(render_buffer);

		render_buffer_lookup.clear();
	}

	bool RenderGraph::swapRenderBuffers(const char *name0, const char *name1)
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
	hardware::Texture RenderGraph::getRenderBufferTexture(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return nullptr;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->texture;
	}

	hardware::BindSet RenderGraph::getRenderBufferBindings(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return nullptr;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->bindings;
	}

	hardware::Format RenderGraph::getRenderBufferFormat(const char *name) const
	{
		uint64_t hash = 0;
		common::HashUtils::combine(hash, std::string_view(name));

		auto it = render_buffer_lookup.find(hash);
		if (it == render_buffer_lookup.end())
			return hardware::Format::UNDEFINED;

		RenderBuffer *render_buffer = it->second;
		return render_buffer->format;
	}

	uint32_t RenderGraph::getRenderBufferDownscale(const char *name) const
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
	hardware::FrameBuffer RenderGraph::fetchFrameBuffer(uint32_t num_attachments, const char *render_buffer_names[])
	{
		uint64_t hash = 0;

		hardware::FrameBufferAttachment attachments[32];

		for (uint32_t i = 0; i < num_attachments; ++i)
		{
			hardware::FrameBufferAttachment &attachment = attachments[i];
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

		hardware::FrameBuffer framebuffer = device->createFrameBuffer(num_attachments, attachments);
		framebuffer_cache.insert({hash, framebuffer});

		return framebuffer;
	}

	/*
	 */
	IRenderPass *RenderGraph::getRenderPass(const char *name) const
	{
		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		for (size_t i = 0; i < passes_runtime.name_hashes.size(); ++i)
			if (passes_runtime.name_hashes[i] == render_pass_hash)
				return passes_runtime.passes[i];

		return nullptr;
	}

	bool RenderGraph::removeRenderPass(size_t index)
	{
		if (index >= passes_runtime.passes.size())
			return false;

		IRenderPass *pass = passes_runtime.passes[index];

		pass->shutdown();
		delete pass;

		passes_runtime.passes.erase(passes_runtime.passes.begin() + index);
		passes_runtime.name_hashes.erase(passes_runtime.name_hashes.begin() + index);
		passes_runtime.type_hashes.erase(passes_runtime.type_hashes.begin() + index);

		return true;
	}

	bool RenderGraph::removeRenderPass(IRenderPass *pass)
	{
		auto it = std::find(passes_runtime.passes.begin(), passes_runtime.passes.end(), pass);
		if (it == passes_runtime.passes.end())
			return false;

		size_t index = std::distance(passes_runtime.passes.begin(), it);
		return removeRenderPass(index);
	}

	void RenderGraph::removeAllRenderPasses()
	{
		for (size_t i = 0; i < passes_runtime.passes.size(); ++i)
		{
			IRenderPass *pass = passes_runtime.passes[i];
			pass->shutdown();
			delete pass;
		}

		passes_runtime.passes.clear();
		passes_runtime.name_hashes.clear();
		passes_runtime.type_hashes.clear();
		passes_runtime.names.clear();
		passes_runtime.type_names.clear();
	}

	/*
	 */
	size_t RenderGraph::getGroupParameterElementSize(const char *group_name, const char *parameter_name) const
	{
		return gpu_bindings.getGroupParameterElementSize(group_name, parameter_name);
	}

	size_t RenderGraph::getGroupParameterNumElements(const char *group_name, const char *parameter_name) const
	{
		return gpu_bindings.getGroupParameterNumElements(group_name, parameter_name);
	}

	const void *RenderGraph::getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const
	{
		return gpu_bindings.getGroupParameter(group_name, parameter_name, index);
	}

	bool RenderGraph::setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data)
	{
		return gpu_bindings.setGroupParameter(group_name, parameter_name, dst_index, num_src_elements, src_data);
	}

	/*
	 */
	bool RenderGraph::registerRenderPassType(const char *type_name, PFN_createRenderPass function)
	{
		uint64_t render_pass_type_hash = 0;
		common::HashUtils::combine(render_pass_type_hash, std::string_view(type_name));

		for (size_t i = 0; i < pass_types_runtime.name_hashes.size(); ++i)
			if (pass_types_runtime.name_hashes[i] == render_pass_type_hash)
				return false;

		pass_types_runtime.constructors.push_back(function);
		pass_types_runtime.name_hashes.push_back(render_pass_type_hash);
		pass_types_runtime.names.push_back(std::string(type_name));

		return true;
	}

	IRenderPass *RenderGraph::createRenderPass(const char *type_name, const char *name)
	{
		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		for (size_t i = 0; i < passes_runtime.name_hashes.size(); ++i)
			if (passes_runtime.name_hashes[i] == render_pass_hash)
				return nullptr;

		uint64_t render_pass_type_hash = 0;
		common::HashUtils::combine(render_pass_type_hash, std::string_view(type_name));

		int32_t type_index = findRenderPassType(render_pass_type_hash);

		if (type_index == -1)
			return nullptr;

		IRenderPass *pass = pass_types_runtime.constructors[type_index](this);

		passes_runtime.passes.push_back(pass);
		passes_runtime.name_hashes.push_back(render_pass_hash);
		passes_runtime.type_hashes.push_back(render_pass_type_hash);
		passes_runtime.names.push_back(std::string(name));
		passes_runtime.type_names.push_back(std::string(type_name));

		return pass;
	}

	int32_t RenderGraph::findRenderPass(const char *name)
	{
		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		return findRenderPass(render_pass_hash);
	}

	int32_t RenderGraph::findRenderPass(uint64_t hash)
	{
		for (size_t i = 0; i < passes_runtime.name_hashes.size(); ++i)
			if (passes_runtime.name_hashes[i] == hash)
				return static_cast<int32_t>(i);

		return -1;
	}

	int32_t RenderGraph::findRenderPassType(const char *type_name)
	{
		uint64_t render_pass_type_hash = 0;
		common::HashUtils::combine(render_pass_type_hash, std::string_view(type_name));

		return findRenderPassType(render_pass_type_hash);
	}

	int32_t RenderGraph::findRenderPassType(uint64_t hash)
	{
		for (size_t i = 0; i < pass_types_runtime.name_hashes.size(); ++i)
			if (pass_types_runtime.name_hashes[i] == hash)
				return static_cast<int32_t>(i);

		return -1;
	}

	/*
	 */
	void RenderGraph::destroyRenderBuffer(RenderGraph::RenderBuffer *buffer)
	{
		assert(buffer);

		invalidateRenderBuffer(buffer);

		delete buffer;
	}

	void RenderGraph::invalidateRenderBuffer(RenderGraph::RenderBuffer *buffer)
	{
		assert(buffer);

		device->destroyTexture(buffer->texture);
		buffer->texture = nullptr;

		device->destroyBindSet(buffer->bindings);
		buffer->bindings = nullptr;
	}

	bool RenderGraph::flushRenderBuffer(RenderGraph::RenderBuffer *texture)
	{
		assert(texture);

		if (texture->texture)
			return false;

		assert(texture->bindings == nullptr);

		hardware::Format format = texture->format;
		uint32_t texture_width = std::max<uint32_t>(1, width / texture->downscale);
		uint32_t texture_height = std::max<uint32_t>(1, height / texture->downscale);

		texture->texture = device->createTexture2D(texture_width, texture_height, 1, format);
		texture->bindings = device->createBindSet();

		device->bindTexture(texture->bindings, 0, texture->texture);
		return true;
	}

	/*
	 */
	void RenderGraph::invalidateFrameBufferCache()
	{
		for (auto &[hash, framebuffer] : framebuffer_cache)
			device->destroyFrameBuffer(framebuffer);

		framebuffer_cache.clear();
	}

	/*
	 */
	void RenderGraph::deserializeRenderBuffers(yaml::NodeRef renderbuffers_root)
	{
		const yaml::NodeRef renderbuffer_container = renderbuffers_root.first_child();

		for (const yaml::NodeRef renderbuffer_node : renderbuffer_container.siblings())
		{
			std::string name;
			hardware::Format format = hardware::Format::UNDEFINED;
			uint32_t downscale = 1;

			for (const yaml::NodeRef renderbuffer_child : renderbuffer_node.children())
			{
				yaml::csubstr renderbuffer_child_key = renderbuffer_child.key();

				if (renderbuffer_child_key.compare("name") == 0 && renderbuffer_child.has_val())
				{
					yaml::csubstr renderbuffer_child_value = renderbuffer_child.val();
					name = std::string(renderbuffer_child_value.data(), renderbuffer_child_value.size());
				}
				else if (renderbuffer_child_key.compare("format") == 0 && renderbuffer_child.has_val())
					renderbuffer_child >> format;
				else if (renderbuffer_child_key.compare("downscale") == 0 && renderbuffer_child.has_val())
					renderbuffer_child >> downscale;
			}

			if (!name.empty() && format != hardware::Format::UNDEFINED)
				addRenderBuffer(name.c_str(), format, downscale);
		}
	}

	/*
	 */
	void RenderGraph::deserializeRenderPass(yaml::NodeRef renderpass_node)
	{
		std::string type_name;
		std::string name;

		for (const yaml::NodeRef renderpass_child : renderpass_node.children())
		{
			yaml::csubstr renderpass_child_key = renderpass_child.key();

			if (renderpass_child_key.compare("name") == 0 && renderpass_child.has_val())
			{
				yaml::csubstr renderpass_child_value = renderpass_child.val();
				name = std::string(renderpass_child_value.data(), renderpass_child_value.size());
			}

			else if (renderpass_child_key.compare("type") == 0 && renderpass_child.has_val())
			{
				yaml::csubstr renderpass_child_value = renderpass_child.val();
				type_name = std::string(renderpass_child_value.data(), renderpass_child_value.size());
			}
		}

		if (name.empty() || type_name.empty())
			return;

		uint64_t render_pass_hash = 0;
		common::HashUtils::combine(render_pass_hash, std::string_view(name));

		int32_t render_pass_index = findRenderPass(render_pass_hash);
		if (render_pass_index != -1)
			return;

		uint64_t render_pass_type_hash = 0;
		common::HashUtils::combine(render_pass_type_hash, std::string_view(type_name));

		int32_t render_pass_type_index = findRenderPassType(render_pass_type_hash);
		if (render_pass_type_index == -1)
			return;

		IRenderPass *render_pass = pass_types_runtime.constructors[render_pass_type_index](this);
		if (!render_pass->deserialize(renderpass_node))
		{
			foundation::Log::error("Can't deserialize render pass with type \"%s\"\n", type_name.c_str());

			delete render_pass;
			render_pass = nullptr;

			return;
		}

		passes_runtime.passes.push_back(render_pass);
		passes_runtime.name_hashes.push_back(render_pass_hash);
		passes_runtime.type_hashes.push_back(render_pass_type_hash);
		passes_runtime.names.push_back(name);
		passes_runtime.type_names.push_back(type_name);
	}
}
