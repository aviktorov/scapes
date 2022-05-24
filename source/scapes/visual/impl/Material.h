#pragma once

#include <scapes/Common.h>
#include <scapes/visual/hardware/Device.h>
#include <scapes/visual/Material.h>

#include "GpuBindings.h"

#include <vector>
#include <unordered_map>

namespace scapes::visual::impl
{
	class Material : public visual::Material
	{
	public:
		Material(
			foundation::resources::ResourceManager *resource_manager,
			hardware::Device *device,
			shaders::Compiler *compiler
		);
		~Material() final;

		SCAPES_INLINE foundation::resources::ResourceManager *getResourceManager() const final { return resource_manager; }
		SCAPES_INLINE hardware::Device *getDevice() const final { return device; }
		SCAPES_INLINE shaders::Compiler *getCompiler() const final { return compiler; }

		MaterialHandle clone() final;
		bool flush() final;

		bool deserialize(const foundation::serde::yaml::Tree &tree) final;
		foundation::serde::yaml::Tree serialize() final;

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

	private:
		size_t getGroupParameterElementSize(const char *group_name, const char *parameter_name) const final;
		size_t getGroupParameterNumElements(const char *group_name, const char *parameter_name) const final;
		const void *getGroupParameter(const char *group_name, const char *parameter_name, size_t index) const final;
		bool setGroupParameter(const char *group_name, const char *parameter_name, size_t dst_index, size_t num_src_elements, const void *src_data) final;

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		hardware::Device *device {nullptr};
		shaders::Compiler *compiler {nullptr};

		GpuBindings gpu_bindings;
	};
}
