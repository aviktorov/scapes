#ifndef SSR_DATA_INC_
#define SSR_DATA_INC_

layout(set = SSR_DATA_SET, binding = 0) uniform SSRData {
	float coarseStepSize;
	int numCoarseSteps;
	int numPrecisionSteps;
	float facingThreshold;
	float bypassDepthThreshold;
} ssrData;

layout(set = SSR_DATA_SET, binding = 1) uniform sampler2D texSSRNoise;

// TODO: move to SSRData
const float brdfBias = 0.7f;
const float minStepMultiplier = 0.25f;
const float maxStepMultiplier = 4.0f;

#endif // SSR_DATA_INC_
