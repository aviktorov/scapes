#version 450
#pragma shader_stage(fragment)

#include <common/Common.h>

#define LBUFFER_SET 0
#include <deferred/LBuffer.h>

#define SSAO_SET 1
#include <deferred/SSAO.h>

#define SSR_SET 2
#include <deferred/SSR.h>

// input
layout(location = 0) in vec2 inUV;

// output
layout(location = 0) out vec4 outCompositeHDRColor;

void main()
{
	float ao = texture(texSSAO, inUV).r;
	vec4 ssr = texture(texSSR, inUV);
	vec4 indirectSpecular = texture(texLBufferSpecular, inUV);
	vec4 indirectDiffuse = texture(texLBufferDiffuse, inUV);

	outCompositeHDRColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	outCompositeHDRColor.rgb += indirectDiffuse.rgb;
	outCompositeHDRColor.rgb += mix(indirectSpecular.rgb, ssr.rgb, ssr.a);
	outCompositeHDRColor.rgb *= ao;
}
