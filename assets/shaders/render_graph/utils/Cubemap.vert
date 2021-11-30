#version 450

// Bindings
layout(set = 0, binding = 0) uniform Faces {
	mat4 views[6];
} faces;

// Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_tangent;
layout(location = 3) in vec3 in_binormal;
layout(location = 4) in vec3 in_normal;
layout(location = 5) in vec4 in_color;

// Output
layout(location = 0) out vec4 out_face_position_vs[6];

void main() {
	gl_Position = vec4(in_position.xyz, 1.0f);

	for (int i = 0; i < 6; i++)
		out_face_position_vs[i] = faces.views[i] * vec4(in_position.xyz, 1.0f);
}
