#pragma once

#include <scapes/visual/HdriImporter.h>

namespace scapes::visual::impl
{
	/*
	 */
	class HdriImporter : public visual::HdriImporter
	{
	public:
		HdriImporter(const HdriImporter::CreateOptions &options);
		~HdriImporter() final;

		TextureHandle bakeBRDF(foundation::render::Format format, uint32_t size) final;
		IBLTextureHandle import(const foundation::io::URI &uri, foundation::render::Format format, uint32_t size, TextureHandle baked_brdf) final;

	private:
		foundation::resources::ResourceManager *resource_manager {nullptr};
		foundation::game::World *world {nullptr};
		foundation::render::Device *device {nullptr};
		foundation::shaders::Compiler *compiler {nullptr};

		MeshHandle unit_quad;
		MeshHandle unit_cube;
		ShaderHandle default_vertex;
		ShaderHandle default_cubemap_geometry;
		ShaderHandle bake_brdf_fragment;
		ShaderHandle equirectangular_projection_fragment;
		ShaderHandle diffuse_irradiance_fragment;
		ShaderHandle prefiltered_specular_fragment;
	};
}
