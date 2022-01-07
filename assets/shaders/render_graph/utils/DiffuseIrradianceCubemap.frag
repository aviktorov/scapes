#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(set = 0, binding = 1) uniform samplerCube tex_environment;

// Input
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_position_ws;

// Output
layout(location = 0) out vec4 out_color;

void main()
{
	vec3 normal = normalize(in_position_ws);

	// from tangent-space vector to world-space sample vector
	vec3 up = abs(normal.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
	vec3 right = normalize(cross(up, normal));
	vec3 forward = cross(normal, right);

	vec3 irradiance = vec3(0.0f);

	float angle_step = 0.010f;
	float num_samples = 0.0f;

	for (float phi = 0.0f; phi < 2.0f * PI; phi += angle_step)
	{
		float cos_phi = cos(phi);
		float sin_phi = sin(phi);

		for (float theta = 0.0f; theta < 0.5f * PI; theta += angle_step)
		{
			float cos_theta = cos(theta);
			float sin_theta = sin(theta);

			vec3 coords = vec3(sin_phi * cos_theta, cos_phi * cos_theta, sin_theta);
			vec3 direction = right * coords.x + forward * coords.y + normal * coords.z;

			irradiance += textureLod(tex_environment, direction, 0.0f).rgb * cos_theta * sin_theta;
			num_samples++;
		}
	}

	out_color = vec4(PI * irradiance * (1.0f / num_samples), 1.0f);
}
