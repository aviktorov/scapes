#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
	mat4 world;
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
} ubo;

// Input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inTangent;
layout(location = 2) in vec3 inBinormal;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inColor;
layout(location = 5) in vec2 inTexCoord;

// Output
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragTangentWS;
layout(location = 3) out vec3 fragBinormalWS;
layout(location = 4) out vec3 fragNormalWS;
layout(location = 5) out vec3 fragPositionWS;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.world * vec4(inPosition, 1.0f);

	fragColor = inColor;
	fragTexCoord = inTexCoord;

	fragTangentWS = vec3(ubo.world * vec4(inTangent, 0.0f));
	fragBinormalWS = vec3(ubo.world * vec4(inBinormal, 0.0f));
	fragNormalWS = vec3(ubo.world * vec4(inNormal, 0.0f));
	fragPositionWS = vec3(ubo.world * vec4(inPosition, 1.0f));
}
