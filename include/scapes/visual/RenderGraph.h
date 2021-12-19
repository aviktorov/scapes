#pragma once

#include <scapes/Common.h>
#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/json/Json.h>

#include <scapes/visual/Resources.h>
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
		virtual void init() = 0;
		virtual void shutdown() = 0;
		virtual void invalidate() = 0;

		virtual void render(foundation::render::CommandBuffer *command_buffer) = 0;

		virtual bool deserialize(const foundation::json::Document &document) = 0;
		virtual foundation::json::Document serialize() = 0;
	};

	/*
	 */
	typedef IRenderPass *(*PFN_createRenderPass)(RenderGraph *render_graph);

	/*
	 */
	class RenderGraph
	{
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

		virtual void setSwapChain(foundation::render::SwapChain *swap_chain) = 0;
		virtual foundation::render::SwapChain *getSwapChain() = 0;
		virtual const foundation::render::SwapChain *getSwapChain() const = 0;

		virtual bool addGroup(const char *name) = 0;
		virtual bool removeGroup(const char *name) = 0;
		virtual void removeAllGroups() = 0;

		virtual foundation::render::BindSet *getGroupBindings(const char *name) const = 0;

		virtual bool addGroupParameter(const char *group_name, const char *parameter_name, size_t size) = 0;
		virtual bool removeGroupParameter(const char *group_name, const char *parameter_name) = 0;
		virtual void removeAllGroupParameters() = 0;

		virtual bool addGroupTexture(const char *group_name, const char *texture_name) = 0;
		virtual bool removeGroupTexture(const char *group_name, const char *texture_name) = 0;
		virtual void removeAllGroupTextures() = 0;

		virtual TextureHandle getGroupTexture(const char *group_name, const char *texture_name) const = 0;
		virtual bool setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle) = 0;

		virtual bool addRenderBuffer(const char *name, foundation::render::Format format, uint32_t downscale) = 0;
		virtual bool removeRenderBuffer(const char *name) = 0;
		virtual void removeAllRenderBuffers() = 0;
		virtual bool swapRenderBuffers(const char *name0, const char *name1) = 0;

		virtual foundation::render::Texture *getRenderBufferTexture(const char *name) const = 0;
		virtual foundation::render::BindSet *getRenderBufferBindings(const char *name) const = 0;
		virtual foundation::render::Format getRenderBufferFormat(const char *name) const = 0;
		virtual uint32_t getRenderBufferDownscale(const char *name) const = 0;

		virtual foundation::render::FrameBuffer *fetchFrameBuffer(uint32_t num_attachments, const char *render_buffer_names[]) = 0;

		virtual size_t getNumRenderPasses() const = 0;
		virtual IRenderPass *getRenderPass(size_t index) const = 0;

		virtual bool removeRenderPass(size_t index) = 0;
		virtual bool removeRenderPass(IRenderPass *pass) = 0;
		virtual void removeAllRenderPasses() = 0;

	public:
		template<typename T>
		SCAPES_INLINE bool addGroupParameter(const char *group_name, const char *parameter_name, size_t count, const T *value)
		{
			bool success = addGroupParameter(group_name, parameter_name, sizeof(T) * count);

			if (success)
				return setGroupParameter<T>(group_name, parameter_name, 1, &value);

			return success;
		}

		template<typename T>
		SCAPES_INLINE bool addGroupParameter(const char *group_name, const char *parameter_name, const T &value)
		{
			return addGroupParameter(group_name, parameter_name, 1, &value);
		}

		template<typename T>
		SCAPES_INLINE T getGroupParameter(const char *group_name, const char *parameter_name) const
		{
			const T *data = getGroupParameter<T>(group_name, parameter_name, 0);
			return *data;
		}

		template<typename T>
		SCAPES_INLINE const T *getGroupParameter(const char *group_name, const char *parameter_name, size_t offset) const
		{
			assert(getGroupParameterSize(group_name, parameter_name) >= offset * sizeof(T));

			const void *data = getGroupParameter(group_name, parameter_name, offset * sizeof(T));
			assert(data);

			return reinterpret_cast<const T*>(data);
		}

		template<typename T>
		SCAPES_INLINE bool setGroupParameter(const char *group_name, const char *parameter_name, size_t count, const T *value)
		{
			assert(getGroupParameterSize(group_name, parameter_name) >= sizeof(T) * count);

			return setGroupParameter(group_name, parameter_name, 0, sizeof(T) * count, value);
		}

		template<typename T>
		SCAPES_INLINE bool setGroupParameter(const char *group_name, const char *parameter_name, const T &value)
		{
			return setGroupParameter<T>(group_name, parameter_name, 1, &value);
		}

		SCAPES_INLINE bool addGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle)
		{
			bool success = addGroupTexture(group_name, texture_name);

			if (success)
				return setGroupTexture(group_name, texture_name, handle);

			return success;
		}

		template<typename T>
		SCAPES_INLINE T *createRenderPass()
		{
			return static_cast<T*>(createRenderPass(TypeTraits<T>::name));
		}

		template<typename T>
		SCAPES_INLINE bool registerRenderPass()
		{
			return registerRenderPass(TypeTraits<T>::name, &T::create);
		}

	protected:
		virtual size_t getGroupParameterSize(const char *group_name, const char *parameter_name) const = 0;
		virtual const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t offset) const = 0;
		virtual bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_offset, size_t src_size, const void *src_data) = 0;

		virtual bool registerRenderPass(const char *name, PFN_createRenderPass function) = 0;
		virtual IRenderPass *createRenderPass(const char *name) = 0;
	};
}
