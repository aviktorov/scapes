#version 450
#pragma shader_stage(fragment)

// TODO: add set define
#include <common/RenderState.inc>

#define GBUFFER_SET 1
#include <deferred/gbuffer.inc>

#define SSR_DATA_SET 2
#include <deferred/ssr_data.inc>

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outSSRTrace;

vec2 getUV(vec3 positionVS)
{
	vec4 ndc = ubo.proj * vec4(positionVS, 1.0f);
	ndc.xyz /= ndc.w;
	return ndc.xy * 0.5f + vec2(0.5f);
}

vec3 traceRay(vec3 positionVS, vec3 directionVS, int num_coarse_steps, float coarse_step_size, int num_precision_steps)
{
	// TODO: better tracing

	// Coarse tracing
	float depth_delta = 0.0f;

	vec2 uv = vec2(0.0f, 0.0f);
	float intersection = 0.0f;

	vec3 sample_positionVS = positionVS;
	for (int i = 0; i < num_coarse_steps; i++)
	{
		sample_positionVS += directionVS * coarse_step_size;

		uv = getUV(sample_positionVS);
		if (uv.x < 0.0f || uv.x > 1.0f)
			break;
		if (uv.y < 0.0f || uv.y > 1.0f)
			break;

		vec3 gbuffer_sample_positionVS = getPositionVS(uv, ubo.invProj);

		float sample_depth = -sample_positionVS.z;
		float gbuffer_depth = -gbuffer_sample_positionVS.z;

		depth_delta = gbuffer_depth - sample_depth;

		if ((directionVS.z * coarse_step_size - depth_delta) > ssr_data.bypass_depth_threshold)
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
		vec3 start = sample_positionVS - directionVS * coarse_step_size;
		vec3 end = sample_positionVS;

		for (int i = 0; i < num_precision_steps; i++)
		{
			vec3 mid = (start + end) * 0.5f;
			uv = getUV(mid);

			vec3 gbuffer_sample_positionVS = getPositionVS(uv, ubo.invProj);

			float sample_depth = -mid.z;
			float gbuffer_depth = -gbuffer_sample_positionVS.z;

			depth_delta = gbuffer_depth - sample_depth;

			if (depth_delta < 0.0f)
				end = mid;
			else
				start = mid;
		}
	}

	return vec3(uv, intersection);
}

float whiteNoise2D(in vec2 pos)
{
	return fract(sin(dot(pos.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

/*
 */
void main()
{
	vec2 uv = fragTexCoord;
	vec2 noise_uv = uv * textureSize(gbufferShading, 0) / textureSize(ssrNoiseTexture, 0).xy;

	vec4 blue_noise = texture(ssrNoiseTexture, noise_uv);

	vec3 positionVS = getPositionVS(uv, ubo.invProj);
	vec3 normalVS = getNormalVS(uv);

	vec3 viewVS = -normalize(positionVS);

	vec2 shading = texture(gbufferShading, uv).rg;
	float roughness = shading.r;

	// trace pass
	vec2 Xi = vec2(whiteNoise2D(fragTexCoord + blue_noise.xy), whiteNoise2D(fragTexCoord + blue_noise.zw));
	Xi.y = lerp(0.0f, brdf_bias, Xi.y);

	vec3 microfacet_normalVS = ImportanceSamplingGGX(Xi, normalVS, roughness);
	vec3 reflectionVS = -reflect(viewVS, microfacet_normalVS);

	float step = ssr_data.coarse_step_size;
	step *= lerp(min_step_multiplier, max_step_multiplier, blue_noise.x);

	vec3 result = traceRay(positionVS, reflectionVS, ssr_data.num_coarse_steps, step, ssr_data.num_precision_steps);

	float border_factor = pow(saturate(1.0f - length(result.xy - vec2(0.5f, 0.5f)) * 2.0f), 1.0f);

	float facing_dot = max(0.0f, dot(viewVS, reflectionVS));
	float facing_factor = 1.0f - saturate(facing_dot / (ssr_data.facing_threshold + EPSILON));

	float rayhit_factor = result.z;

	float fade = border_factor * facing_factor * rayhit_factor;

	outSSRTrace = vec4(result.xy, fade, 0.0f);
}
