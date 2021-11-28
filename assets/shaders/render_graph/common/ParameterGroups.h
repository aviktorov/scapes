#ifndef RENDER_GRAPH_PARAMETER_GROUPS_H__
#define RENDER_GRAPH_PARAMETER_GROUPS_H__

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

#ifdef RENDER_GRAPH_SSAO_KERNEL_SET
#define MAX_SSAO_SAMPLES 64

layout(set = RENDER_GRAPH_SSAO_KERNEL_SET, binding = 0) uniform SSAOKernel {
	int num_samples;
	float radius;
	float intensity;
	vec4 samples[MAX_SSAO_SAMPLES];
} ssao_kernel;
#endif // RENDER_GRAPH_SSAO_KERNEL_SET

#endif // RENDER_GRAPH_PARAMETER_GROUPS_H__
