#pragma once

#include <scapes/Common.h>
#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/json/Json.h>

#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	class IRenderPass
	{
	public:
		virtual ~IRenderPass() { }

	public:
		virtual void init(const RenderGraph *render_graph) = 0;
		virtual void shutdown() = 0;
		virtual void invalidate() = 0;

		virtual void render(foundation::render::CommandBuffer *command_buffer) = 0;

		virtual bool deserialize(const foundation::json::Document &document) = 0;
		virtual foundation::json::Document serialize() = 0;
	};

	/*
	 */
	typedef IRenderPass *(*PFN_createRenderPass)();

	/*
	 */
	class RenderGraph
	{
	public:
		enum TextureType
		{
			INVALID = 0,
			RENDER_BUFFER,
			RESOURCE,
			SWAP_CHAIN,
		};

	public:
		static SCAPES_API RenderGraph *create(foundation::render::Device *device, foundation::game::World *world);
		static SCAPES_API void destroy(RenderGraph *render_graph);

		virtual ~RenderGraph() { }

	public:
		virtual foundation::render::Device *getDevice() const = 0;
		virtual foundation::game::World *getWorld() const = 0;

		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;

		virtual void init(uint32_t width, uint32_t height) = 0;
		virtual void shutdown() = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;
		virtual void render(foundation::render::CommandBuffer *command_buffer) = 0;

		virtual bool load(const foundation::io::URI &uri) = 0;
		virtual bool save(const foundation::io::URI &uri) = 0;

		virtual bool deserialize(const foundation::json::Document &document) = 0;
		virtual foundation::json::Document serialize() = 0;

		virtual void addParameterGroup(const char *name) = 0;
		virtual bool removeParameterGroup(const char *name) = 0;
		virtual void removeAllParameterGroups() = 0;

		virtual foundation::render::BindSet *getParameterGroupBindings(const char *name) const = 0;

		virtual bool addParameter(const char *group, const char *name, size_t size) = 0;
		virtual bool removeParameter(const char *group, const char *name) = 0;
		virtual void removeAllParameters() = 0;

		virtual void addTextureRenderBuffer(const char *name, foundation::render::Format format, uint32_t downscale) = 0;
		virtual void addTextureResource(const char *name, TextureHandle handle) = 0;
		virtual void addTextureSwapChain(const char *name, foundation::render::SwapChain *swap_chain) = 0;

		virtual bool removeTexture(const char *name) = 0;
		virtual void removeAllTextures() = 0;

		virtual TextureType getTextureType(const char *name) const = 0;
		virtual foundation::render::Texture *getTexture(const char *name) const = 0;

		virtual foundation::render::Texture *getTextureRenderBuffer(const char *name) const = 0;
		virtual foundation::render::Format getTextureRenderBufferFormat(const char *name) const = 0;
		virtual uint32_t getTextureRenderBufferDownscale(const char *name) const = 0;

		virtual bool swapTextureRenderBuffers(const char *name0, const char *name1) = 0;

		virtual TextureHandle getTextureResource(const char *name) const = 0;
		virtual bool setTextureResource(const char *name, TextureHandle handle) = 0;

		virtual foundation::render::SwapChain *getTextureSwapChain(const char *name) const = 0;
		virtual bool setTextureSwapChain(const char *name, foundation::render::SwapChain *swap_chain) = 0;

		virtual size_t getNumRenderPasses() const = 0;
		virtual IRenderPass *getRenderPass(size_t index) const = 0;

		virtual void addRenderPass(IRenderPass *pass) = 0;
		virtual bool addRenderPass(IRenderPass *pass, size_t index) = 0;
		virtual bool removeRenderPass(size_t index) = 0;
		virtual void removeAllRenderPasses() = 0;

		// TODO: add template method addParameter with default value

		template<typename T>
		SCAPES_INLINE T getParameterValue(const char *group, const char *name) const
		{
			const T *data = getParameterValue<T>(group, name, 0);
			return *data;
		}

		template<typename T>
		SCAPES_INLINE const T *getParameterValue(const char *group, const char *name, size_t offset) const
		{
			assert(getParameterSize(group, name) >= offset * sizeof(T));

			const void *data = getParameterValue(group, name, offset * sizeof(T));
			assert(data);

			return reinterpret_cast<const T*>(data);
		}

		template<typename T>
		SCAPES_INLINE bool setParameterValue(const char *group, const char *name, size_t count, const T *value)
		{
			assert(getParameterSize(group, name) >= sizeof(T) * count);

			return setParameterValue(group, name, 0, sizeof(T) * count, value);
		}

		template<typename T>
		SCAPES_INLINE bool setParameterValue(const char *group, const char *name, const T &value)
		{
			return setParameterValue<T>(group, name, 1, &value);
		}

		template<typename T>
		SCAPES_INLINE bool registerRenderPassType()
		{
			return registerRenderPassType(TypeTraits<T>::name, &T::create);
		}

	protected:
		virtual size_t getParameterSize(const char *group, const char *name) const = 0;
		virtual const void *getParameterValue(const char *group, const char *name, size_t offset) const = 0;
		virtual bool setParameterValue(const char *group, const char *name, size_t dst_offset, size_t src_size, const void *src_data) = 0;
		virtual bool registerRenderPassType(const char *name, PFN_createRenderPass function) = 0;

	};
}
