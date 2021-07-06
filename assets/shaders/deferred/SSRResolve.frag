#version 450
#pragma shader_stage(fragment)

#include <materials/pbr/BRDF.h>

#define APPLICATION_STATE_SET 0
#include <common/ApplicationState.h>

#define CAMERA_SET 1
#include <common/Camera.h>

#define GBUFFER_SET 2
#include <deferred/GBuffer.h>

#define SSR_SET 3
#include <deferred/SSR.h>

#define SSR_DATA_SET 4
#include <deferred/SSRData.h>

#define COMPOSITE_SET 5
#include <deferred/Composite.h>

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outSSR;
layout(location = 1) out vec2 outSSRVelocity;

vec2 getMotionVector(vec2 uv, vec4 traceResult)
{
	vec3 intersectionPointVS = getGBufferPositionVS(traceResult.xy, camera.iprojection);
	vec3 startPointVS = getGBufferPositionVS(uv, camera.iprojection);
	float startPointLinearDepth = length(startPointVS);
	float rayDepth = length(startPointVS - intersectionPointVS);

	vec3 reflectedPointVS = startPointVS * (1.0f + rayDepth / startPointLinearDepth);
	vec4 reflectedPointNDC = camera.projection * vec4(reflectedPointVS, 1.0f);

	vec4 reflectedPointWS = camera.iview * vec4(reflectedPointVS, 1.0f);
	vec4 oldReflectedPointVS = camera.viewOld * reflectedPointWS;

	vec4 oldReflectedPointNDC = camera.projection * oldReflectedPointVS;

	return (oldReflectedPointNDC.xy / oldReflectedPointNDC.w - reflectedPointNDC.xy / reflectedPointNDC.w) * 0.5f;
}

vec4 resolveRay(vec2 uv, vec4 traceResult, vec2 motionVector)
{
	float roughness = texture(texGBufferShading, uv).r;
	int maxMip = textureMips(texCompositeHDRColor);

	// TODO: sample mip based on roughness value
	vec4 result;
	result.rgb = textureLod(texCompositeHDRColor, traceResult.xy + motionVector, 0.0f).rgb;
	result.a = traceResult.z;

	return result;
}

const int numResolveSamples = 5;

const vec2 resolveOffsets[5] =
{
	vec2(-2.0f, 0.0f),
	vec2(-1.0f, 0.0f),
	vec2( 0.0f, 0.0f),
	vec2( 1.0f, 0.0f),
	vec2( 2.0f, 0.0f)
};

const float resolveWeights[5] =
{
	0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f,
};

// [Jimenez 2014] "Next Generation Post Processing In Call Of Duty Advanced Warfare"
float InterleavedGradientNoise(in vec2 pos, in vec2 random)
{
	vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
	return fract(magic.z * fract(dot(pos.xy + random, magic.xy)));
}

/*
 */
void main()
{
	vec3 positionVS = getGBufferPositionVS(inUV, camera.iprojection);
	vec3 normalVS = getGBufferNormalVS(inUV);

	vec3 viewVS = -normalize(positionVS);
	vec2 shading = texture(texGBufferShading, inUV).rg;

	Material material;
	material.albedo = texture(texGBufferBaseColor, inUV).rgb;
	material.roughness = shading.r;
	material.metalness = shading.g;
	material.ao = 1.0f;
	material.f0 = mix(vec3(0.04f), material.albedo, material.metalness);

	float dotNV = max(0.0f, dot(normalVS, viewVS));
	vec3 F = F_Shlick(dotNV, material.f0, material.roughness);

	vec2 isize = 1.0f / vec2(textureSize(texSSR, 0));

	vec2 noise_uv = inUV * textureSize(texGBufferBaseColor, 0) / textureSize(texSSRNoise, 0).xy;

	// TODO: better TAA noise
	noise_uv.xy += applicationState.time;

	vec4 blue_noise = texture(texSSRNoise, noise_uv);

	float theta = PI2 * InterleavedGradientNoise(inUV, blue_noise.xy);
	float sin_theta = sin(theta);
	float cos_theta = cos(theta);
	mat2 rotation = mat2(cos_theta, sin_theta, -sin_theta, cos_theta);

	outSSR = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	outSSRVelocity = vec2(0.0f, 0.0f);

	for (int i = 0; i < numResolveSamples; ++i)
	{
		vec2 offset = rotation * resolveOffsets[i];
		vec2 uv = inUV + offset * isize;

		vec4 traceResult = texture(texSSR, uv);
		vec2 motionVector = getMotionVector(uv, traceResult);

		outSSR += resolveRay(uv, traceResult, motionVector) * resolveWeights[i];
		outSSRVelocity += motionVector;
	}

	outSSRVelocity /= numResolveSamples;
	outSSR.rgb *= F;
}
