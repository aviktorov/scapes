#version 450

// Bindings
layout(set = 0, binding = 0) uniform Cubemap {
	mat4 projection;
	mat4 face_views[6];
} cubemap;

// Input
layout(triangles) in;
layout(location = 0) in vec2 in_uv[];

// Output
layout(triangle_strip, max_vertices = 3 * 6) out;
layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_position_ws;

void main()
{
	for (int i = 0; i < 6; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			gl_Position = cubemap.projection * cubemap.face_views[i] * gl_in[j].gl_Position;
			gl_Layer = i;

			out_position_ws = gl_in[j].gl_Position.xyz;
			out_uv = in_uv[j];

			EmitVertex();
		}
		EndPrimitive();
	}
}
