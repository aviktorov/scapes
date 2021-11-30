#version 450

// Includes
#include <shaders/common/Common.h>

// Bindings
layout(set = 0, binding = 0) uniform sampler2D tex_lbuffer_diffuse;
layout(set = 0, binding = 1) uniform sampler2D tex_lbuffer_specular;
layout(set = 0, binding = 2) uniform sampler2D tex_ssao;
layout(set = 0, binding = 3) uniform sampler2D tex_ssr;

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out vec4 out_composite;

//
void main()
{
	float ao = texture(tex_ssao, in_uv).r;
	vec4 ssr = texture(tex_ssr, in_uv);

	vec4 indirect_specular = texture(tex_lbuffer_specular, in_uv);
	vec4 indirect_diffuse = texture(tex_lbuffer_diffuse, in_uv);

	out_composite = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	out_composite.rgb += indirect_diffuse.rgb;
	out_composite.rgb += mix(indirect_specular.rgb, ssr.rgb, ssr.a);
	out_composite.rgb *= ao;
}
