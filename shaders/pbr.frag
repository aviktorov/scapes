#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 world;
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
} ubo;

layout(binding = 1) uniform sampler2D albedoSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D aoSampler;
layout(binding = 4) uniform sampler2D shadingSampler;
layout(binding = 5) uniform sampler2D emissionSampler;
layout(binding = 6) uniform samplerCube environmentSampler;
layout(binding = 7) uniform samplerCube diffuseIrradianceSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragTangentWS;
layout(location = 3) in vec3 fragBinormalWS;
layout(location = 4) in vec3 fragNormalWS;
layout(location = 5) in vec3 fragPositionWS;

layout(location = 0) out vec4 outColor;

const float PI = 3.141592653589798979f;
const float iPI = 0.31830988618379f;

struct Surface
{
	vec3 light;
	vec3 view;
	vec3 normal;
	vec3 halfVector;
	float dotNH;
	float dotNL;
	float dotNV;
	float dotHV;
};

float sqr(float a)
{
	return a * a;
}

float lerp(float a, float b, float t)
{
	return a * (1.0f - t) + b * t;
}

vec3 lerp(vec3 a, vec3 b, float t)
{
	return a * (1.0f - t) + b * t;
}

//////////////// Microfacet world

struct MicrofacetMaterial
{
	vec3 albedo;
	float roughness;
	float metalness;
	vec3 f0;
};

float D_GGX(Surface surface, float roughness)
{
	float alpha2 = sqr(roughness * roughness);

	return alpha2 / (PI * sqr(1.0f + surface.dotNH * surface.dotNH * (alpha2 - 1.0f)));
}

float G_SmithGGX_Normalized(Surface surface, float roughness)
{
	float alpha2 = sqr(roughness * roughness);

	float ggx_NV = surface.dotNV + sqrt(alpha2 + (1.0f - alpha2) * surface.dotNV * surface.dotNV);
	float ggx_NL = surface.dotNL + sqrt(alpha2 + (1.0f - alpha2) * surface.dotNL * surface.dotNL);

	return 1.0f / (ggx_NV * ggx_NL);
}

vec3 F_Shlick(Surface surface, vec3 f0)
{
	return f0 + (vec3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - surface.dotHV, 5);
}

vec3 F_Shlick(float cosTheta, vec3 f0, float roughness)
{
	return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0f - cosTheta, 5);
}

vec3 MicrofacetBRDF(Surface surface, MicrofacetMaterial material)
{
	float D = D_GGX(surface, material.roughness);
	vec3 F = F_Shlick(surface, material.f0);
	float G_normalized = G_SmithGGX_Normalized(surface, material.roughness);

	vec3 specular_reflection = D * F * G_normalized;
	vec3 diffuse_reflection = material.albedo * lerp(vec3(1.0f) - F, vec3(0.0f), material.metalness);

	return (diffuse_reflection * iPI + specular_reflection);
}

void main() {
	vec3 lightPos = ubo.cameraPos;
	vec3 lightDirWS = normalize(lightPos - fragPositionWS);
	vec3 cameraDirWS = normalize(ubo.cameraPos - fragPositionWS);

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
	microfacet_material.f0 = lerp(vec3(0.04f), microfacet_material.albedo, microfacet_material.metalness);

	// Direct light
	float attenuation = 1.0f / dot(lightPos - fragPositionWS, lightPos - fragPositionWS);

	vec3 light = MicrofacetBRDF(surface, microfacet_material) * attenuation * 2.0f * surface.dotNL;

	// Ambient light (diffuse IBL)
	vec3 ambient = texture(diffuseIrradianceSampler, ibl.light).rgb;
	ambient *= texture(aoSampler, fragTexCoord).r;
	ambient *= (1.0f - F_Shlick(ibl.dotNV, microfacet_material.f0, microfacet_material.roughness));

	// Result
	vec3 color = ambient;
	color += light;
	color += texture(emissionSampler, fragTexCoord).rgb;

	// TODO: move to separate pass
	// Tonemapping + gamma correction
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	outColor = vec4(color, 1.0f);
}
