#version 450
#pragma shader_stage(fragment)

layout(set = 0, binding = 0) uniform sampler2D ssrTexture;
layout(set = 1, binding = 0) uniform sampler2D ssrOldTexture;

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outSSR;

float calc_alpha(vec2 uv)
{
	return 0.1f;
}

/*
 */
void main()
{
	vec4 color = texture(ssrTexture, fragTexCoord);
	vec4 old_color = texture(ssrOldTexture, fragTexCoord);

	float alpha = calc_alpha(fragTexCoord);
	outSSR = lerp(old_color, color, alpha);
}
