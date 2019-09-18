#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 world;
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
} ubo;

layout(binding = 6) uniform samplerCube environmentSampler;
layout(binding = 7) uniform samplerCube diffuseIrradianceSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPositionOS;

layout(location = 0) out vec4 outColor;

const float PI = 3.141592653589798979f;
const float iPI = 0.31830988618379f;

void main() {
	vec3 color = texture(environmentSampler, normalize(fragPositionOS)).rgb;

	// TODO: move to separate pass
	// Tonemapping + gamma correction
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	outColor = vec4(color, 1.0f);
}
