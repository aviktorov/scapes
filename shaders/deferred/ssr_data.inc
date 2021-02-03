#ifndef SSR_DATA_INC_
#define SSR_DATA_INC_

layout(set = SSR_DATA_SET, binding = 0) uniform SSRData {
	float coarse_step_size;
	int num_coarse_steps;
	int num_precision_steps;
	float facing_threshold;
	float bypass_depth_threshold;
} ssr_data;

layout(set = SSR_DATA_SET, binding = 1) uniform sampler2D ssrNoiseTexture;

// TODO: move to SSRData
const float brdf_bias = 0.7f;
const float min_step_multiplier = 0.25f;
const float max_step_multiplier = 4.0f;


#endif // SSR_DATA_INC_
