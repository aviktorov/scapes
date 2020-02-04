#version 450
#pragma shader_stage(fragment)

#include "brdf.inc"

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec2 IntegrateBRDF(float roughness, float dotNV)
{
	vec3 view;
	view.x = sqrt(1.0f - dotNV * dotNV);
	view.y = 0.0f;
	view.z = dotNV;

	vec3 normal = vec3(0.0f, 0.0f, 1.0f);

	float A = 0.0f;
	float B = 0.0f;

	const uint samples = 1024;

	Surface sample_surface;
	sample_surface.view = view;
	sample_surface.normal = normal;
	sample_surface.dotNV = max(0.0f, dot(sample_surface.normal, sample_surface.view));

	for (uint i = 0; i < samples; ++i)
	{
		vec2 Xi = Hammersley(i, samples);

		sample_surface.halfVector = ImportanceSamplingGGX(Xi, sample_surface.normal, roughness);
		sample_surface.light = -reflect(sample_surface.view, sample_surface.halfVector);

		sample_surface.dotNH = max(0.0f, dot(sample_surface.normal, sample_surface.halfVector));
		sample_surface.dotNL = max(0.0f, dot(sample_surface.normal, sample_surface.light));
		sample_surface.dotHV = max(0.0f, dot(sample_surface.halfVector, sample_surface.view));

		float G_normalized = G_SmithGGX(sample_surface, roughness);
		float F = pow(1 - sample_surface.dotHV, 5.0f);

		float pdf = sample_surface.dotNH / (4.0f * sample_surface.dotHV);

		float fr = G_normalized * sample_surface.dotNL / pdf;

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
