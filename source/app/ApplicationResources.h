#pragma once

#include <unordered_map>
#include <render/backend/Driver.h>
#include <common/ResourceManager.h>

namespace render::shaders
{
	class Compiler;
}

class Mesh;
class Shader;
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

class Mesh;
class Shader;
struct Texture;

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

	inline const resources::ResourceHandle<Texture> getBlueNoiseTexture() const { return blue_noise; }

	inline const resources::ResourceHandle<Texture> getHDRTexture(int index) const { return hdr_cubemaps[index]; }
	inline const resources::ResourceHandle<Texture> getHDREnvironmentCubemap(int index) const { return environment_cubemaps[index]; }
	inline const resources::ResourceHandle<Texture> getHDRIrradianceCubemap(int index) const { return irradiance_cubemaps[index]; }
	const char *getHDRTexturePath(int index) const;
	size_t getNumHDRTextures() const;

	inline const resources::ResourceHandle<Texture> getBakedBRDFTexture() const { return baked_brdf; }

	inline const Mesh *getSkybox() const { return skybox; }
	inline const Mesh *getFullscreenQuad() const { return fullscreen_quad; }

	inline render::backend::Texture *getDefaultAlbedo() const { return default_albedo;}
	inline render::backend::Texture *getDefaultNormal() const { return default_normal;}
	inline render::backend::Texture *getDefaultRoughness() const { return default_roughness;}
	inline render::backend::Texture *getDefaultMetalness() const { return default_metalness;}

	void reloadShaders();

	Mesh *getMesh(int id) const;
	Mesh *loadMesh(int id, const char *path);
	void unloadMesh(int id);

	Shader *getShader(int id) const;
	Shader *loadShader(int id, render::backend::ShaderType type, const char *path);
	bool reloadShader(int id);
	void unloadShader(int id);

	resources::ResourceHandle<Texture> loadTexture(const resources::URI &uri);
	resources::ResourceHandle<Texture> loadTextureFromMemory(const uint8_t *data, size_t size);

private:
	render::backend::Driver *driver {nullptr};
	render::shaders::Compiler *compiler {nullptr};
	resources::ResourceManager *resource_manager {nullptr};

	Mesh *fullscreen_quad {nullptr};
	Mesh *skybox {nullptr};

	resources::ResourceHandle<Texture> baked_brdf;
	std::vector<resources::ResourceHandle<Texture> > hdr_cubemaps;
	std::vector<resources::ResourceHandle<Texture> > environment_cubemaps;
	std::vector<resources::ResourceHandle<Texture> > irradiance_cubemaps;

	resources::ResourceHandle<Texture> blue_noise;

	render::backend::Texture *default_albedo {nullptr};
	render::backend::Texture *default_normal {nullptr};
	render::backend::Texture *default_roughness {nullptr};
	render::backend::Texture *default_metalness {nullptr};

	std::unordered_map<int, Mesh *> meshes;
	std::unordered_map<int, Shader *> shaders;

	std::vector<resources::ResourceHandle<Texture> > loaded_textures;
};
