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

	inline resources::ResourceHandle<Texture> getHDRTexture(int index) const { return hdr_cubemaps[index]; }
	inline resources::ResourceHandle<Texture> getHDREnvironmentCubemap(int index) const { return environment_cubemaps[index]; }
	inline resources::ResourceHandle<Texture> getHDRIrradianceCubemap(int index) const { return irradiance_cubemaps[index]; }
	const char *getHDRTexturePath(int index) const;
	size_t getNumHDRTextures() const;

	inline resources::ResourceHandle<Texture> getBakedBRDFTexture() const { return baked_brdf; }

	inline resources::ResourceHandle<Mesh> getSkybox() const { return skybox; }
	inline resources::ResourceHandle<Mesh> getFullscreenQuad() const { return fullscreen_quad; }

	inline render::backend::Texture *getDefaultAlbedo() const { return default_albedo;}
	inline render::backend::Texture *getDefaultNormal() const { return default_normal;}
	inline render::backend::Texture *getDefaultRoughness() const { return default_roughness;}
	inline render::backend::Texture *getDefaultMetalness() const { return default_metalness;}

	inline resources::ResourceHandle<Shader> getShader(int index) const { return loaded_shaders[index]; }
	
	resources::ResourceHandle<Shader> loadShader(const resources::URI &uri, render::backend::ShaderType type);

	resources::ResourceHandle<Texture> loadTexture(const resources::URI &uri);
	resources::ResourceHandle<Texture> loadTextureFromMemory(const uint8_t *data, size_t size);

	resources::ResourceHandle<Mesh> loadMesh(const resources::URI &uri);
	resources::ResourceHandle<Mesh> createMeshFromAssimp(const aiMesh *mesh);
	resources::ResourceHandle<Mesh> createMeshFromCGLTF(const cgltf_mesh *mesh);

private:
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};
	resources::ResourceManager *resource_manager {nullptr};

	resources::ResourceHandle<Mesh> fullscreen_quad;
	resources::ResourceHandle<Mesh> skybox;

	resources::ResourceHandle<Texture> baked_brdf;
	std::vector<resources::ResourceHandle<Texture> > hdr_cubemaps;
	std::vector<resources::ResourceHandle<Texture> > environment_cubemaps;
	std::vector<resources::ResourceHandle<Texture> > irradiance_cubemaps;

	resources::ResourceHandle<Texture> blue_noise;

	render::backend::Texture *default_albedo {nullptr};
	render::backend::Texture *default_normal {nullptr};
	render::backend::Texture *default_roughness {nullptr};
	render::backend::Texture *default_metalness {nullptr};

	std::vector<resources::ResourceHandle<Shader> > loaded_shaders;
	std::vector<resources::ResourceHandle<Texture> > loaded_textures;
	std::vector<resources::ResourceHandle<Mesh> > loaded_meshes;
};
