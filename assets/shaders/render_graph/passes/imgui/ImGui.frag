#version 450

// Bindings
layout(set = 0, binding = 0) uniform sampler2D tex_color;

// Input
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;

// Output
layout(location = 0) out vec4 out_color;

//
void main()
{
	out_color = in_color * texture(tex_color, in_uv);
}
