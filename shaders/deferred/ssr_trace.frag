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
	int intersection_index = num_coarse_steps;
	float depth_delta = 0.0f;

	vec2 uv = vec2(0.0f, 0.0f);

	for (int i = 1; i <= num_coarse_steps; i++)
	{
		vec3 sample_positionVS = positionVS + directionVS * coarse_step_size * i;

		uv = getUV(sample_positionVS);

		vec3 gbuffer_sample_positionVS = getPositionVS(uv, ubo.invProj);

		float sample_depth = -sample_positionVS.z;
		float gbuffer_depth = -gbuffer_sample_positionVS.z;

		depth_delta = gbuffer_depth - sample_depth;
		if (depth_delta < 0.0f)
		{
			intersection_index = i;
			break;
		}
	}

	// Precise tracing
	vec3 start = positionVS + directionVS * coarse_step_size * (intersection_index - 1);
	vec3 end = positionVS + directionVS * coarse_step_size * intersection_index;

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

	float fade = (abs(depth_delta) > ssr_data.bypass_depth_threshold) ? 0.0f : 1.0f;
	return vec3(uv, fade);
}

/*
 */
void main()
{
	vec2 noise_uv = fragTexCoord * textureSize(gbufferBaseColor, 0) / textureSize(ssrNoiseTexture, 0).xy;
	vec4 blue_noise = texture(ssrNoiseTexture, noise_uv);

	vec2 uv = fragTexCoord;

	vec3 positionVS = getPositionVS(uv, ubo.invProj);
	vec3 normalVS = getNormalVS(uv);

	vec3 viewVS = -normalize(positionVS);
	vec2 shading = texture(gbufferShading, uv).rg;

	SurfaceMaterial material;
	material.albedo = texture(gbufferBaseColor, uv).rgb;
	material.roughness = shading.r;
	material.metalness = shading.g;
	material.ao = 1.0f;
	material.f0 = lerp(vec3(0.04f), material.albedo, material.metalness);

	// trace pass
	uint size = 4;
	ivec2 sequence_coords = ivec2(uv * textureSize(gbufferBaseColor, 0)) % ivec2(size, size);

	uint N = size * size;
	uint i = uint(sequence_coords.y) * size + uint(sequence_coords.x);
	vec2 Xi = Hammersley(i, N);
	// vec2 Xi = blue_noise.xy;

	vec3 microfacet_normalVS = ImportanceSamplingGGX(Xi, normalVS, material.roughness);
	vec3 reflectionVS = -reflect(viewVS, microfacet_normalVS);

	// float step = lerp(ssr_data.coarse_step_size * 0.5f, ssr_data.coarse_step_size * 4.0f, blue_noise.z);
	float step = ssr_data.coarse_step_size;

	vec3 result = traceRay(positionVS, reflectionVS, ssr_data.num_coarse_steps, step, ssr_data.num_precision_steps);

	float border_factor = pow(saturate(1.0f - length(result.xy - vec2(0.5f, 0.5f)) * 2.0f), 1.0f);

	float facing_threshold = ssr_data.precision_step_depth_threshold; // TODO: replace by another threshold
	float facing_dot = max(0.0f, dot(viewVS, reflectionVS));
	float facing_factor = 1.0f - saturate((facing_dot - facing_threshold) / (1.0f - facing_threshold));

	float thickness_factor = result.z;

	float fade = border_factor * facing_factor * thickness_factor;

	outSSRTrace = vec4(result.xy, fade, 0.0f);
}
