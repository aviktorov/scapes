#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(push_constant) uniform Push
{
	float roughness;
} push;

layout(set = 0, binding = 1) uniform samplerCube tex_environment;

// 
layout(location = 0) in vec4 inFacePositionVS[6];

layout(location = 0) out vec4 outPrefilteredSpecular0;
layout(location = 1) out vec4 outPrefilteredSpecular1;
layout(location = 2) out vec4 outPrefilteredSpecular2;
layout(location = 3) out vec4 outPrefilteredSpecular3;
layout(location = 4) out vec4 outPrefilteredSpecular4;
layout(location = 5) out vec4 outPrefilteredSpecular5;

//
vec3 prefilterSpecularEnvironment(vec3 view, vec3 normal, float roughness)
{
	vec3 result = vec3(0.0);

	float weight = 0.0f;

	const uint samples = 2048;
	for (uint i = 0; i < samples; ++i)
	{
		vec2 Xi = hammersley(i, samples);

		vec3 half_vector = importanceSamplingGGX(Xi, normal, roughness);
		vec3 light = -reflect(view, half_vector);

		float dot_NL = max(0.0f, dot(normal, light));
		vec3 Li = texture(tex_environment, light).rgb;

		result += Li * dot_NL;
		weight += dot_NL;
	}

	return result / weight;
}

vec4 fillFace(int index)
{
	vec3 direction = normalize(inFacePositionVS[index].xyz);

	// TODO: figure out why cubemap sampling direction is inversed and rotated 90 degress around Z axis
	direction = -direction;
	direction.xy = vec2(direction.y, -direction.x);

	vec4 color;
	color.rgb = prefilterSpecularEnvironment(direction, direction, push.roughness);
	color.a = 1.0f;

	return color;
}

void main()
{
	outPrefilteredSpecular0 = fillFace(0);
	outPrefilteredSpecular1 = fillFace(1);
	outPrefilteredSpecular2 = fillFace(2);
	outPrefilteredSpecular3 = fillFace(3);
	outPrefilteredSpecular4 = fillFace(4);
	outPrefilteredSpecular5 = fillFace(5);
}
