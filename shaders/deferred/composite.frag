#version 450
#pragma shader_stage(fragment)

#include <common/brdf.inc>

#define LBUFFER_SET 0
#include <deferred/lbuffer.inc>

#define SSAO_SET 1
#include <deferred/ssao.inc>

#define SSR_SET 2
#include <deferred/ssr.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outHdrColor;

void main()
{
	float ao = texture(ssaoTexture, fragTexCoord).r;
	vec4 ssr = texture(ssrTexture, fragTexCoord);
	vec4 indirect_specular = texture(lbufferSpecular, fragTexCoord);
	vec4 indirect_diffuse = texture(lbufferDiffuse, fragTexCoord);

	outHdrColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	outHdrColor.rgb += indirect_diffuse.rgb;
	outHdrColor.rgb += lerp(indirect_specular.rgb, ssr.rgb, ssr.a);
	outHdrColor.rgb *= ao;
}
