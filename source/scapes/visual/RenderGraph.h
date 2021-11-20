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

		void addParameterGroup(const char *name) final;
		bool removeParameterGroup(const char *name) final;
		void removeAllParameterGroups() final;

		foundation::render::BindSet *getParameterGroupBindings(const char *name) const final;

		bool addParameter(const char *group_name, const char *parameter_name, size_t size) final;
		bool removeParameter(const char *group_name, const char *parameter_name) final;
		void removeAllParameters() final;

		size_t getParameterSize(const char *group_name, const char *parameter_name) const final;
		const void *getParameterValue(const char *group_name, const char *parameter_name, size_t offset) const final;
		bool setParameterValue(const char *group_name, const char *parameter_name, size_t dst_offset, size_t src_size, const void *src_data) final;

		void addTextureRenderBuffer(const char *name, foundation::render::Format format, uint32_t downscale) final;
		void addTextureResource(const char *name, TextureHandle handle) final;
		void addTextureSwapChain(const char *name, foundation::render::SwapChain *swap_chain) final;

		bool removeTexture(const char *name) final;
		void removeAllTextures() final;

		TextureType getTextureType(const char *name) const final;
		foundation::render::Texture *getTexture(const char *name) const final;

		foundation::render::Texture *getTextureRenderBuffer(const char *name) const final;
		foundation::render::Format getTextureRenderBufferFormat(const char *name) const final;
		uint32_t getTextureRenderBufferDownscale(const char *name) const final;

		TextureHandle getTextureResource(const char *name) const final;
		bool setTextureResource(const char *name, TextureHandle handle) final;

		foundation::render::SwapChain *getTextureSwapChain(const char *name) const final;
		bool setTextureSwapChain(const char *name, foundation::render::SwapChain *swap_chain) final;

		void addRenderPass(IRenderPass *pass) final;
		bool addRenderPass(IRenderPass *pass, size_t index) final;
		bool removeRenderPass(size_t index) final;
		void removeAllRenderPasses() final;

		bool registerRenderPassType(const char *name, PFN_createRenderPass function) final;

	private:
		struct Parameter;
		struct ParameterGroup;
		struct TextureResource;

		struct Parameter
		{
			ParameterGroup *group {nullptr};

			size_t size {0};
			void *memory {nullptr};
		};

		struct ParameterGroup
		{
			foundation::render::UniformBuffer *gpu_resource {nullptr};
			uint32_t gpu_resource_size {0};

			foundation::render::BindSet *bindings {nullptr};
			std::vector<Parameter *> parameters;
			bool dirty {true};
		};

		struct TextureResource
		{
			TextureType type {TextureType::INVALID};
			foundation::render::Format render_buffer_format {foundation::render::Format::UNDEFINED};
			uint32_t render_buffer_downscale {1};

			foundation::render::SwapChain *swap_chain;
			foundation::render::Texture *render_buffer;
			TextureHandle resource;
		};

	private:
		void destroyParameterGroup(ParameterGroup *group);
		void invalidateParameterGroup(ParameterGroup *group);
		bool flushParameterGroup(ParameterGroup *group);

		void destroyParameter(Parameter *parameter);

		void destroyTextureResource(TextureResource *texture);
		void invalidateTextureResource(TextureResource *texture);
		bool flushTextureResource(TextureResource *texture);

	private:
		uint32_t width {0};
		uint32_t height {0};

		std::unordered_map<const char *, PFN_createRenderPass> registered_render_pass_types;
		std::vector<IRenderPass *> passes;

		ParameterAllocator parameter_allocator;

		std::unordered_map<uint64_t, ParameterGroup *> parameter_group_lookup;
		std::unordered_map<uint64_t, Parameter *> parameter_lookup;
		std::unordered_map<uint64_t, TextureResource *> texture_lookup;

		scapes::foundation::render::Device *device {nullptr};
		scapes::foundation::game::World *world {nullptr};
	};
}
