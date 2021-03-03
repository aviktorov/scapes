#version 450
#pragma shader_stage(fragment)

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec2 IntegrateBRDF(float roughness, float dotNV)
{
	vec3 view;
	view.x = sqrt(saturate(1.0f - dotNV * dotNV));
	view.y = 0.0f;
	view.z = dotNV;

	vec3 normal = vec3(0.0f, 0.0f, 1.0f);

	float A = 0.0f;
	float B = 0.0f;

	const uint samples = 2048;

	Surface sample_surface;
	sample_surface.view = view;
	sample_surface.normal = normal;
	sample_surface.dotNV = dotNV;

	for (uint i = 0; i < samples; ++i)
	{
		vec2 Xi = Hammersley(i, samples);

		sample_surface.halfVector = ImportanceSamplingGGX(Xi, sample_surface.normal, roughness);
		sample_surface.light = -reflect(sample_surface.view, sample_surface.halfVector);

		sample_surface.dotNH = max(0.0f, dot(sample_surface.normal, sample_surface.halfVector));
		sample_surface.dotNL = max(0.0f, dot(sample_surface.normal, sample_surface.light));
		sample_surface.dotHV = max(0.0f, dot(sample_surface.halfVector, sample_surface.view));

		float G = G_SmithGGX(sample_surface, roughness);
		float F = F_Shlick(sample_surface);

		float fr = (G * sample_surface.dotHV) / (sample_surface.dotNH * sample_surface.dotNV + EPSILON);

		A += (1 - F) * fr;
		B += F * fr;
	}

	return vec2(A, B) / float(samples);
}

void main()
{
	vec2 bakedBRDF = IntegrateBRDF(fragTexCoord.x, fragTexCoord.y);
	outColor = vec4(bakedBRDF.x, bakedBRDF.y, 0.0f, 0.0f);
}
