#version 450
#pragma shader_stage(fragment)

// TODO: add set define
#include <common/RenderState.inc>

#define GBUFFER_SET 1
#include <deferred/gbuffer.inc>

#define SSR_SET 2
#include <deferred/ssr.inc>

#define SSR_DATA_SET 3
#include <deferred/ssr_data.inc>

#define COMPOSITE_SET 4
#include <deferred/composite.inc>

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outSSR;

vec2 getUV(vec3 positionVS)
{
	vec4 ndc = ubo.proj * vec4(positionVS, 1.0f);
	ndc.xyz /= ndc.w;
	return ndc.xy * 0.5f + vec2(0.5f);
}

vec4 resolveRay(vec2 uv)
{
	vec4 traceResult = texture(ssrTexture, uv);
	float roughness = texture(gbufferShading, fragTexCoord).r;
	int maxMip = getTextureNumMips(compositeHdrColor);

	vec4 result;
	result.rgb = textureLod(compositeHdrColor, traceResult.xy, roughness * maxMip).rgb;
	result.a = traceResult.z;

	return result;
}

const int num_samples = 5;

const vec2 offsets[5] = 
{
	vec2(-2.0f, 0.0f),
	vec2(-1.0f, 0.0f),
	vec2( 0.0f, 0.0f),
	vec2( 1.0f, 0.0f),
	vec2( 2.0f, 0.0f)
};

const float weights[5] =
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
	vec3 positionVS = getPositionVS(fragTexCoord, ubo.invProj);
	vec3 normalVS = getNormalVS(fragTexCoord);

	vec3 viewVS = -normalize(positionVS);
	vec2 shading = texture(gbufferShading, fragTexCoord).rg;

	SurfaceMaterial material;
	material.albedo = texture(gbufferBaseColor, fragTexCoord).rgb;
	material.roughness = shading.r;
	material.metalness = shading.g;
	material.ao = 1.0f;
	material.f0 = lerp(vec3(0.04f), material.albedo, material.metalness);

	float dotNV = max(0.0f, dot(normalVS, viewVS));
	vec3 F = F_Shlick(dotNV, material.f0, material.roughness);

	vec2 isize = 1.0f / vec2(textureSize(ssrTexture, 0));

	vec2 noise_uv = fragTexCoord * textureSize(gbufferBaseColor, 0) / textureSize(ssrNoiseTexture, 0).xy;
	vec4 blue_noise = texture(ssrNoiseTexture, noise_uv);

	float theta = PI2 * InterleavedGradientNoise(fragTexCoord, blue_noise.xy);
	float sin_theta = sin(theta);
	float cos_theta = cos(theta);
	mat2 rotation = mat2(cos_theta, sin_theta, -sin_theta, cos_theta);

	outSSR = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	
	for (int i = 0; i < num_samples; ++i)
	{
		vec2 offset = rotation * offsets[i];
		outSSR += resolveRay(fragTexCoord + offset * isize) * weights[i];
	}

	outSSR.rgb *= F;
}
