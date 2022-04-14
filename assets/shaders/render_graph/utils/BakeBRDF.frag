#version 450

// Includes
#include <shaders/common/Common.h>
#include <shaders/render_graph/common/BRDF.h>

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out vec4 out_brdf;

//
vec2 integrateBRDF(float roughness, float dot_NV)
{
	vec3 view;
	view.x = sqrt(saturate(1.0f - dot_NV * dot_NV));
	view.y = 0.0f;
	view.z = dot_NV;

	vec3 normal = vec3(0.0f, 0.0f, 1.0f);

	float A = 0.0f;
	float B = 0.0f;

	const uint samples = 2048;

	Surface surface;
	surface.view = view;
	surface.normal = normal;
	surface.dotNV = dot_NV;

	for (uint i = 0; i < samples; ++i)
	{
		vec2 Xi = hammersley(i, samples);

		surface.halfVector = importanceSamplingGGX(Xi, surface.normal, roughness);
		surface.light = -reflect(surface.view, surface.halfVector);

		surface.dotNH = max(0.0f, dot(surface.normal, surface.halfVector));
		surface.dotNL = max(0.0f, dot(surface.normal, surface.light));
		surface.dotHV = max(0.0f, dot(surface.halfVector, surface.view));

		float G = G_SmithGGX(surface, roughness);
		float F = F_Shlick(surface);

		float fr = (G * surface.dotHV) / (surface.dotNH * surface.dotNV + EPSILON);

		A += (1 - F) * fr;
		B += F * fr;
	}

	return vec2(A, B) / float(samples);
}

void main()
{
	vec2 baked_brdf = integrateBRDF(in_uv.x, in_uv.y);
	out_brdf = vec4(baked_brdf.x, baked_brdf.y, 0.0f, 0.0f);
}
