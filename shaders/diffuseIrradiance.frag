#version 450
#pragma shader_stage(fragment)

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

const float PI = 3.141592653589798979f;
const float iPI = 0.31830988618379f;

vec4 fillFace(int index)
{
	vec3 normal = normalize(fragFacePositions[index].xyz);
	normal.z *= -1.0f;

	vec3 forward = vec3(0.0f, 1.0f, 0.0f);
	vec3 right = normalize(cross(forward, normal));
	forward = normalize(cross(normal, right));

	vec3 irradiance = vec3(0.0f);

	float angleStep = 0.015f;
	float numSamples = 0.0f;

	for (float phi = 0.0f; phi < 2.0f * PI; phi += angleStep)
	{
		for (float theta = 0.0f; theta < 0.5f * PI; theta += angleStep)
		{
			vec3 tangent = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 sampleVec = right * tangent.x + forward * tangent.y + normal * tangent.z;

			irradiance += texture(environmentSampler, sampleVec).rgb * cos(theta) * sin(theta);
			numSamples++;
		}
	}

	return vec4(PI * irradiance * (1.0f / numSamples), 1.0f);
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
