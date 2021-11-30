#version 450

// Bindings
layout(set = 0, binding = 0) uniform sampler2D tex_color;

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out vec4 out_color;

//
void main()
{
	vec4 color = texture(tex_color, in_uv);
	color.rgb = pow(color.rgb, vec3(1.0f / 2.2f));

	out_color = color;
}
