#version 450
#pragma shader_stage(fragment)

// TODO: add set define
#include <common/RenderState.inc>

#define GBUFFER_SET 1
#include <deferred/gbuffer.inc>

#define SSAO_INTERNAL
#define SSAO_SET 2
#include <deferred/ssao.inc>

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out float outSSAO;

float getOcclusion(vec3 originVS, mat3 TBN, float radius, float bias, int num_samples)
{
	float occlusion = 0.0f;
	float origin_depth = originVS.z;

	for (int i = 0; i < num_samples; ++i)
	{
		vec3 offset = ssao.samples[i].xyz;
		vec3 sample_positionVS = originVS + TBN * offset * radius;

		vec4 ndc = ubo.proj * vec4(sample_positionVS, 1.0f);
		ndc.xyz /= ndc.w;
		ndc.xy = ndc.xy * 0.5f + vec2(0.5f);

		vec3 gbuffer_sample_positionVS = getPositionVS(ndc.xy, ubo.invProj);

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
	vec3 originVS = getPositionVS(fragTexCoord, ubo.invProj);

	vec3 random_direction = normalize(vec3(mod(fragTexCoord * 100.0f, 1.0f), 0.0f));

	vec3 normalVS = getNormalVS(fragTexCoord);
	vec3 tangentVS   = normalize(random_direction - normalVS * dot(random_direction, normalVS));
	vec3 binormalVS = cross(normalVS, tangentVS);

	mat3 TBN = mat3(tangentVS, binormalVS, normalVS); 

	float occlusion = getOcclusion(originVS, TBN, ssao.radius, 3.0f, ssao.num_samples);

	outSSAO = 1.0f - occlusion / ssao.num_samples;
	outSSAO = pow(outSSAO, ssao.intensity);
}
