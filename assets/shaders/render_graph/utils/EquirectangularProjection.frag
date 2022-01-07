#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(set = 0, binding = 1) uniform sampler2D tex_hdri;

// Input
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_position_ws;

// Output
layout(location = 0) out vec4 out_color;

//
const vec2 iatan = vec2(iPI2, iPI);

vec2 sampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.x, v.y), -asin(v.z));
	uv *= iatan;
	uv += 0.5;
	return uv;
}

void main()
{
	vec3 direction = normalize(in_position_ws);
	vec2 uv = sampleSphericalMap(direction);

	vec4 color;
	color.rgb = textureLod(tex_hdri, uv, 0.0f).rgb;
	color.a = 1.0f;
 
	out_color = color;
}
