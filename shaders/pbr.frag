#version 450
#pragma shader_stage(fragment)

#include "RenderState.inc"
#include "SceneTextures.inc"
#include "brdf.inc"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragTangentWS;
layout(location = 3) in vec3 fragBinormalWS;
layout(location = 4) in vec3 fragNormalWS;
layout(location = 5) in vec3 fragPositionWS;

layout(location = 0) out vec4 outColor;

struct MicrofacetMaterial
{
	vec3 albedo;
	float ao;
	float roughness;
	float metalness;
	vec3 f0;
};

vec3 ApproximateSpecularIBL(vec3 f0, vec3 view, vec3 normal, float roughness)
{
	float dotNV = max(0.0f, dot(normal, view));

	// TODO: remove magic constant, pass max mip levels instead
	float mip = roughness * 8.0f;
	vec3 prefilteredLi = textureLod(environmentSampler, -reflect(view, normal), mip).rgb;
	vec2 integratedBRDF = texture(bakedBRDFSampler, vec2(roughness, dotNV)).xy;

	return prefilteredLi * (f0 * integratedBRDF.x + integratedBRDF.y);
}

vec3 DirectBRDF(Surface surface, MicrofacetMaterial material)
{
	float D = D_GGX(surface, material.roughness);
	vec3 F = F_Shlick(surface, material.f0);
	float G = G_SmithGGX(surface, material.roughness);

	vec3 specular_reflection = D * F * G / (4.0f * surface.dotNV * surface.dotNL);
	vec3 diffuse_reflection = material.albedo * lerp(vec3(1.0f) - F, vec3(0.0f), material.metalness);

	return diffuse_reflection * iPI + specular_reflection;
}

vec3 IBLBRDF(Surface surface, MicrofacetMaterial material)
{
	vec3 F = F_Shlick(surface.dotNV, material.f0, material.roughness);
	vec3 irradiance = texture(diffuseIrradianceSampler, surface.normal).rgb * material.ao;

	vec3 diffuse_reflection = irradiance * material.albedo * lerp(vec3(1.0f) - F, vec3(0.0f), material.metalness);
	vec3 specular_reflection = ApproximateSpecularIBL(material.f0, surface.view, surface.normal, material.roughness);

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

	MicrofacetMaterial microfacet_material;
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

	vec3 light = DirectBRDF(surface, microfacet_material) * attenuation * 2.0f * surface.dotNL;

	// Ambient light (diffuse & specular IBL)
	vec3 ambient = IBLBRDF(surface, microfacet_material);

	// Result
	vec3 color = vec3(0.0f);
	color += ambient;
	color += light;

	// TODO: move to separate pass
	// Tonemapping + gamma correction
	// color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0f / 2.2f));

	outColor = vec4(color, 1.0f);
}
