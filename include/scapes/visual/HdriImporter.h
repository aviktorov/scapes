#pragma once

#include <scapes/visual/Fwd.h>
#include <scapes/visual/IBLTexture.h>
#include <scapes/visual/Mesh.h>
#include <scapes/visual/Shader.h>
#include <scapes/visual/Texture.h>

namespace scapes::visual
{
	/*
	 */
	class HdriImporter
	{
	public:
		struct CreateOptions
		{
			foundation::resources::ResourceManager *resource_manager {nullptr};
			foundation::game::World *world {nullptr};
			hardware::Device *device {nullptr};
			shaders::Compiler *compiler {nullptr};

			MeshHandle unit_quad;
			MeshHandle unit_cube;
			ShaderHandle default_vertex;
			ShaderHandle default_cubemap_geometry;
			ShaderHandle bake_brdf_fragment;
			ShaderHandle equirectangular_projection_fragment;
			ShaderHandle diffuse_irradiance_fragment;
			ShaderHandle prefiltered_specular_fragment;
		};

	public:
		static SCAPES_API HdriImporter *create(const CreateOptions &options);
		static SCAPES_API void destroy(HdriImporter *importer);
		
		virtual ~HdriImporter() { }

	public:
		virtual TextureHandle bakeBRDF(hardware::Format format, uint32_t size) = 0;
		virtual IBLTextureHandle import(const foundation::io::URI &uri, hardware::Format format, uint32_t size, TextureHandle baked_brdf) = 0;
	};
}
