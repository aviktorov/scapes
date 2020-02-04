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
	float roughness;
	float metalness;
	vec3 f0;
};

vec3 MicrofacetBRDF(Surface surface, MicrofacetMaterial material)
{
	float D = D_GGX(surface, material.roughness);
	vec3 F = F_Shlick(surface, material.f0);
	float G_normalized = G_SmithGGX_Normalized(surface, material.roughness);

	vec3 specular_reflection = D * F * G_normalized;
	vec3 diffuse_reflection = material.albedo * lerp(vec3(1.0f) - F, vec3(0.0f), material.metalness);

	return (diffuse_reflection * iPI + specular_reflection);
}

vec3 SpecularIBL(Surface surface, MicrofacetMaterial material)
{
	vec3 result = vec3(0.0);
	const uint samples = 128;

	Surface sample_surface;
	sample_surface.view = surface.view;
	sample_surface.normal = surface.normal;
	sample_surface.dotNV = max(0.0f, dot(sample_surface.normal, sample_surface.view));

	for (uint i = 0; i < samples; ++i)
	{
		vec2 Xi = Hammersley(i, samples);

		sample_surface.halfVector = ImportanceSamplingGGX(Xi, sample_surface.normal, material.roughness);
		sample_surface.light = -reflect(sample_surface.view, sample_surface.halfVector);

		sample_surface.dotNH = max(0.0f, dot(sample_surface.normal, sample_surface.halfVector));
		sample_surface.dotNL = max(0.0f, dot(sample_surface.normal, sample_surface.light));
		sample_surface.dotHV = max(0.0f, dot(sample_surface.halfVector, sample_surface.view));

		float D = D_GGX(surface, material.roughness);
		vec3 F = F_Shlick(sample_surface, material.f0);
		float G_normalized = G_SmithGGX(surface, material.roughness);

		vec3 Li = texture(environmentSampler, sample_surface.light).rgb;
		vec3 specular_brdf = D * F * G_normalized;

		float pdf = D * sample_surface.dotNH / (4.0f * sample_surface.dotHV); // ?

		result += specular_brdf * Li * sample_surface.dotNL / pdf;
	}

	return result / float(samples);
}

vec3 PrefilterSpecularEnvMap(vec3 view, vec3 normal, float roughness)
{
	vec3 result = vec3(0.0);

	float weight = 0.0f;

	const uint samples = 256;
	for (uint i = 0; i < samples; ++i)
	{
		vec2 Xi = Hammersley(i, samples);

		vec3 halfVector = ImportanceSamplingGGX(Xi, normal, roughness);
		vec3 light = -reflect(view, halfVector);

		float dotNL = max(0.0f, dot(normal, light));
		vec3 Li = texture(environmentSampler, light).rgb;

		result += Li * dotNL;
		weight += dotNL;
	}

	return result / weight;
}

vec3 ApproximateSpecularIBL(vec3 f0, vec3 view, vec3 normal, float roughness)
{
	float dotNV = max(0.0f, dot(normal, view));

	vec3 prefilteredLi = PrefilterSpecularEnvMap(view, normal, roughness);
	vec2 integratedBRDF = texture(bakedBRDFSampler, vec2(roughness, dotNV)).xy;

	return prefilteredLi * (f0 * integratedBRDF.x + integratedBRDF.y);
}

void main() {
	vec3 lightPosWS = ubo.cameraPosWS;
	vec3 lightDirWS = normalize(lightPosWS - fragPositionWS);
	vec3 cameraDirWS = normalize(ubo.cameraPosWS - fragPositionWS);

	vec3 normal = texture(normalSampler, fragTexCoord).xyz * 2.0f - vec3(1.0f, 1.0f, 1.0f);

	mat3 m;
	m[0] = normalize(fragTangentWS);
	m[1] = normalize(fragBinormalWS);
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

	Surface ibl;
	ibl.light = reflect(-surface.view, surface.normal);
	ibl.view = cameraDirWS;
	ibl.normal = normalize(m * normal);
	ibl.halfVector = normalize(lightDirWS + cameraDirWS);
	ibl.dotNH = max(0.0f, dot(ibl.normal, ibl.halfVector));
	ibl.dotNL = max(0.0f, dot(ibl.normal, ibl.light));
	ibl.dotNV = max(0.0f, dot(ibl.normal, ibl.view));
	ibl.dotHV = max(0.0f, dot(ibl.halfVector, ibl.view));

	MicrofacetMaterial microfacet_material;
	microfacet_material.albedo = texture(albedoSampler, fragTexCoord).rgb;
	microfacet_material.roughness = texture(shadingSampler, fragTexCoord).g;
	microfacet_material.metalness = texture(shadingSampler, fragTexCoord).b;

	microfacet_material.albedo = lerp(microfacet_material.albedo, vec3(0.5f, 0.5f, 0.5f), ubo.lerpUserValues);
	microfacet_material.roughness = lerp(microfacet_material.roughness, ubo.userRoughness, ubo.lerpUserValues);
	microfacet_material.metalness = lerp(microfacet_material.metalness, ubo.userMetalness, ubo.lerpUserValues);

	microfacet_material.f0 = lerp(vec3(0.04f), microfacet_material.albedo, microfacet_material.metalness);

	// Direct light
	float attenuation = 1.0f / dot(lightPosWS - fragPositionWS, lightPosWS - fragPositionWS);

	vec3 light = MicrofacetBRDF(surface, microfacet_material) * attenuation * 2.0f * surface.dotNL;

	// Ambient light (diffuse & specular IBL)
	vec3 ibl_diffuse = texture(diffuseIrradianceSampler, ibl.light).rgb * microfacet_material.albedo;
	ibl_diffuse *= lerp(vec3(1.0f) - F_Shlick(ibl.dotNV, microfacet_material.f0, microfacet_material.roughness), vec3(0.0f), microfacet_material.metalness);

	vec3 ibl_specular = ApproximateSpecularIBL(microfacet_material.f0, ibl.view, ibl.normal, microfacet_material.roughness);

	vec3 ambient = ibl_diffuse * iPI + ibl_specular;
	ambient *= texture(aoSampler, fragTexCoord).r;

	// Result
	vec3 color = vec3(0.0f);
	color += ambient;
	// color += light;
	// color += texture(emissionSampler, fragTexCoord).rgb;

	// TODO: move to separate pass
	// Tonemapping + gamma correction
	color = color / (color + vec3(1.0));
	// color = pow(color, vec3(1.0f / 2.2f));

	outColor = vec4(color, 1.0f);
}
