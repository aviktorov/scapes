#pragma once

#include <vector>
#include <unordered_map>
#include <render/backend/Driver.h>
#include <common/ResourceManager.h>

namespace render::shaders
{
	class Compiler;
}

struct aiMesh;
struct cgltf_mesh;

struct Shader;
struct Mesh;
struct Texture;
struct IBLTexture;
struct RenderMaterial;

namespace config
{
	enum Shaders
	{
		CubemapVertex = 0,
		FullscreenQuadVertex,
		EquirectangularProjectionFragment,
		PrefilteredSpecularCubemapFragment,
		DiffuseIrradianceCubemapFragment,
		BakedBRDFFragment,
		SkylightDeferredFragment,
		GBufferVertex,
		GBufferFragment,
		SSAOFragment,
		SSAOBlurFragment,
		SSRTraceFragment,
		SSRResolveFragment,
		CompositeFragment,
		TemporalFilterFragment,
		TonemappingFragment,
		FinalFragment,
	};
}

/*
 */
class ApplicationResources
{
public:
	ApplicationResources(render::backend::Driver *driver, render::shaders::Compiler *compiler, resources::ResourceManager *resource_manager)
		: driver(driver), compiler(compiler), resource_manager(resource_manager) { }

	virtual ~ApplicationResources();

	void init();
	void shutdown();

	inline resources::ResourceManager *getResourceManager() const { return resource_manager; }

	inline resources::ResourceHandle<Texture> getBlueNoiseTexture() const { return blue_noise; }

	inline resources::ResourceHandle<IBLTexture> getIBLTexture(int index) const { return loaded_ibl_textures[index]; }
	inline size_t getNumIBLTextures() const { return loaded_ibl_textures.size(); }
	const char *getIBLTexturePath(int index) const;

	inline resources::ResourceHandle<Texture> getBakedBRDFTexture() const { return baked_brdf; }

	inline resources::ResourceHandle<Mesh> getSkybox() const { return skybox; }
	inline resources::ResourceHandle<Mesh> getFullscreenQuad() const { return fullscreen_quad; }

	inline resources::ResourceHandle<Texture> getDefaultAlbedo() const { return default_albedo; }
	inline resources::ResourceHandle<Texture> getDefaultNormal() const { return default_normal; }
	inline resources::ResourceHandle<Texture> getDefaultRoughness() const { return default_roughness; }
	inline resources::ResourceHandle<Texture> getDefaultMetalness() const { return default_metalness; }

	inline resources::ResourceHandle<Shader> getShader(int index) const { return loaded_shaders[index]; }

	resources::ResourceHandle<Shader> loadShader(const resources::URI &uri, render::backend::ShaderType type);

	resources::ResourceHandle<Texture> loadTexture(const resources::URI &uri);
	resources::ResourceHandle<Texture> loadTextureFromMemory(const uint8_t *data, size_t size);

	resources::ResourceHandle<IBLTexture> loadIBLTexture(const resources::URI &uri);

	resources::ResourceHandle<Mesh> loadMesh(const resources::URI &uri);
	resources::ResourceHandle<Mesh> createMeshFromAssimp(const aiMesh *mesh);
	resources::ResourceHandle<Mesh> createMeshFromCGLTF(const cgltf_mesh *mesh);

	resources::ResourceHandle<RenderMaterial> createRenderMaterial(
		resources::ResourceHandle<Texture> albedo,
		resources::ResourceHandle<Texture> normal,
		resources::ResourceHandle<Texture> roughness,
		resources::ResourceHandle<Texture> metalness
	);

private:
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};
	resources::ResourceManager *resource_manager {nullptr};

	resources::ResourceHandle<Mesh> fullscreen_quad;
	resources::ResourceHandle<Mesh> skybox;

	resources::ResourceHandle<Texture> baked_brdf;
	resources::ResourceHandle<Texture> blue_noise;

	resources::ResourceHandle<Texture> default_albedo;
	resources::ResourceHandle<Texture> default_normal;
	resources::ResourceHandle<Texture> default_roughness;
	resources::ResourceHandle<Texture> default_metalness;

	std::vector<resources::ResourceHandle<Shader> > loaded_shaders;
	std::vector<resources::ResourceHandle<Texture> > loaded_textures;
	std::vector<resources::ResourceHandle<IBLTexture> > loaded_ibl_textures;
	std::vector<resources::ResourceHandle<Mesh> > loaded_meshes;
	std::vector<resources::ResourceHandle<RenderMaterial> > loaded_render_materials;
};
