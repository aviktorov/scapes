#version 450
#pragma shader_stage(vertex)

#include "RenderState.inc"

layout(push_constant) uniform RenderNodeState {
	mat4 transform;
} node;

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
layout(location = 2) out vec3 fragTangentVS;
layout(location = 3) out vec3 fragBinormalVS;
layout(location = 4) out vec3 fragNormalVS;
layout(location = 5) out vec3 fragPositionVS;

void main() {
	mat4 modelview = ubo.view * ubo.world * node.transform;
	gl_Position = ubo.proj * modelview * vec4(inPosition, 1.0f);

	fragColor = inColor;
	fragTexCoord = inTexCoord;

	fragTangentVS = vec3(modelview * vec4(inTangent, 0.0f));
	fragBinormalVS = vec3(modelview * vec4(inBinormal, 0.0f));
	fragNormalVS = vec3(modelview * vec4(inNormal, 0.0f));
	fragPositionVS = vec3(modelview * vec4(inPosition, 1.0f));
}
