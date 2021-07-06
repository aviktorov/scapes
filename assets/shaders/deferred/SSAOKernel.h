#ifndef SSAO_KERNEL_INC_
#define SSAO_KERNEL_INC_

#define MAX_SAMPLES 256

layout(set = SSAO_KERNEL_SET, binding = 0) uniform SSAOKernel {
	int numSamples;
	float radius;
	float intensity;
	vec4 samples[MAX_SAMPLES];
} ssaoKernel;

layout(set = SSAO_KERNEL_SET, binding = 1) uniform sampler2D texSSAOKernelNoise;

#endif // SSAO_KERNEL_INC_
