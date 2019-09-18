#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
	mat4 faces[6];
} ubo;

// Input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inTangent;
layout(location = 2) in vec3 inBinormal;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inColor;
layout(location = 5) in vec2 inTexCoord;

// Output
layout(location = 0) out vec4 fragFacePositions[6];

void main() {
	gl_Position = vec4(inPosition.xyz, 1.0f);

	for (int i = 0; i < 6; i++)
		fragFacePositions[i] = ubo.faces[i] * vec4(inPosition.xyz, 1.0f);
}
