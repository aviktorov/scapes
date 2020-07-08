#version 450
#pragma shader_stage(fragment)

#include <common/brdf.inc>

layout(push_constant) uniform UserData {
	float roughness;
} push;

layout(binding = 0) uniform UniformBufferObject {
	mat4 faces[6];
} ubo;

layout(binding = 1) uniform samplerCube environmentSampler;

layout(location = 0) in vec4 fragFacePositions[6];

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;
layout(location = 2) out vec4 outColor2;
layout(location = 3) out vec4 outColor3;
layout(location = 4) out vec4 outColor4;
layout(location = 5) out vec4 outColor5;

vec3 PrefilterSpecularEnvMap(vec3 view, vec3 normal, float roughness)
{
	vec3 result = vec3(0.0);

	float weight = 0.0f;

	const uint samples = 2048;
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

vec4 fillFace(int index)
{
	vec3 dir = normalize(fragFacePositions[index].xyz);
	vec4 color;

	// TODO: figure out why cubemap sampling direction is inversed and rotated 90 degress around Z axis
	dir = -dir;
	dir.xy = vec2(dir.y, -dir.x);

	color.rgb = PrefilterSpecularEnvMap(dir, dir, push.roughness);
	color.a = 1.0f;

	return color;
}

void main()
{
	outColor0 = fillFace(0);
	outColor1 = fillFace(1);
	outColor2 = fillFace(2);
	outColor3 = fillFace(3);
	outColor4 = fillFace(4);
	outColor5 = fillFace(5);
}
