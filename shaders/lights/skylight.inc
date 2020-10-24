#ifndef SKYLIGHT_INC_
#define SKYLIGHT_INC_

#include <common/brdf.inc>

layout(set = SKYLIGHT_SET, binding = 0) uniform sampler2D skyLightBakedBRDF;
layout(set = SKYLIGHT_SET, binding = 1) uniform samplerCube skyLightEnvironmentCube;
layout(set = SKYLIGHT_SET, binding = 2) uniform samplerCube skyLightIrradianceCube;

vec3 SkyLight_Diffuse(vec3 normalWS, vec3 viewWS, SurfaceMaterial material)
{
	float dotNV = max(0.0f, dot(normalWS, viewWS));

	vec3 F = F_Shlick(dotNV, material.f0, material.roughness);
	vec3 irradiance = texture(skyLightIrradianceCube, normalWS).rgb;

	vec3 result = material.albedo * material.ao;
	result *= lerp(vec3(1.0f) - F, vec3(0.0f), material.metalness);
	result *= irradiance;

	return result;
}

vec3 SkyLight_Specular(vec3 normalWS, vec3 viewWS, SurfaceMaterial material)
{
	float dotNV = max(0.0f, dot(normalWS, viewWS));

	// TODO: remove magic constant, pass max mip levels instead
	float mip = material.roughness * 8.0f;
	vec3 Li = textureLod(skyLightEnvironmentCube, -reflect(viewWS, normalWS), mip).rgb;
	vec2 brdf = texture(skyLightBakedBRDF, vec2(material.roughness, dotNV)).xy;

	return Li * (material.f0 * brdf.x + brdf.y);
}

#endif // SKYLIGHT_INC_
