#pragma once

#include <vector>

#include <scapes/visual/IBLTexture.h>
#include <scapes/visual/Mesh.h>
#include <scapes/visual/Material.h>
#include <scapes/visual/Shader.h>
#include <scapes/visual/Texture.h>

/*
 */
class ApplicationResources
{
public:
	ApplicationResources(
		scapes::foundation::resources::ResourceManager *resource_manager,
		scapes::visual::hardware::Device *device,
		scapes::visual::shaders::Compiler *compiler,
		scapes::foundation::game::World *world
	)
		: resource_manager(resource_manager), device(device), compiler(compiler), world(world)
	{ }

	virtual ~ApplicationResources();

	void init();
	void shutdown();

	SCAPES_INLINE scapes::visual::IBLTextureHandle getIBLTexture(int index) const { return loaded_ibl_textures[index]; }
	SCAPES_INLINE size_t getNumIBLTextures() const { return loaded_ibl_textures.size(); }
	const char *getIBLTexturePath(int index) const;

	SCAPES_INLINE scapes::visual::MeshHandle getUnitQuad() const { return unit_quad; }

private:
	scapes::visual::MeshHandle generateMeshQuad(float size);
	scapes::visual::MeshHandle generateMeshCube(float size);

private:
	scapes::foundation::resources::ResourceManager *resource_manager {nullptr};
	scapes::visual::hardware::Device *device {nullptr};
	scapes::visual::shaders::Compiler *compiler {nullptr};
	scapes::foundation::game::World *world {nullptr};

	scapes::visual::MeshHandle unit_quad;
	scapes::visual::MeshHandle unit_cube;

	scapes::visual::TextureHandle baked_brdf;
	scapes::visual::MaterialHandle default_material;

	std::vector<scapes::visual::ShaderHandle> loaded_shaders;
	std::vector<scapes::visual::IBLTextureHandle> loaded_ibl_textures;

	scapes::visual::GlbImporter *glb_importer {nullptr};
	scapes::visual::HdriImporter *hdri_importer {nullptr};
};
