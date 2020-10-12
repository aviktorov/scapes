#ifndef SSR_DATA_INC_
#define SSR_DATA_INC_

layout(set = SSR_DATA_SET, binding = 0) uniform SSRData {
	float step;
	int num_steps;
	int num_precision_steps;
	float precision_step_depth_threshold;
	float bypass_depth_threshold;
} ssr_data;

#endif // SSR_DATA_INC_
