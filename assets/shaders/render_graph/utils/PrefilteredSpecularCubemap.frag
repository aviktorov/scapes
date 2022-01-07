#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(push_constant) uniform Push
{
	float roughness;
} push;

layout(set = 0, binding = 1) uniform samplerCube tex_environment;

// Input
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_position_ws;

// Output
layout(location = 0) out vec4 out_color;

//
vec3 prefilterSpecularEnvironment(vec3 view, vec3 normal, float roughness)
{
	vec3 result = vec3(0.0);

	float weight = 0.0f;

	const uint samples = 2048;
	for (uint i = 0; i < samples; ++i)
	{
		vec2 Xi = hammersley(i, samples);

		vec3 microfacet_normal = importanceSamplingGGX(Xi, normal, roughness);
		vec3 light = -reflect(view, microfacet_normal);

		float dotNL = max(0.0f, dot(normal, light));
		vec3 Li = texture(tex_environment, light).rgb;

		result += Li * dotNL;
		weight += dotNL;
	}

	return result / weight;
}

void main()
{
	vec3 direction = normalize(in_position_ws);

	vec4 color;
	color.rgb = prefilterSpecularEnvironment(direction, direction, push.roughness);
	color.a = 1.0f;

	out_color = color;
}
