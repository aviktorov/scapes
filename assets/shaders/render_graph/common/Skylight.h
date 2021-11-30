#ifndef SKYLIGHT_INC_
#define SKYLIGHT_INC_

#include <shaders/render_graph/common/BRDF.h>

layout(set = RENDER_GRAPH_SKYLIGHT_SET, binding = 0) uniform sampler2D tex_skylight_brdf;
layout(set = RENDER_GRAPH_SKYLIGHT_SET, binding = 1) uniform samplerCube tex_skylight_environment;
layout(set = RENDER_GRAPH_SKYLIGHT_SET, binding = 2) uniform samplerCube tex_skylight_irradiance;

vec3 SkyLight_Diffuse(vec3 normal_ws, vec3 view_ws, Material material)
{
	float dot_NV = max(0.0f, dot(normal_ws, view_ws));

	vec3 F = F_Shlick(dot_NV, material.f0, material.roughness);
	vec3 irradiance = texture(tex_skylight_irradiance, normal_ws).rgb;

	vec3 result = material.albedo * material.ao;
	result *= mix(vec3(1.0f) - F, vec3(0.0f), material.metalness);
	result *= irradiance;

	return result;
}

vec3 SkyLight_Specular(vec3 normal_ws, vec3 view_ws, Material material)
{
	float dot_NV = max(0.0f, dot(normal_ws, view_ws));

	// TODO: remove magic constant, pass max mip levels instead
	float mip = material.roughness * 8.0f;
	vec3 Li = textureLod(tex_skylight_environment, -reflect(view_ws, normal_ws), mip).rgb;
	vec2 brdf = texture(tex_skylight_brdf, vec2(material.roughness, dot_NV)).xy;

	return Li * (material.f0 * brdf.x + brdf.y);
}

#endif // SKYLIGHT_INC_
