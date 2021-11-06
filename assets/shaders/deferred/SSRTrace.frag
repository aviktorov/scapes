#version 450

#include <shaders/common/Common.h>

#define APPLICATION_STATE_SET 0
#include <shaders/common/ApplicationState.h>

#define CAMERA_SET 1
#include <shaders/common/Camera.h>

#define GBUFFER_SET 2
#include <shaders/deferred/GBuffer.h>

#define SSR_DATA_SET 3
#include <shaders/deferred/SSRData.h>

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outSSRTrace;

vec3 traceRay(vec3 positionVS, vec3 directionVS, int numCoarseSteps, float coarseStepSize, int numPrecisionSteps)
{
	// TODO: better tracing

	// Coarse tracing
	float depthDelta = 0.0f;

	vec2 uv = vec2(0.0f, 0.0f);
	float intersection = 0.0f;

	vec3 samplePositionVS = positionVS;
	for (int i = 0; i < numCoarseSteps; i++)
	{
		samplePositionVS += directionVS * coarseStepSize;

		uv = getUV(samplePositionVS);
		if (uv.x < 0.0f || uv.x > 1.0f)
			break;
		if (uv.y < 0.0f || uv.y > 1.0f)
			break;

		vec3 gbufferPositionVS = getGBufferPositionVS(uv, camera.iprojection);

		float sampleDepth = -samplePositionVS.z;
		float gbufferDepth = -gbufferPositionVS.z;

		depthDelta = gbufferDepth - sampleDepth;

		if ((directionVS.z * coarseStepSize - depthDelta) > ssrData.bypassDepthThreshold)
			break;

		if (depthDelta < 0.0f)
		{
			intersection = 1.0f;
			break;
		}
	}

	// Precise tracing
	if (intersection > 0.0f)
	{
		vec3 start = samplePositionVS - directionVS * coarseStepSize;
		vec3 end = samplePositionVS;

		for (int i = 0; i < numPrecisionSteps; i++)
		{
			vec3 mid = (start + end) * 0.5f;
			uv = getUV(mid);

			vec3 gbufferPositionVS = getGBufferPositionVS(uv, camera.iprojection);

			float sampleDepth = -mid.z;
			float gbufferDepth = -gbufferPositionVS.z;

			depthDelta = gbufferDepth - sampleDepth;

			if (depthDelta < 0.0f)
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
	GBuffer gbuffer = sampleGBuffer(inUV);

	vec3 positionVS = getGBufferPositionVS(inUV, gbuffer.depth, camera.iprojection);
	vec3 viewVS = -normalize(positionVS);
	float roughness = gbuffer.roughness;

	// calculate random reflection vector
	// TODO: better TAA noise
	vec2 noiseUV = inUV * textureSize(texGBufferShading, 0) / textureSize(texSSRNoise, 0).xy;
	noiseUV.xy += applicationState.time;
	vec4 blueNoise = texture(texSSRNoise, noiseUV);

	vec2 Xi = vec2(whiteNoise2D(inUV + blueNoise.xy), whiteNoise2D(inUV + blueNoise.zw));
	Xi.y = mix(0.0f, brdfBias, Xi.y);

	vec3 microfacetNormalVS = importanceSamplingGGX(Xi, gbuffer.normalVS, roughness);
	vec3 reflectionVS = -reflect(viewVS, microfacetNormalVS);

	// trace ray
	float traceStep = ssrData.coarseStepSize;
	traceStep *= mix(minStepMultiplier, maxStepMultiplier, blueNoise.x);

	vec3 result = traceRay(positionVS, reflectionVS, ssrData.numCoarseSteps, traceStep, ssrData.numPrecisionSteps);

	// calc fade factors
	float rayhitNDC = saturate(2.0f * length(result.xy - vec2(0.5f, 0.5f)));
	float borderFactor = 1.0f - pow(rayhitNDC, 8.0f);

	float facingDot = max(0.0f, dot(viewVS, reflectionVS));
	float facingFactor = 1.0f - saturate(facingDot / (ssrData.facingThreshold + EPSILON));
	float rayhitFactor = result.z;

	float fade = borderFactor * facingFactor * rayhitFactor;

	outSSRTrace = vec4(result.xy, fade, 0.0f);
}
