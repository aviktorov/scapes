#pragma once

#include <scapes/foundation/render/Device.h>

#include <scapes/visual/RenderGraph.h>
#include <scapes/visual/Resources.h>

#include <vector>
#include <unordered_map>

namespace scapes::visual
{
	class ParameterAllocator
	{
	public:
		void *allocate(size_t size);
		void deallocate(void *memory);
	
	private:
	};

	class RenderGraphImpl : public RenderGraph
	{
	public:
		RenderGraphImpl(foundation::render::Device *device, foundation::game::World *world);
		~RenderGraphImpl() final;

		SCAPES_INLINE foundation::render::Device *getDevice() const final { return device; }
		SCAPES_INLINE foundation::game::World *getWorld() const final { return world; }

		SCAPES_INLINE uint32_t getWidth() const final { return width; }
		SCAPES_INLINE uint32_t getHeight() const final { return height; }

		void init(uint32_t width, uint32_t height) final;
		void shutdown() final;

		void resize(uint32_t width, uint32_t height) final;
		void render(foundation::render::CommandBuffer *command_buffer) final;

		bool load(const foundation::io::URI &uri) final;
		bool save(const foundation::io::URI &uri) final;

		bool deserialize(const foundation::json::Document &document) final;
		foundation::json::Document serialize() final;

		SCAPES_INLINE void setSwapChain(foundation::render::SwapChain *chain) final { swap_chain = chain; }
		SCAPES_INLINE foundation::render::SwapChain *getSwapChain() final { return swap_chain; }
		SCAPES_INLINE const foundation::render::SwapChain *getSwapChain() const final { return swap_chain; }

		bool addGroup(const char *name) final;
		bool removeGroup(const char *name) final;
		void removeAllGroups() final;

		foundation::render::BindSet *getGroupBindings(const char *name) const final;

		bool addGroupParameter(const char *group_name, const char *parameter_name, size_t size) final;
		bool removeGroupParameter(const char *group_name, const char *parameter_name) final;
		void removeAllGroupParameters() final;

		bool addGroupTexture(const char *group_name, const char *texture_name) final;
		bool removeGroupTexture(const char *group_name, const char *texture_name) final;
		void removeAllGroupTextures() final;

		TextureHandle getGroupTexture(const char *group_name, const char *texture_name) const final;
		bool setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle) final;

		bool addRenderBuffer(const char *name, foundation::render::Format format, uint32_t downscale) final;
		bool removeRenderBuffer(const char *name) final;
		void removeAllRenderBuffers() final;
		bool swapRenderBuffers(const char *name0, const char *name1) final;

		foundation::render::Texture *getRenderBufferTexture(const char *name) const final;
		foundation::render::BindSet *getRenderBufferBindings(const char *name) const final;
		foundation::render::Format getRenderBufferFormat(const char *name) const final;
		uint32_t getRenderBufferDownscale(const char *name) const final;

		foundation::render::FrameBuffer *fetchFrameBuffer(uint32_t num_attachments, const char *render_buffer_names[]) final;

		SCAPES_INLINE size_t getNumRenderPasses() const final { return passes.size(); }
		SCAPES_INLINE IRenderPass *getRenderPass(size_t index) const final { return passes[index]; }

		bool removeRenderPass(size_t index) final;
		bool removeRenderPass(IRenderPass *pass) final;
		void removeAllRenderPasses() final;

	private:
		struct Group;
		struct GroupParameter;
		struct GroupTexture;

		struct GroupParameter
		{
			Group *group {nullptr};

			size_t size {0};
			void *memory {nullptr};
		};

		struct GroupTexture
		{
			Group *group {nullptr};

			TextureHandle texture;
		};

		struct Group
		{
			foundation::render::UniformBuffer *buffer {nullptr};
			uint32_t buffer_size {0};

			foundation::render::BindSet *bindings {nullptr};
			std::vector<GroupParameter *> parameters;
			std::vector<GroupTexture *> textures;
			bool dirty {true};
		};

		struct RenderBuffer
		{
			foundation::render::Format format {foundation::render::Format::UNDEFINED};
			foundation::render::Texture *texture {nullptr};
			foundation::render::BindSet *bindings {nullptr};
			uint32_t downscale {1};
	};

	private:
		size_t getGroupParameterSize(const char *group_name, const char *parameter_name) const final;
		const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t offset) const final;
		bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_offset, size_t src_size, const void *src_data) final;

		bool registerRenderPass(const char *name, PFN_createRenderPass function) final;
		IRenderPass *createRenderPass(const char *name) final;

	private:
		void destroyGroup(Group *group);
		void invalidateGroup(Group *group);
		bool flushGroup(Group *group);

		void destroyGroupParameter(GroupParameter *parameter);
		void destroyGroupTexture(GroupTexture *texture);

		void destroyRenderBuffer(RenderBuffer *buffer);
		void invalidateRenderBuffer(RenderBuffer *buffer);
		bool flushRenderBuffer(RenderBuffer *buffer);

		void invalidateFrameBufferCache();

	private:
		uint32_t width {0};
		uint32_t height {0};

		std::unordered_map<uint64_t, PFN_createRenderPass> render_pass_type_lookup;
		std::vector<IRenderPass *> passes;

		ParameterAllocator parameter_allocator;

		std::unordered_map<uint64_t, Group *> group_lookup;
		std::unordered_map<uint64_t, GroupParameter *> group_parameter_lookup;
		std::unordered_map<uint64_t, GroupTexture *> group_texture_lookup;
		std::unordered_map<uint64_t, RenderBuffer *> render_buffer_lookup;

		std::unordered_map<uint64_t, foundation::render::FrameBuffer *> framebuffer_cache;

		foundation::render::SwapChain *swap_chain {nullptr};
		foundation::render::Device *device {nullptr};
		foundation::game::World *world {nullptr};
	};
}
