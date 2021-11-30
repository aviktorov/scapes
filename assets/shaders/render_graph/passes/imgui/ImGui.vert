#version 450

// Bindings
layout(push_constant) uniform Push
{
	mat4 projection;
} push;

// Input
layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

// Output
layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

//
void main()
{
	out_color = in_color;
	out_uv = in_uv;
	gl_Position = push.projection * vec4(in_pos, 0.0f, 1.0f);
}
