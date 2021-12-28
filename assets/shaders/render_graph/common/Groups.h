#ifndef RENDER_GRAPH_GROUPS_H__
#define RENDER_GRAPH_GROUPS_H__

#ifdef RENDER_GRAPH_APPLICATION_SET
layout(set = RENDER_GRAPH_APPLICATION_SET, binding = 0) uniform Application
{
	float override_basecolor;
	float override_shading;
	float user_metalness;
	float user_roughness;
	float time;
} application;
#endif // RENDER_GRAPH_APPLICATION_SET

#ifdef RENDER_GRAPH_CAMERA_SET
layout(set = RENDER_GRAPH_CAMERA_SET, binding = 0) uniform Camera
{
	mat4 view;
	mat4 iview;
	mat4 projection;
	mat4 iprojection;
	mat4 view_old;
	vec4 parameters; // (zNear, zFar, 1.0f / zNear, 1.0f / zFar)
	vec3 position_ws;
} camera;
#endif // RENDER_GRAPH_CAMERA_SET

#ifdef RENDER_GRAPH_SSAO_SET
#define MAX_SSAO_SAMPLES 32

layout(set = RENDER_GRAPH_SSAO_SET, binding = 0) uniform SSAO {
	int num_samples;
	float radius;
	float intensity;
	vec4 samples[MAX_SSAO_SAMPLES];
} ssao;

layout(set = RENDER_GRAPH_SSAO_SET, binding = 1) uniform sampler2D tex_ssao_noise;
#endif // RENDER_GRAPH_SSAO_SET

#ifdef RENDER_GRAPH_SSR_SET
#define MAX_SSR_SAMPLES 64

layout(set = RENDER_GRAPH_SSR_SET, binding = 0) uniform SSR {
	float coarse_step;
	int num_coarse_steps;
	int num_precision_steps;
	float facing_threshold;
	float depth_bypass_threshold;
	float brdf_bias;
	float min_step_multiplier;
	float max_step_multiplier;
} ssr;

layout(set = RENDER_GRAPH_SSR_SET, binding = 1) uniform sampler2D tex_ssr_noise;
#endif // RENDER_GRAPH_SSR_SET

#endif // RENDER_GRAPH_GROUPS_H__
