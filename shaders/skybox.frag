#version 450
#pragma shader_stage(fragment)

#include <common/RenderState.inc>
#include "SceneTextures.inc"

#define SKYLIGHT_SET 2
#include <lights/skylight.inc>

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPositionOS;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 color = texture(skyLightEnvironmentCube, normalize(fragPositionOS)).rgb;

	// TODO: move to separate pass
	// Tonemapping + gamma correction
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	outColor = vec4(color, 1.0f);
}
