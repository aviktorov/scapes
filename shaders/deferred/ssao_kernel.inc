#ifndef SSAO_KERNEL_INC_
#define SSAO_KERNEL_INC_

#define MAX_SAMPLES 256

layout(set = SSAO_KERNEL_SET, binding = 0) uniform SSAOKernel {
	int num_samples;
	float radius;
	float intensity;
	vec4 samples[MAX_SAMPLES];
} ssao_kernel;

layout(set = SSAO_KERNEL_SET, binding = 1) uniform sampler2D ssaoKernelNoiseTexture;

#endif // SSAO_KERNEL_INC_
