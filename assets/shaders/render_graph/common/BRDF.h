#ifndef BRDF_H_
#define BRDF_H_

#include <shaders/common/Common.h>

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

struct Material
{
	vec3 albedo;
	vec3 f0;
	float ao;
	float roughness;
	float metalness;
};

float D_GGX(Surface surface, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = saturate(sqr(alpha));

	return alpha2 / (PI * sqr(1.0f + surface.dotNH * surface.dotNH * (alpha2 - 1.0f)));
}

float G_SmithGGX_Normalized(Surface surface, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = saturate(sqr(alpha));

	float ggxNV = surface.dotNV + sqrt(alpha2 + (1.0f - alpha2) * surface.dotNV * surface.dotNV);
	float ggxNL = surface.dotNL + sqrt(alpha2 + (1.0f - alpha2) * surface.dotNL * surface.dotNL);

	return 1.0f / (ggxNV * ggxNL + EPSILON);
}

float G_SmithGGX(Surface surface, float roughness)
{
	return 4.0f * surface.dotNV * surface.dotNL * G_SmithGGX_Normalized(surface, roughness);
}

float F_Shlick(Surface surface)
{
	return pow(1.0f - surface.dotHV, 5.0f);
}

vec3 F_Shlick(Surface surface, vec3 f0)
{
	return f0 + (vec3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - surface.dotHV, 5.0f);
}

vec3 F_Shlick(float cosTheta, vec3 f0, float roughness)
{
	return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 Diffuse_Lambert(Material material)
{
	return vec3(material.albedo * iPI);
}

vec3 Specular_CookTorrance(Surface surface, Material material, out vec3 F)
{
	F = F_Shlick(surface, material.f0);

	float D = D_GGX(surface, material.roughness);
	float G = G_SmithGGX_Normalized(surface, material.roughness);

	return D * F * G;
}

void GetBRDF(Surface surface, Material material, out vec3 diffuse, out vec3 specular)
{
	vec3 F;
	diffuse = Diffuse_Lambert(material);
	specular = Specular_CookTorrance(surface, material, F);

	diffuse *= mix(vec3(1.0f) - F, vec3(0.0f), material.metalness);
}

#endif // BRDF_H_
