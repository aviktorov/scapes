#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(set = 0, binding = 1) uniform sampler2D tex_hdri;

// Input
layout(location = 0) in vec4 in_face_position_vs[6];

// Output
layout(location = 0) out vec4 out_hdr_color0;
layout(location = 1) out vec4 out_hdr_color1;
layout(location = 2) out vec4 out_hdr_color2;
layout(location = 3) out vec4 out_hdr_color3;
layout(location = 4) out vec4 out_hdr_color4;
layout(location = 5) out vec4 out_hdr_color5;

//
const vec2 iatan = vec2(0.1591f, 0.3183f);

vec2 sampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.y, v.x), asin(v.z));
	uv *= iatan;
	uv += 0.5;
	return uv;
}

vec4 fillFace(int index)
{
	vec3 direction = normalize(in_face_position_vs[index].xyz);
	vec2 uv = sampleSphericalMap(direction);

	vec4 color;
	color.rgb = texture(tex_hdri, uv).rgb;
	color.a = 1.0f;
 
	return color;
}

void main()
{
	out_hdr_color0 = fillFace(0);
	out_hdr_color1 = fillFace(1);
	out_hdr_color2 = fillFace(2);
	out_hdr_color3 = fillFace(3);
	out_hdr_color4 = fillFace(4);
	out_hdr_color5 = fillFace(5);
}
