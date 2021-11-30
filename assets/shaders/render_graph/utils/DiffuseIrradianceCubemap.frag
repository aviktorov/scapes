#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(set = 0, binding = 1) uniform samplerCube tex_environment;

// Input
layout(location = 0) in vec4 in_face_position_vs[6];

// Output
layout(location = 0) out vec4 out_irradiance_face0;
layout(location = 1) out vec4 out_irradiance_face1;
layout(location = 2) out vec4 out_irradiance_face2;
layout(location = 3) out vec4 out_irradiance_face3;
layout(location = 4) out vec4 out_irradiance_face4;
layout(location = 5) out vec4 out_irradiance_face5;

//
vec4 fillFace(int index)
{
	vec3 normal = normalize(in_face_position_vs[index].xyz);

	// TODO: figure out why cubemap sampling direction is inversed and rotated 90 degress around Z axis
	normal = -normal;
	normal.xy = vec2(normal.y, -normal.x);

	vec3 forward = vec3(0.0f, 1.0f, 0.0f);
	vec3 right = normalize(cross(forward, normal));
	forward = normalize(cross(normal, right));

	vec3 irradiance = vec3(0.0f);

	float angle_step = 0.015f;
	float num_samples = 0.0f;

	for (float phi = 0.0f; phi < 2.0f * PI; phi += angle_step)
	{
		float cos_phi = cos(phi);
		float sin_phi = sin(phi);

		for (float theta = 0.0f; theta < 0.5f * PI; theta += angle_step)
		{
			float cos_theta = cos(theta);
			float sin_theta = sin(theta);

			vec3 tangent = vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
			vec3 direction = right * tangent.x + forward * tangent.y + normal * tangent.z;

			irradiance += texture(tex_environment, direction).rgb * cos_theta * sin_theta;
			num_samples++;
		}
	}

	return vec4(PI * irradiance * (1.0f / num_samples), 1.0f);
}

void main()
{
	out_irradiance_face0 = fillFace(0);
	out_irradiance_face1 = fillFace(1);
	out_irradiance_face2 = fillFace(2);
	out_irradiance_face3 = fillFace(3);
	out_irradiance_face4 = fillFace(4);
	out_irradiance_face5 = fillFace(5);
}
