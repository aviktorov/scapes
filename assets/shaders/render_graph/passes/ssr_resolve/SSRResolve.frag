#version 450

// Includes
#include <shaders/common/Common.h>
#include <shaders/common/GBuffer.h>
#include <shaders/render_graph/common/BRDF.h>

// Bindings
#define RENDER_GRAPH_APPLICATION_SET 0
#define RENDER_GRAPH_CAMERA_SET 1
#include <shaders/render_graph/common/ParameterGroups.h>

layout(set = 2, binding = 0) uniform sampler2D tex_gbuffer_basecolor;
layout(set = 2, binding = 1) uniform sampler2D tex_gbuffer_normal;
layout(set = 2, binding = 2) uniform sampler2D tex_gbuffer_shading;
layout(set = 2, binding = 3) uniform sampler2D tex_gbuffer_depth;
layout(set = 2, binding = 4) uniform sampler2D tex_ssr_trace;
layout(set = 2, binding = 5) uniform sampler2D tex_ssr_noise;
layout(set = 2, binding = 6) uniform sampler2D tex_composite_old;

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out vec4 out_ssr;
layout(location = 1) out vec2 out_ssr_velocity;

//
vec2 getMotionVector(vec2 uv, vec3 position_vs, vec4 trace_result)
{
	vec3 intersection_point_vs = getGBufferPositionVS(tex_gbuffer_depth, trace_result.xy, camera.iprojection);
	float ray_length = length(position_vs);
	float intersected_ray_length = length(position_vs - intersection_point_vs);

	vec3 reflected_point_vs = position_vs * (1.0f + intersected_ray_length / ray_length);
	vec4 reflected_point_ndc = camera.projection * vec4(reflected_point_vs, 1.0f);

	vec4 reflected_point_old_vs = camera.view_old * camera.iview * vec4(reflected_point_vs, 1.0f);
	vec4 reflected_point_old_ndc = camera.projection * reflected_point_old_vs;

	return (reflected_point_old_ndc.xy / reflected_point_old_ndc.w - reflected_point_ndc.xy / reflected_point_ndc.w) * 0.5f;
}

vec4 resolveRay(vec4 trace_result, vec2 motion_vector)
{
	// TODO: sample mip based on roughness value

	vec4 result;
	result.rgb = textureLod(tex_composite_old, trace_result.xy + motion_vector, 0.0f).rgb;
	result.a = trace_result.z;

	return result;
}

const int num_resolve_samples = 5;

const vec2 resolve_offets[5] =
{
	vec2(-2.0f, 0.0f),
	vec2(-1.0f, 0.0f),
	vec2( 0.0f, 0.0f),
	vec2( 1.0f, 0.0f),
	vec2( 2.0f, 0.0f)
};

const float resolve_weights[5] =
{
	0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f,
};

/*
 */
void main()
{
	GBuffer gbuffer = sampleGBuffer(tex_gbuffer_basecolor, tex_gbuffer_normal, tex_gbuffer_shading, tex_gbuffer_depth, in_uv);

	vec3 position_vs = getGBufferPositionVS(gbuffer.depth, in_uv, camera.iprojection);
	vec3 normal_vs = gbuffer.normal_vs;
	vec3 view_vs = -normalize(position_vs);

	Material material;
	material.albedo = gbuffer.basecolor;
	material.roughness = gbuffer.roughness;
	material.metalness = gbuffer.metalness;
	material.ao = 1.0f;
	material.f0 = mix(vec3(0.04f), material.albedo, material.metalness);

	float dot_NV = max(0.0f, dot(normal_vs, view_vs));
	vec3 F = F_Shlick(dot_NV, material.f0, material.roughness);

	vec2 isize = 1.0f / vec2(textureSize(tex_ssr_trace, 0));

	vec2 noise_uv = in_uv * textureSize(tex_ssr_trace, 0) / textureSize(tex_ssr_noise, 0).xy;

	// TODO: better TAA noise
	noise_uv.xy += application.time;

	vec4 blue_noise = texture(tex_ssr_noise, noise_uv);

	float theta = PI2 * interleavedGradientNoise(in_uv, blue_noise.xy);
	float sin_theta = sin(theta);
	float cos_theta = cos(theta);
	mat2 rotation = mat2(cos_theta, sin_theta, -sin_theta, cos_theta);

	out_ssr = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	out_ssr_velocity = vec2(0.0f, 0.0f);

	for (int i = 0; i < num_resolve_samples; ++i)
	{
		vec2 offset = rotation * resolve_offets[i];
		vec2 uv = in_uv + offset * isize;

		vec4 trace_result = texture(tex_ssr_trace, uv);
		vec2 motion_vector = getMotionVector(uv, position_vs, trace_result);

		out_ssr += resolveRay(trace_result, motion_vector) * resolve_weights[i];
		out_ssr_velocity += motion_vector;
	}

	out_ssr_velocity /= num_resolve_samples;
	out_ssr.rgb *= F;
}
