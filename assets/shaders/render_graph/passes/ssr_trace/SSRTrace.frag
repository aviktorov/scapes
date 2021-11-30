#version 450

// Includes
#include <shaders/common/Common.h>
#include <shaders/common/GBuffer.h>

// Bindings
#define RENDER_GRAPH_APPLICATION_SET 0
#define RENDER_GRAPH_CAMERA_SET 1
#define RENDER_GRAPH_SSR_SET 2
#include <shaders/render_graph/common/ParameterGroups.h>

layout(set = 3, binding = 0) uniform sampler2D tex_gbuffer_normal;
layout(set = 3, binding = 1) uniform sampler2D tex_gbuffer_shading;
layout(set = 3, binding = 2) uniform sampler2D tex_gbuffer_depth;
layout(set = 3, binding = 3) uniform sampler2D tex_ssr_noise;

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out vec4 out_ssr_trace;

//
vec3 traceRay(vec3 position_vs, vec3 direction_vs, float trace_step)
{
	// TODO: better tracing

	// Coarse tracing
	float depth_delta = 0.0f;

	vec2 uv = vec2(0.0f, 0.0f);
	float intersection = 0.0f;

	vec3 sample_position_vs = position_vs;
	for (int i = 0; i < ssr.num_coarse_steps; i++)
	{
		sample_position_vs += direction_vs * trace_step;

		uv = getUV(sample_position_vs, camera.projection);
		if (uv.x < 0.0f || uv.x > 1.0f)
			break;
		if (uv.y < 0.0f || uv.y > 1.0f)
			break;

		vec3 gbuffer_position_vs = getGBufferPositionVS(tex_gbuffer_depth, uv, camera.iprojection);

		float sample_depth = -sample_position_vs.z;
		float gbuffer_depth = -gbuffer_position_vs.z;

		depth_delta = gbuffer_depth - sample_depth;

		if ((direction_vs.z * trace_step - depth_delta) > ssr.depth_bypass_threshold)
			break;

		if (depth_delta < 0.0f)
		{
			intersection = 1.0f;
			break;
		}
	}

	// Precise tracing
	if (intersection > 0.0f)
	{
		vec3 start = sample_position_vs - direction_vs * trace_step;
		vec3 end = sample_position_vs;

		for (int i = 0; i < ssr.num_precision_steps; i++)
		{
			vec3 mid = (start + end) * 0.5f;
			uv = getUV(mid, camera.projection);

			vec3 gbuffer_position_vs = getGBufferPositionVS(tex_gbuffer_depth, uv, camera.iprojection);

			float sampleDepth = -mid.z;
			float gbufferDepth = -gbuffer_position_vs.z;

			depth_delta = gbufferDepth - sampleDepth;

			if (depth_delta < 0.0f)
				end = mid;
			else
				start = mid;
		}
	}

	return vec3(uv, intersection);
}

/*
 */
void main()
{
	GBuffer gbuffer = sampleGBuffer(tex_gbuffer_normal, tex_gbuffer_shading, tex_gbuffer_depth, in_uv);

	vec3 position_vs = getGBufferPositionVS(gbuffer.depth, in_uv, camera.iprojection);
	vec3 view_vs = -normalize(position_vs);

	// calculate random reflection vector
	// TODO: better TAA noise
	vec2 noise_uv = in_uv * textureSize(tex_gbuffer_shading, 0) / textureSize(tex_ssr_noise, 0).xy;
	noise_uv.xy += application.time;

	vec4 blue_noise = texture(tex_ssr_noise, noise_uv);

	vec2 Xi = vec2(whiteNoise(in_uv + blue_noise.xy), whiteNoise(in_uv + blue_noise.zw));
	Xi.y *= ssr.brdf_bias;

	vec3 microfacet_normal_vs = importanceSamplingGGX(Xi, gbuffer.normal_vs, gbuffer.roughness);
	vec3 reflection_vs = -reflect(view_vs, microfacet_normal_vs);

	// trace ray
	float trace_step = ssr.coarse_step;
	trace_step *= mix(ssr.min_step_multiplier, ssr.max_step_multiplier, blue_noise.x);

	vec3 result = traceRay(position_vs, reflection_vs, trace_step);

	// calc fade factors
	float rayhit_ndc = saturate(2.0f * length(result.xy - vec2(0.5f, 0.5f)));
	float factor_border = 1.0f - pow(rayhit_ndc, 8.0f);

	float dot_VR = max(0.0f, dot(view_vs, reflection_vs));
	float factor_facing = 1.0f - saturate(dot_VR / (ssr.facing_threshold + EPSILON));
	float factor_rayhit = result.z;

	float fade = factor_border * factor_facing * factor_rayhit;

	out_ssr_trace = vec4(result.xy, fade, 0.0f);
}
