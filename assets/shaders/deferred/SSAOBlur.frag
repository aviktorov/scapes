#version 450
#pragma shader_stage(fragment)

#define SSAO_SET 1
#include <shaders/deferred/SSAO.h>

layout(location = 0) in vec2 inUV;

layout(location = 0) out float outSSAOBlur;

const int blur_size = 4;

/*
 */
void main()
{
	vec2 texel_size = 1.0f / vec2(textureSize(texSSAO, 0));
	vec2 half_size = vec2(float(-blur_size) * 0.5f + 0.5f);

	float result = 0.0;
	for (int x = 0; x < blur_size; ++x)
	{
		for (int y = 0; y < blur_size; ++y)
		{
			vec2 offset = (half_size + vec2(float(x), float(y))) * texel_size;
			result += texture(texSSAO, inUV + offset).r;
		}
	}

	outSSAOBlur = result / float(blur_size * blur_size);
}
