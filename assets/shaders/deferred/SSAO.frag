#version 450

#include <shaders/materials/pbr/BRDF.h>

#define CAMERA_SET 0
#include <shaders/common/Camera.h>

#define GBUFFER_SET 1
#include <shaders/deferred/GBuffer.h>

#define SSAO_KERNEL_SET 2
#include <shaders/deferred/SSAOKernel.h>

layout(location = 0) in vec2 inUV;

layout(location = 0) out float outSSAO;

float getOcclusion(vec3 originVS, mat3 TBN, float radius, float bias, int numSamples)
{
	float occlusion = 0.0f;
	float originDepth = originVS.z;

	for (int i = 0; i < numSamples; ++i)
	{
		vec3 offset = ssaoKernel.samples[i].xyz;
		vec3 samplePositionVS = originVS + TBN * offset * radius;

		vec4 ndc = camera.projection * vec4(samplePositionVS, 1.0f);
		ndc.xyz /= ndc.w;
		ndc.xy = ndc.xy * 0.5f + vec2(0.5f);

		vec3 gbufferPositionVS = getGBufferPositionVS(ndc.xy, camera.iprojection);

		float sampleDepth = samplePositionVS.z;
		float gbufferDepth = gbufferPositionVS.z - bias;

		float rangeCheck = smoothstep(1.0f, 0.0f, abs(originDepth - gbufferDepth) / radius);

		float occluded = (gbufferDepth > sampleDepth) ? 1.0f : 0.0f;
		occlusion += occluded * rangeCheck;
	}

	return occlusion;
}

/*
 */
void main()
{
	vec3 originVS = getGBufferPositionVS(inUV, camera.iprojection);

	vec2 noiseUV = inUV * textureSize(texGBufferBaseColor, 0) / textureSize(texSSAOKernelNoise, 0).xy;
	vec3 randomDirection = vec3(texture(texSSAOKernelNoise, noiseUV).xy, 0.0f);

	vec3 normalVS = getGBufferNormalVS(inUV);
	vec3 tangentVS = normalize(randomDirection - normalVS * dot(randomDirection, normalVS));
	vec3 binormalVS = cross(normalVS, tangentVS);

	mat3 TBN = mat3(tangentVS, binormalVS, normalVS);

	float occlusion = getOcclusion(originVS, TBN, ssaoKernel.radius, 0.01f, ssaoKernel.numSamples);

	outSSAO = 1.0f - occlusion / ssaoKernel.numSamples;
	outSSAO = pow(outSSAO, ssaoKernel.intensity);
}
