#version 450
#pragma shader_stage(fragment)

// TODO: add set define
#include <common/RenderState.inc>

#define GBUFFER_SET 1
#include <deferred/gbuffer.inc>

#define SSR_DATA_SET 2
#include <deferred/ssr_data.inc>

#define LBUFFER_SET 3
#include <deferred/lbuffer.inc>

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outSSR;

vec2 getUV(vec3 positionVS)
{
	vec4 ndc = ubo.proj * vec4(positionVS, 1.0f);
	ndc.xyz /= ndc.w;
	return ndc.xy * 0.5f + vec2(0.5f);
}

/*
 */
void main()
{
	vec3 positionVS = getPositionVS(fragTexCoord, ubo.invProj);
	vec3 normalVS = getNormalVS(fragTexCoord);

	vec3 viewVS = -normalize(positionVS);
	vec3 reflectionVS = -reflect(viewVS, normalVS);

	outSSR.rgb = vec3(0.0f, 0.0f, 0.0f);
	outSSR.a = 0.0f;

	vec2 shading = texture(gbufferShading, fragTexCoord).rg;

	SurfaceMaterial material;
	material.albedo = texture(gbufferBaseColor, fragTexCoord).rgb;
	material.roughness = shading.r;
	material.metalness = shading.g;
	material.ao = 1.0f;
	material.f0 = lerp(vec3(0.04f), material.albedo, material.metalness);

	float dotNV = max(0.0f, dot(normalVS, viewVS));
	vec3 F = F_Shlick(dotNV, material.f0, material.roughness);

	// Coarse tracing
	int intersection_index = -1;

	for (int i = 1; i <= ssr_data.num_steps; i++)
	{
		vec3 ray_positionVS = positionVS + reflectionVS * ssr_data.step * i;

		vec2 uv = getUV(ray_positionVS);
		if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
			discard;

		vec3 gbuffer_sample_positionVS = getPositionVS(uv, ubo.invProj);

		float sample_depth = ray_positionVS.z;
		float gbuffer_depth = gbuffer_sample_positionVS.z;

		if (gbuffer_depth > sample_depth)
		{
			intersection_index = i;
			break;
		}
	}

	if (intersection_index == -1)
		discard;

	// Precise tracing
	vec3 start = positionVS + reflectionVS * ssr_data.step * (intersection_index - 1);
	vec3 end = positionVS + reflectionVS * ssr_data.step * intersection_index;

	for (int i = 0; i < ssr_data.num_precision_steps; i++)
	{
		vec3 mid = (start + end) * 0.5f;

		vec2 uv = getUV(mid);
		if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
			discard;

		vec3 gbuffer_sample_positionVS = getPositionVS(uv, ubo.invProj);

		float sample_depth = mid.z;
		float gbuffer_depth = gbuffer_sample_positionVS.z;

		float delta = sample_depth - gbuffer_depth;
		if (delta < 0)
			end = mid;
		else
			start = mid;

		if (abs(delta) < ssr_data.precision_step_depth_threshold)
			break;
	}

	vec3 intersection = (start + end) * 0.5f;
	vec2 uv = getUV(intersection);
	if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
		discard;

	vec3 gbuffer_sample_positionVS = getPositionVS(uv, ubo.invProj);

	float sample_depth = intersection.z;
	float gbuffer_depth = gbuffer_sample_positionVS.z;

	float delta = abs(sample_depth - gbuffer_depth);

	if (delta > ssr_data.bypass_depth_threshold)
		discard;

	vec3 color = texture(lbufferDiffuse, uv).rgb + texture(lbufferSpecular, uv).rgb;

	outSSR.rgb = color * F;
	outSSR.a = 1.0f;
}
