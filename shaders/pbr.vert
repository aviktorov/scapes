#version 450
#pragma shader_stage(vertex)

#include <common/RenderState.inc>

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
	gl_Position = ubo.projection * ubo.view * vec4(inPosition, 1.0f);

	fragColor = inColor;
	fragTexCoord = inTexCoord;

	// TODO: pass node world transform;
	mat4 world = mat4(1.0f);

	fragTangentWS = vec3(world * vec4(inTangent, 0.0f));
	fragBinormalWS = vec3(world * vec4(inBinormal, 0.0f));
	fragNormalWS = vec3(world * vec4(inNormal, 0.0f));
	fragPositionWS = vec3(world * vec4(inPosition, 1.0f));
}
