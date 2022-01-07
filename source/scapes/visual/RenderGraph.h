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
		RenderGraphImpl(API *api, foundation::io::FileSystem *file_system);
		~RenderGraphImpl() final;

		SCAPES_INLINE API *getAPI() const final { return api; }
		SCAPES_INLINE foundation::io::FileSystem *getFileSystem() const final { return file_system; }

		SCAPES_INLINE uint32_t getWidth() const final { return width; }
		SCAPES_INLINE uint32_t getHeight() const final { return height; }

		void init(uint32_t width, uint32_t height) final;
		void shutdown() final;

		void resize(uint32_t width, uint32_t height) final;
		void render(foundation::render::CommandBuffer *command_buffer) final;

		bool load(const foundation::io::URI &uri) final;
		bool save(const foundation::io::URI &uri) final;

		bool deserialize(const foundation::serde::yaml::Tree &tree) final;
		foundation::serde::yaml::Tree serialize() final;

		SCAPES_INLINE void setSwapChain(foundation::render::SwapChain *chain) final { swap_chain = chain; }
		SCAPES_INLINE foundation::render::SwapChain *getSwapChain() final { return swap_chain; }
		SCAPES_INLINE const foundation::render::SwapChain *getSwapChain() const final { return swap_chain; }

		bool addGroup(const char *name) final;
		bool removeGroup(const char *name) final;
		void removeAllGroups() final;

		foundation::render::BindSet *getGroupBindings(const char *name) const final;

		bool addGroupParameter(const char *group_name, const char *parameter_name, size_t type_size, size_t num_elements) final;
		bool addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements) final;
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

		SCAPES_INLINE size_t getNumRenderPasses() const final { return passes_runtime.passes.size(); }
		SCAPES_INLINE IRenderPass *getRenderPass(size_t index) const final { return passes_runtime.passes[index]; }
		IRenderPass *getRenderPass(const char *name) const final;

		bool removeRenderPass(size_t index) final;
		bool removeRenderPass(IRenderPass *pass) final;
		void removeAllRenderPasses() final;

	private:
		struct Group;
		struct GroupParameter;
		struct GroupTexture;

		struct GroupParameter
		{
			std::string name;
			GroupParameterType type {GroupParameterType::UNDEFINED};
			size_t type_size {0};
			size_t num_elements {0};
			void *memory {nullptr};

			Group *group {nullptr};
		};

		struct GroupTexture
		{
			std::string name;
			TextureHandle texture;

			Group *group {nullptr};
		};

		struct Group
		{
			std::string name;
			std::vector<GroupParameter *> parameters;
			std::vector<GroupTexture *> textures;

			foundation::render::BindSet *bindings {nullptr};
			foundation::render::UniformBuffer *buffer {nullptr};
			uint32_t buffer_size {0};
			bool dirty {true};
		};

		struct RenderBuffer
		{
			std::string name;
			foundation::render::Format format {foundation::render::Format::UNDEFINED};
			foundation::render::Texture *texture {nullptr};
			foundation::render::BindSet *bindings {nullptr};
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
		bool addGroupParameterInternal(const char *group_name, const char *parameter_name, GroupParameterType type, size_t type_size, size_t num_elements);

		size_t getGroupParameterTypeSize(const char *group_name, const char *parameter_name) const final;
		size_t getGroupParameterNumElements(const char *group_name, const char *parameter_name) const final;
		const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const final;
		bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data) final;

		bool registerRenderPassType(const char *type_name, PFN_createRenderPass function) final;
		IRenderPass *createRenderPass(const char *type_name, const char *name) final;
		int32_t findRenderPass(const char *name);
		int32_t findRenderPass(uint64_t hash);
		int32_t findRenderPassType(const char *type_name);
		int32_t findRenderPassType(uint64_t hash);

		void deserializeGroup(foundation::serde::yaml::NodeRef group_node);
		void deserializeGroupParameter(const char *group_name, foundation::serde::yaml::NodeRef parameter_node);
		void deserializeGroupTexture(const char *group_name, foundation::serde::yaml::NodeRef texture_node);
		void deserializeRenderBuffers(foundation::serde::yaml::NodeRef renderbuffers_root);
		void deserializeRenderPass(foundation::serde::yaml::NodeRef renderpass_node);

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
		API *api {nullptr};
		foundation::io::FileSystem *file_system {nullptr};

		uint32_t width {0};
		uint32_t height {0};

		RenderPassTypeRuntime pass_types_runtime;
		RenderPassesRuntime passes_runtime;

		ParameterAllocator parameter_allocator;

		std::unordered_map<uint64_t, Group *> group_lookup;
		std::unordered_map<uint64_t, GroupParameter *> group_parameter_lookup;
		std::unordered_map<uint64_t, GroupTexture *> group_texture_lookup;
		std::unordered_map<uint64_t, RenderBuffer *> render_buffer_lookup;

		std::unordered_map<uint64_t, foundation::render::FrameBuffer *> framebuffer_cache;

		foundation::render::SwapChain *swap_chain {nullptr};
	};
}
