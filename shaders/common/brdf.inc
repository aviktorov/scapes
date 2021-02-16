#ifndef BRDF_H_
#define BRDF_H_

const float PI =  3.141592653589798979f;
const float PI2 = 6.283185307179586477f;
const float iPI = 0.318309886183790672f;
const float EPSILON = 1e-6f;

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

struct SurfaceMaterial
{
	vec3 albedo;
	float ao;
	float roughness;
	float metalness;
	vec3 f0;
};

float sqr(float a)
{
	return a * a;
}

float lerp(float a, float b, float t)
{
	return a * (1.0f - t) + b * t;
}

vec2 lerp(vec2 a, vec2 b, float t)
{
	return a * (1.0f - t) + b * t;
}

vec3 lerp(vec3 a, vec3 b, float t)
{
	return a * (1.0f - t) + b * t;
}

vec4 lerp(vec4 a, vec4 b, float t)
{
	return a * (1.0f - t) + b * t;
}

vec3 lerp(vec3 a, vec3 b, vec3 t)
{
	return a * (1.0f - t) + b * t;
}

float saturate(float v)
{
	return clamp(v, 0.0f, 1.0f);
}

vec2 saturate(vec2 v)
{
	vec2 ret;
	ret.x = clamp(v.x, 0.0f, 1.0f);
	ret.y = clamp(v.y, 0.0f, 1.0f);
	return ret;
}

vec3 saturate(vec3 v)
{
	vec3 ret;
	ret.x = clamp(v.x, 0.0f, 1.0f);
	ret.y = clamp(v.y, 0.0f, 1.0f);
	ret.z = clamp(v.z, 0.0f, 1.0f);
	return ret;
}

vec3 pow(vec3 v, float power)
{
	vec3 ret;
	ret.x = pow(v.x, power);
	ret.y = pow(v.y, power);
	ret.z = pow(v.z, power);
	return ret;
}

int getTextureNumMips(in sampler2D tex)
{
	ivec2 size = textureSize(tex, 0);
	return 1 + int(floor(log2(max(size.x, size.y))));
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), float(bitfieldReverse(i)) * 2.3283064365386963e-10);
}

vec3 ImportanceSamplingGGX(vec2 Xi, vec3 normal, float roughness)
{
	float alpha = saturate(roughness * roughness);
	float alpha2 = saturate(sqr(alpha));

	Xi.y = saturate(Xi.y - EPSILON);

	float phi = PI2 * Xi.x;
	float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (alpha2 - 1.0f) * Xi.y));
	float sinTheta = sqrt(1.0f - sqr(cosTheta));

	// from spherical coordinates to cartesian coordinates
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, normal));
	vec3 binormal = cross(normal, tangent);

	vec3 direction = tangent * H.x + binormal * H.y + normal * H.z;
	return normalize(direction);
}

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

	float ggx_NV = surface.dotNV + sqrt(alpha2 + (1.0f - alpha2) * surface.dotNV * surface.dotNV);
	float ggx_NL = surface.dotNL + sqrt(alpha2 + (1.0f - alpha2) * surface.dotNL * surface.dotNL);

	return 1.0f / (ggx_NV * ggx_NL + EPSILON);
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

vec3 Diffuse_Lambert(SurfaceMaterial material)
{
	return vec3(material.albedo * iPI);
}

vec3 Specular_CookTorrance(Surface surface, SurfaceMaterial material, out vec3 F)
{
	F = F_Shlick(surface, material.f0);

	float D = D_GGX(surface, material.roughness);
	float G = G_SmithGGX_Normalized(surface, material.roughness);

	return D * F * G;
}

void GetBRDF(Surface surface, SurfaceMaterial material, out vec3 diffuse, out vec3 specular)
{
	vec3 F;
	diffuse = Diffuse_Lambert(material);
	specular = Specular_CookTorrance(surface, material, F);

	diffuse *= lerp(vec3(1.0f) - F, vec3(0.0f), material.metalness);
}

#endif // BRDF_H_
