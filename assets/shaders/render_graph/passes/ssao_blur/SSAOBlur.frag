#version 450

// Bindings
layout(set = 0, binding = 0) uniform sampler2D tex_ssao;

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out float out_ssao_blur;

//
void main()
{
	const int blur_size = 4;

	vec2 texel_size = 1.0f / vec2(textureSize(tex_ssao, 0));
	vec2 half_size = vec2(float(-blur_size) * 0.5f + 0.5f);

	float result = 0.0;
	for (int x = 0; x < blur_size; ++x)
	{
		for (int y = 0; y < blur_size; ++y)
		{
			vec2 offset = (half_size + vec2(float(x), float(y))) * texel_size;
			result += texture(tex_ssao, in_uv + offset).r;
		}
	}

	out_ssao_blur = result / float(blur_size * blur_size);
}
