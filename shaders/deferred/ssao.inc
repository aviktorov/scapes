#ifndef SSAO_INC_
#define SSAO_INC_

#define MAX_SAMPLES 512

layout(set = SSAO_SET, binding = 0) uniform SSAO {
	int num_samples;
	float radius;
	float intensity;
	vec4 samples[MAX_SAMPLES];
} ssao;

#ifndef SSAO_INTERNAL
	layout(set = SSAO_SET, binding = 1) uniform sampler2D ssaoTexture;
#endif

#endif // SSAO_INC_
