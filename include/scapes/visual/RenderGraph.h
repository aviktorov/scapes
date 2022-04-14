#pragma once

#include <scapes/Common.h>
#include <scapes/foundation/TypeTraits.h>
#include <scapes/foundation/serde/Yaml.h>

#include <scapes/visual/Mesh.h>
#include <scapes/visual/Texture.h>
#include <scapes/visual/Fwd.h>

namespace scapes::visual
{
	/*
	 */
	enum class GroupParameterType
	{
		UNDEFINED = 0,
		FLOAT,
		INT,
		UINT,
		VEC2,
		VEC3,
		VEC4,
		IVEC2,
		IVEC3,
		IVEC4,
		UVEC2,
		UVEC3,
		UVEC4,
		MAT3,
		MAT4,

		MAX,
	};

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

		virtual void render(hardware::CommandBuffer command_buffer) = 0;

		virtual bool deserialize(const foundation::serde::yaml::NodeRef node) = 0;
		virtual bool serialize(foundation::serde::yaml::NodeRef node) = 0;
	};

	/*
	 */
	typedef IRenderPass *(*PFN_createRenderPass)(RenderGraph *render_graph);

	/*
	 */
	class RenderGraph
	{
	public:
		static SCAPES_API RenderGraph *create(
			foundation::resources::ResourceManager *resource_manager,
			hardware::Device *device,
			shaders::Compiler *compiler,
			foundation::game::World *world,
			MeshHandle unit_quad
		);
		static SCAPES_API void destroy(RenderGraph *render_graph);

		virtual ~RenderGraph() { }

	public:
		virtual foundation::resources::ResourceManager *getResourceManager() const = 0;
		virtual hardware::Device *getDevice() const = 0;
		virtual shaders::Compiler *getCompiler() const = 0;
		virtual foundation::game::World *getWorld() const = 0;
		virtual MeshHandle getUnitQuad() const = 0;

		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;

		virtual void init(uint32_t width, uint32_t height) = 0;
		virtual void shutdown() = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;
		virtual void render(hardware::CommandBuffer command_buffer) = 0;

		virtual bool load(const foundation::io::URI &uri) = 0;
		virtual bool save(const foundation::io::URI &uri) = 0;

		virtual bool deserialize(const foundation::serde::yaml::Tree &tree) = 0;
		virtual foundation::serde::yaml::Tree serialize() = 0;

		virtual void setSwapChain(hardware::SwapChain swap_chain) = 0;
		virtual hardware::SwapChain getSwapChain() = 0;
		virtual const hardware::SwapChain getSwapChain() const = 0;

		virtual bool addGroup(const char *name) = 0;
		virtual bool removeGroup(const char *name) = 0;
		virtual void removeAllGroups() = 0;

		virtual hardware::BindSet getGroupBindings(const char *name) const = 0;

		virtual bool addGroupParameter(const char *group_name, const char *parameter_name, size_t type_size, size_t num_elements) = 0;
		virtual bool addGroupParameter(const char *group_name, const char *parameter_name, GroupParameterType type, size_t num_elements) = 0;
		virtual bool removeGroupParameter(const char *group_name, const char *parameter_name) = 0;
		virtual void removeAllGroupParameters() = 0;

		virtual bool addGroupTexture(const char *group_name, const char *texture_name) = 0;
		virtual bool removeGroupTexture(const char *group_name, const char *texture_name) = 0;
		virtual void removeAllGroupTextures() = 0;

		virtual TextureHandle getGroupTexture(const char *group_name, const char *texture_name) const = 0;
		virtual bool setGroupTexture(const char *group_name, const char *texture_name, TextureHandle handle) = 0;

		virtual bool addRenderBuffer(const char *name, hardware::Format format, uint32_t downscale) = 0;
		virtual bool removeRenderBuffer(const char *name) = 0;
		virtual void removeAllRenderBuffers() = 0;
		virtual bool swapRenderBuffers(const char *name0, const char *name1) = 0;

		virtual hardware::Texture getRenderBufferTexture(const char *name) const = 0;
		virtual hardware::BindSet getRenderBufferBindings(const char *name) const = 0;
		virtual hardware::Format getRenderBufferFormat(const char *name) const = 0;
		virtual uint32_t getRenderBufferDownscale(const char *name) const = 0;

		virtual hardware::FrameBuffer fetchFrameBuffer(uint32_t num_attachments, const char *render_buffer_names[]) = 0;

		virtual size_t getNumRenderPasses() const = 0;
		virtual IRenderPass *getRenderPass(size_t index) const = 0;
		virtual IRenderPass *getRenderPass(const char *name) const = 0;

		virtual bool removeRenderPass(size_t index) = 0;
		virtual bool removeRenderPass(IRenderPass *pass) = 0;
		virtual void removeAllRenderPasses() = 0;

	public:
		template<typename T>
		SCAPES_INLINE bool addGroupParameter(const char *group_name, const char *parameter_name, size_t num_elements, const T *value)
		{
			bool success = addGroupParameter(group_name, parameter_name, sizeof(T), num_elements);

			if (success)
				return setGroupParameter<T>(group_name, parameter_name, 1, value);

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
		SCAPES_INLINE const T *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const
		{
			assert(getGroupParameterTypeSize(group_name, parameter_name) == sizeof(T));
			assert(getGroupParameterNumElements(group_name, parameter_name) > index);

			const void *data = getGroupParameter(group_name, parameter_name, index);
			assert(data);

			return reinterpret_cast<const T*>(data);
		}

		template<typename T>
		SCAPES_INLINE bool setGroupParameter(const char *group_name, const char *parameter_name, size_t num_elements, const T *value)
		{
			assert(getGroupParameterTypeSize(group_name, parameter_name) == sizeof(T));
			assert(getGroupParameterNumElements(group_name, parameter_name) >= num_elements);

			return setGroupParameter(group_name, parameter_name, 0, num_elements, value);
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
		SCAPES_INLINE T *createRenderPass(const char *name)
		{
			return static_cast<T*>(createRenderPass(TypeTraits<T>::name), name);
		}

		template<typename T>
		SCAPES_INLINE T *getRenderPass(const char *name)
		{
			return static_cast<T*>(getRenderPass(name));
		}

		template<typename T>
		SCAPES_INLINE bool registerRenderPassType()
		{
			return registerRenderPassType(TypeTraits<T>::name, &T::create);
		}

	protected:
		virtual size_t getGroupParameterTypeSize(const char *group_name, const char *parameter_name) const = 0;
		virtual size_t getGroupParameterNumElements(const char *group_name, const char *parameter_name) const = 0;
		virtual const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const = 0;
		virtual bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data) = 0;

		virtual bool registerRenderPassType(const char *type_name, PFN_createRenderPass function) = 0;
		virtual IRenderPass *createRenderPass(const char *type_name, const char *name) = 0;
	};
}
