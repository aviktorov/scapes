#version 450

layout(set = 0, binding = 1) uniform samplerCube texEnvironment;

layout(location = 0) in vec4 inFacePositionVS[6];

layout(location = 0) out vec4 outIrradiance0;
layout(location = 1) out vec4 outIrradiance1;
layout(location = 2) out vec4 outIrradiance2;
layout(location = 3) out vec4 outIrradiance3;
layout(location = 4) out vec4 outIrradiance4;
layout(location = 5) out vec4 outIrradiance5;

const float PI = 3.141592653589798979f;
const float iPI = 0.31830988618379f;

vec4 fillFace(int index)
{
	vec3 normal = normalize(inFacePositionVS[index].xyz);

	// TODO: figure out why cubemap sampling direction is inversed and rotated 90 degress around Z axis
	normal = -normal;
	normal.xy = vec2(normal.y, -normal.x);

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
			vec3 direction = right * tangent.x + forward * tangent.y + normal * tangent.z;

			irradiance += texture(texEnvironment, direction).rgb * cos(theta) * sin(theta);
			numSamples++;
		}
	}

	return vec4(PI * irradiance * (1.0f / numSamples), 1.0f);
}

void main()
{
	outIrradiance0 = fillFace(0);
	outIrradiance1 = fillFace(1);
	outIrradiance2 = fillFace(2);
	outIrradiance3 = fillFace(3);
	outIrradiance4 = fillFace(4);
	outIrradiance5 = fillFace(5);
}
