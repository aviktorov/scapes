#pragma once

#include <render/backend/Driver.h>

class Mesh;
class Shader;
class Texture;

class Light
{
public:
	Light(
		render::backend::Driver *driver,
		const Shader *vertex,
		const Shader *fragment
	);
	virtual ~Light();

	inline const Mesh *getMesh() const { return mesh; }
	inline const Shader *getVertexShader() const { return vertex_shader; }
	inline const Shader *getFragmentShader() const { return fragment_shader; }
	inline render::backend::BindSet *getBindSet() const { return bind_set; }

protected:
	render::backend::Driver *driver {nullptr};
	render::backend::BindSet *bind_set {nullptr};

	Mesh *mesh {nullptr};
	const Shader *vertex_shader {nullptr};
	const Shader *fragment_shader {nullptr};
};

/*
 */
class SkyLight : public Light
{
public:
	SkyLight(
		render::backend::Driver *driver,
		const Shader *vertex,
		const Shader *fragment
	);
	virtual ~SkyLight();

	void setBakedBRDFTexture(const Texture *brdf_texture);
	void setEnvironmentCubemap(const Texture *environment_cubemap);
	void setIrradianceCubemap(const Texture *irradiance_cubemap);
};
