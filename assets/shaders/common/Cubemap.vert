#version 450

// Uniforms
layout(set = 0, binding = 0) uniform Faces {
	mat4 views[6];
} faces;

// Input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec3 inBinormal;
layout(location = 4) in vec3 inNormal;
layout(location = 5) in vec4 inColor;

// Output
layout(location = 0) out vec4 outFacePositionsVS[6];

void main() {
	gl_Position = vec4(inPosition.xyz, 1.0f);

	for (int i = 0; i < 6; i++)
		outFacePositionsVS[i] = faces.views[i] * vec4(inPosition.xyz, 1.0f);
}
