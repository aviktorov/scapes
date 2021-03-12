#version 450
#pragma shader_stage(fragment)

// TODO: add set define
#include <common/RenderState.inc>

#define GBUFFER_SET 1
#include <deferred/gbuffer.inc>

#define SSAO_KERNEL_SET 2
#include <deferred/ssao_kernel.inc>

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out float outSSAO;

float getOcclusion(vec3 originVS, mat3 TBN, float radius, float bias, int num_samples)
{
	float occlusion = 0.0f;
	float origin_depth = originVS.z;

	for (int i = 0; i < num_samples; ++i)
	{
		vec3 offset = ssao_kernel.samples[i].xyz;
		vec3 sample_positionVS = originVS + TBN * offset * radius;

		vec4 ndc = ubo.projection * vec4(sample_positionVS, 1.0f);
		ndc.xyz /= ndc.w;
		ndc.xy = ndc.xy * 0.5f + vec2(0.5f);

		vec3 gbuffer_sample_positionVS = getPositionVS(ndc.xy, ubo.iprojection);

		float sample_depth = sample_positionVS.z;
		float gbuffer_depth = gbuffer_sample_positionVS.z - bias;

		float range_check = smoothstep(1.0f, 0.0f, abs(origin_depth - gbuffer_depth) / radius);

		float occluded = (gbuffer_depth > sample_depth) ? 1.0f : 0.0f;
		occluded *= range_check;

		occlusion += occluded;
	}

	return occlusion;
}

/*
 */
void main()
{
	vec3 originVS = getPositionVS(fragTexCoord, ubo.iprojection);

	vec2 noise_uv = fragTexCoord * textureSize(gbufferBaseColor, 0) / textureSize(ssaoKernelNoiseTexture, 0).xy;
	vec3 random_direction = vec3(texture(ssaoKernelNoiseTexture, noise_uv).xy, 0.0f);

	vec3 normalVS = getNormalVS(fragTexCoord);
	vec3 tangentVS   = normalize(random_direction - normalVS * dot(random_direction, normalVS));
	vec3 binormalVS = cross(normalVS, tangentVS);

	mat3 TBN = mat3(tangentVS, binormalVS, normalVS);

	float occlusion = getOcclusion(originVS, TBN, ssao_kernel.radius, 3.0f, ssao_kernel.num_samples);

	outSSAO = 1.0f - occlusion / ssao_kernel.num_samples;
	outSSAO = pow(outSSAO, ssao_kernel.intensity);
}
