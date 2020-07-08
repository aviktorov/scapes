#version 450
#pragma shader_stage(fragment)

#include <common/RenderState.inc>
#include <common/brdf.inc>

#define SKYLIGHT_SET 2
#include <lights/skylight.inc>

#include "SceneTextures.inc"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragTangentWS;
layout(location = 3) in vec3 fragBinormalWS;
layout(location = 4) in vec3 fragNormalWS;
layout(location = 5) in vec3 fragPositionWS;

layout(location = 0) out vec4 outColor;

vec3 DirectBRDF(Surface surface, SurfaceMaterial material)
{
	vec3 diffuse;
	vec3 specular;
	GetBRDF(surface, material, diffuse, specular);

	return diffuse + specular;
}

vec3 IBLBRDF(Surface surface, SurfaceMaterial material)
{
	vec3 diffuse_reflection = SkyLight_Diffuse(surface.normal, surface.view, material);
	vec3 specular_reflection = SkyLight_Specular(surface.normal, surface.view, material);

	return diffuse_reflection + specular_reflection;
}

void main()
{
	vec3 lightPosWS = ubo.cameraPosWS;
	vec3 lightDirWS = normalize(lightPosWS - fragPositionWS);
	vec3 cameraDirWS = normalize(ubo.cameraPosWS - fragPositionWS);

	vec3 normal = texture(normalSampler, fragTexCoord).xyz * 2.0f - vec3(1.0f);

	mat3 m;
	m[0] = normalize(fragTangentWS);
	m[1] = normalize(-fragBinormalWS);
	m[2] = normalize(fragNormalWS);

	Surface surface;
	surface.light = lightDirWS;
	surface.view = cameraDirWS;
	surface.normal = normalize(m * normal);
	surface.halfVector = normalize(lightDirWS + cameraDirWS);
	surface.dotNH = max(0.0f, dot(surface.normal, surface.halfVector));
	surface.dotNL = max(0.0f, dot(surface.normal, surface.light));
	surface.dotNV = max(0.0f, dot(surface.normal, surface.view));
	surface.dotHV = max(0.0f, dot(surface.halfVector, surface.view));

	SurfaceMaterial microfacet_material;
	microfacet_material.albedo = texture(albedoSampler, fragTexCoord).rgb;
	microfacet_material.albedo = pow(microfacet_material.albedo, vec3(2.2f));
	microfacet_material.roughness = texture(shadingSampler, fragTexCoord).g;
	microfacet_material.metalness = texture(shadingSampler, fragTexCoord).b;
	microfacet_material.ao = texture(aoSampler, fragTexCoord).r;
	microfacet_material.ao = pow(microfacet_material.ao, 2.2f);

	microfacet_material.albedo = lerp(microfacet_material.albedo, vec3(0.5f, 0.5f, 0.5f), ubo.lerpUserValues);
	microfacet_material.roughness = lerp(microfacet_material.roughness, ubo.userRoughness, ubo.lerpUserValues);
	microfacet_material.metalness = lerp(microfacet_material.metalness, ubo.userMetalness, ubo.lerpUserValues);

	microfacet_material.f0 = lerp(vec3(0.04f), microfacet_material.albedo, microfacet_material.metalness);

	// Direct light
	float attenuation = 1.0f / dot(lightPosWS - fragPositionWS, lightPosWS - fragPositionWS);
	float Li = 2.0f * attenuation;

	vec3 direct_brdf = DirectBRDF(surface, microfacet_material);
	vec3 light = direct_brdf * Li * surface.dotNL;

	// Ambient light (diffuse & specular IBL)
	vec3 ambient = IBLBRDF(surface, microfacet_material);

	// Result
	vec3 color = vec3(0.0f);
	color += ambient;
	color += light;

	// TODO: move to separate pass
	// Tonemapping + gamma correction
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0f / 2.2f));

	outColor = vec4(color, 1.0f);
}
