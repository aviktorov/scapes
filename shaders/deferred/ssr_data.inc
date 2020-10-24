#ifndef SSR_DATA_INC_
#define SSR_DATA_INC_

layout(set = SSR_DATA_SET, binding = 0) uniform SSRData {
	float coarse_step_size;
	int num_coarse_steps;
	int num_precision_steps;
	float precision_step_depth_threshold;
	float bypass_depth_threshold;
} ssr_data;

layout(set = SSR_DATA_SET, binding = 1) uniform sampler2D ssrNoiseTexture;

#endif // SSR_DATA_INC_
