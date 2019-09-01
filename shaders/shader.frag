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

//////////////// Vanilla world

struct VanillaMaterial
{
	float shininess;
	vec3 color;
};

vec3 DiffuseLambertBRDF(Surface surface, VanillaMaterial material)
{
	float dotNL = max(dot(surface.normal, surface.light), 0.0f);
	return vec3(1.0f, 1.0f, 1.0f) * dotNL;
}

vec3 DiffuseHalfLambertBRDF(Surface surface, VanillaMaterial material)
{
	float dotNL = max(dot(surface.normal, surface.light), 0.0f);
	float halfLambert = (dotNL + 1) * 0.5f;
	return vec3(1.0f, 1.0f, 1.0f) * halfLambert * halfLambert;
}

vec3 SpecularBlinnPhongBRDF(Surface surface, VanillaMaterial material)
{
	vec3 h = normalize(surface.light + surface.view);
	float dotNH = max(dot(surface.normal, h), 0.0f);
	return vec3(1.0f, 1.0f, 1.0f) * pow(dotNH, material.shininess);
}

vec3 SpecularPhongBRDF(Surface surface, VanillaMaterial material)
{
	vec3 r = reflect(-surface.light, surface.normal);
	float dotRV = max(dot(r, surface.view), 0.0f);
	return vec3(1.0f, 1.0f, 1.0f) * pow(dotRV, material.shininess / 4.0f);
}

vec3 VanillaBRDF(Surface surface, VanillaMaterial material)
{
	vec3 diffuse = DiffuseHalfLambertBRDF(surface, material);
	vec3 specular = SpecularBlinnPhongBRDF(surface, material);

	return diffuse * material.color + specular;
}

//////////////// Microfacet world

struct MicrofacetMaterial
{
	vec3 albedo;
	float roughness;
	float metalness;
};

float D_GGX(Surface surface, float roughness)
{
	float alpha2 = sqr(roughness * roughness);
	float dotNH = dot(surface.normal, surface.halfVector);

	return alpha2 * iPI / sqr(1.0f + dotNH * dotNH * (alpha2 - 1.0f));
}

float G_SmithGGX(Surface surface, float roughness)
{
	float dotNL = max(dot(surface.normal, surface.light), 0.0f);
	float dotNV = max(dot(surface.normal, surface.view), 0.0f);
	float dotNH = max(dot(surface.normal, surface.halfVector), 0.0f);

	float alpha2 = sqr(roughness * roughness);

	float ggx_VH = 2.0f * dotNV / (dotNV + sqrt(alpha2 + (1.0f - alpha2) * dotNV * dotNV));
	float ggx_LH = 2.0f * dotNL / (dotNL + sqrt(alpha2 + (1.0f - alpha2) * dotNL * dotNL));

	return ggx_VH * ggx_LH;
}

vec3 F_Shlick(Surface surface, vec3 f0)
{
	float dotHV = max(dot(surface.halfVector, surface.view), 0.0f);

	return f0 + (vec3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - dotHV, 5);
}

vec3 MicrofacetMetalBRDF(Surface surface, MicrofacetMaterial material)
{
	float dotNL = max(dot(surface.normal, surface.light), 0.0f);
	float dotNV = max(dot(surface.normal, surface.view), 0.0f);

	float D = D_GGX(surface, material.roughness);
	float G = G_SmithGGX(surface, material.roughness);
	vec3 F = F_Shlick(surface, material.albedo);

	return D * G * F / (4.0f * dotNL * dotNV);
}

vec3 MicrofacetDielectricBDRF(Surface surface, MicrofacetMaterial material)
{
	vec3 f0_dielectric = vec3(0.04f, 0.04f, 0.04f);

	float dotNL = max(dot(surface.normal, surface.light), 0.0f);
	float dotNV = max(dot(surface.normal, surface.view), 0.0f);

	float D = D_GGX(surface, material.roughness);
	float G = G_SmithGGX(surface, material.roughness);
	vec3 F = F_Shlick(surface, f0_dielectric);

	return D * G * F / (4.0f * dotNL * dotNV) + material.albedo * iPI;
}

vec3 MicrofacetBRDF(Surface surface, MicrofacetMaterial material)
{
	vec3 metal = MicrofacetMetalBRDF(surface, material);
	vec3 dielectric = MicrofacetDielectricBDRF(surface, material);

	return lerp(metal, dielectric, material.metalness);
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

	if (true) {
		MicrofacetMaterial microfacet_material;
		microfacet_material.albedo = texture(albedoSampler, fragTexCoord).rgb;
		microfacet_material.roughness = texture(shadingSampler, fragTexCoord).b;
		microfacet_material.metalness = texture(shadingSampler, fragTexCoord).g;

		vec3 microfacet_bdrf = MicrofacetBRDF(surface, microfacet_material);
		outColor = vec4(microfacet_bdrf, 1.0f);
	}
	else
	{
		VanillaMaterial vanilla_material;
		vanilla_material.shininess = 10.0f;
		vanilla_material.color = texture(albedoSampler, fragTexCoord).rgb;

		vec3 vanilla_brdf = VanillaBRDF(surface, vanilla_material);
		outColor = vec4(vanilla_brdf, 1.0f);
	}

	outColor *= texture(aoSampler, fragTexCoord);
	outColor += texture(emissionSampler, fragTexCoord);
}
