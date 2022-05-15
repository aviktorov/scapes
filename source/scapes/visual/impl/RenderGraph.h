#pragma once

#include <scapes/Common.h>
#include <scapes/visual/hardware/Device.h>
#include <scapes/visual/RenderGraph.h>

#include "GpuBindings.h"

#include <vector>
#include <unordered_map>

namespace scapes::visual::impl
{
	class RenderGraph : public visual::RenderGraph
	{
	public:
		RenderGraph(
			foundation::resources::ResourceManager *resource_manager,
			hardware::Device *device,
			shaders::Compiler *compiler,
			foundation::game::World *world,
			MeshHandle unit_quad
		);
		~RenderGraph() final;

		SCAPES_INLINE foundation::resources::ResourceManager *getResourceManager() const final { return resource_manager; }
		SCAPES_INLINE hardware::Device *getDevice() const final { return device; }
		SCAPES_INLINE shaders::Compiler *getCompiler() const final { return compiler; }
		SCAPES_INLINE foundation::game::World *getWorld() const final { return world; }
		SCAPES_INLINE MeshHandle getUnitQuad() const final { return unit_quad; }

		SCAPES_INLINE uint32_t getWidth() const final { return width; }
		SCAPES_INLINE uint32_t getHeight() const final { return height; }

		void init(uint32_t width, uint32_t height) final;
		void shutdown() final;

		void resize(uint32_t width, uint32_t height) final;
		void render(hardware::CommandBuffer command_buffer) final;

		bool load(const foundation::io::URI &uri) final;
		bool save(const foundation::io::URI &uri) final;

		bool deserialize(const foundation::serde::yaml::Tree &tree) final;
		foundation::serde::yaml::Tree serialize() final;

		SCAPES_INLINE void setSwapChain(hardware::SwapChain chain) final { swap_chain = chain; }
		SCAPES_INLINE hardware::SwapChain getSwapChain() final { return swap_chain; }
		SCAPES_INLINE const hardware::SwapChain getSwapChain() const final { return swap_chain; }

		bool addGroup(const char *name) final;
		bool removeGroup(const char *name) final;
		void removeAllGroups() final;

		hardware::BindSet getGroupBindings(const char *name) const final;

		bool addGroupParameter(const char *group_name, const char *parameter_name, size_t element_size, size_t num_elements) final;
		bool addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements) final;
		bool removeGroupParameter(const char *group_name, const char *parameter_name) final;

		bool addGroupTexture(const char *group_name, const char *texture_name) final;
		bool removeGroupTexture(const char *group_name, const char *texture_name) final;

		TextureHandle getGroupTexture(const char *group_name, const char *texture_name) const final;
		bool setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle) final;

		bool addRenderBuffer(const char *name, hardware::Format format, uint32_t downscale) final;
		bool removeRenderBuffer(const char *name) final;
		void removeAllRenderBuffers() final;
		bool swapRenderBuffers(const char *name0, const char *name1) final;

		hardware::Texture getRenderBufferTexture(const char *name) const final;
		hardware::BindSet getRenderBufferBindings(const char *name) const final;
		hardware::Format getRenderBufferFormat(const char *name) const final;
		uint32_t getRenderBufferDownscale(const char *name) const final;

		hardware::FrameBuffer fetchFrameBuffer(uint32_t num_attachments, const char *render_buffer_names[]) final;

		SCAPES_INLINE size_t getNumRenderPasses() const final { return passes_runtime.passes.size(); }
		SCAPES_INLINE IRenderPass *getRenderPass(size_t index) const final { return passes_runtime.passes[index]; }
		IRenderPass *getRenderPass(const char *name) const final;

		bool removeRenderPass(size_t index) final;
		bool removeRenderPass(IRenderPass *pass) final;
		void removeAllRenderPasses() final;

	private:
		struct RenderBuffer
		{
			std::string name;
			hardware::Format format {hardware::Format::UNDEFINED};
			hardware::Texture texture {SCAPES_NULL_HANDLE};
			hardware::BindSet bindings {SCAPES_NULL_HANDLE};
			uint32_t downscale {1};
		};

		struct RenderPassTypeRuntime
		{
			std::vector<std::string> names;
			std::vector<uint64_t> name_hashes;
			std::vector<PFN_createRenderPass> constructors;
		};

		struct RenderPassesRuntime
		{
			std::vector<std::string> names;
			std::vector<std::string> type_names;
			std::vector<uint64_t> name_hashes;
			std::vector<uint64_t> type_hashes;
			std::vector<IRenderPass *> passes;
		};

	private:
		size_t getGroupParameterElementSize(const char *group_name, const char *parameter_name) const final;
		size_t getGroupParameterNumElements(const char *group_name, const char *parameter_name) const final;
		const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const final;
		bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data) final;

		bool registerRenderPassType(const char *type_name, PFN_createRenderPass function) final;
		IRenderPass *createRenderPass(const char *type_name, const char *name) final;
		int32_t findRenderPass(const char *name);
		int32_t findRenderPass(uint64_t hash);
		int32_t findRenderPassType(const char *type_name);
		int32_t findRenderPassType(uint64_t hash);

		void deserializeRenderBuffers(foundation::serde::yaml::NodeRef renderbuffers_root);
		void deserializeRenderPass(foundation::serde::yaml::NodeRef renderpass_node);

	private:
		void destroyRenderBuffer(RenderBuffer *buffer);
		void invalidateRenderBuffer(RenderBuffer *buffer);
		bool flushRenderBuffer(RenderBuffer *buffer);

		void invalidateFrameBufferCache();

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		hardware::Device *device {nullptr};
		shaders::Compiler *compiler {nullptr};
		foundation::game::World *world {nullptr};

		MeshHandle unit_quad;

		uint32_t width {0};
		uint32_t height {0};

		RenderPassTypeRuntime pass_types_runtime;
		RenderPassesRuntime passes_runtime;

		ParameterAllocator parameter_allocator;
		GpuBindings gpu_bindings;

		std::unordered_map<uint64_t, RenderBuffer *> render_buffer_lookup;
		std::unordered_map<uint64_t, hardware::FrameBuffer> framebuffer_cache;

		hardware::SwapChain swap_chain {SCAPES_NULL_HANDLE};
	};
}
