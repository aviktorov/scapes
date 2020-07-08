#pragma once

#include <render/backend/driver.h>

namespace render
{
	class Mesh;
	class Shader;
	class Texture;
}

class Light
{
public:
	Light(
		render::backend::Driver *driver,
		const render::Shader *vertex,
		const render::Shader *fragment
	);
	virtual ~Light();

	inline const render::Mesh *getMesh() const { return mesh; }
	inline const render::Shader *getVertexShader() const { return vertex_shader; }
	inline const render::Shader *getFragmentShader() const { return fragment_shader; }
	inline render::backend::BindSet *getBindSet() const { return bind_set; }

protected:
	render::backend::Driver *driver {nullptr};
	render::backend::BindSet *bind_set {nullptr};

	render::Mesh *mesh {nullptr};
	const render::Shader *vertex_shader {nullptr};
	const render::Shader *fragment_shader {nullptr};
};

/*
 */
class SkyLight : public Light
{
public:
	SkyLight(
		render::backend::Driver *driver,
		const render::Shader *vertex,
		const render::Shader *fragment
	);
	virtual ~SkyLight();

	void setBakedBRDFTexture(const render::Texture *brdf_texture);
	void setEnvironmentCubemap(const render::Texture *environment_cubemap);
	void setIrradianceCubemap(const render::Texture *irradiance_cubemap);
};
