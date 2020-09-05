#version 450
#pragma shader_stage(fragment)

#define LBUFFER_SET 0
#include <deferred/lbuffer.inc>

#define SSAO_SET 1
#include <deferred/ssao.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outHdrColor;

void main()
{
	float ao = texture(ssaoTexture, fragTexCoord).r;

	outHdrColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	outHdrColor += texture(lbufferDiffuse, fragTexCoord) * ao;
	outHdrColor += texture(lbufferSpecular, fragTexCoord);
}
