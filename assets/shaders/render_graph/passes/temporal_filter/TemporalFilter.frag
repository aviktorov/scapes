#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(set = 0, binding = 0) uniform sampler2D tex_color;
layout(set = 1, binding = 0) uniform sampler2D tex_velocity;
layout(set = 2, binding = 0) uniform sampler2D tex_color_old;

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out vec4 out_color_filtered;

//
float calcAlpha(vec4 color, vec4 color_old)
{
	return 1.0f / 30.0f;
}

vec2 getReprojectedUV(vec2 uv)
{
	vec2 motion_vector = texture(tex_velocity, uv).xy;
	return uv + motion_vector;
}

void getNeighborsMinMax(in vec4 center, in vec2 uv, out vec4 color_min, out vec4 color_max)
{
	color_min = center;
	color_max = center;

	#define SAMPLE_NEIGHBOR(OFFSET) \
	{ \
		vec4 neighbor = textureOffset(tex_color, uv, OFFSET); \
		color_min = min(color_min, neighbor); \
		color_max = max(color_max, neighbor); \
	} \

	SAMPLE_NEIGHBOR(ivec2(-1, 0));
	SAMPLE_NEIGHBOR(ivec2(1, 0));
	SAMPLE_NEIGHBOR(ivec2(0, -1));
	SAMPLE_NEIGHBOR(ivec2(0, 1));
	SAMPLE_NEIGHBOR(ivec2(-1, 1));
	SAMPLE_NEIGHBOR(ivec2(1, 1));
	SAMPLE_NEIGHBOR(ivec2(1, -1));
	SAMPLE_NEIGHBOR(ivec2(1, 1));
}

/*
 */
void main()
{
	vec2 uv_current = in_uv;
	vec2 uv_old = getReprojectedUV(uv_current);

	vec4 color = texture(tex_color, uv_current);

	if (uv_old.x < 0.0f || uv_old.x > 1.0f || uv_old.y < 0.0f || uv_old.y > 1.0f)
	{
		out_color_filtered = color;
	}
	else
	{
		vec4 color_old = texture(tex_color_old, uv_old);

		vec4 color_min;
		vec4 color_max;
		getNeighborsMinMax(color, uv_current, color_min, color_max);

		color_old = clamp(color_old, color_min, color_max);

		float alpha = calcAlpha(color, color_old);
		out_color_filtered = mix(color_old, color, alpha);
	}
}
