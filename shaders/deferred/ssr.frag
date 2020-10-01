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

/*
 */
void main()
{
	vec3 positionVS = getPositionVS(fragTexCoord, ubo.invProj);
	vec3 normalVS = getNormalVS(fragTexCoord);

	vec3 direction_to_camera = -normalize(positionVS);
	vec3 reflectionVS = -reflect(direction_to_camera, normalVS);

	outSSR.rgb = vec3(0.0f, 0.0f, 0.0f);
	outSSR.a = 0.0f;

	for (int i = 1; i <= ssr_data.num_steps; i++)
	{
		vec3 ray_positionVS = positionVS + reflectionVS * ssr_data.step * i;

		vec4 ndc = ubo.proj * vec4(ray_positionVS, 1.0f);
		ndc.xyz /= ndc.w;
		ndc.xy = ndc.xy * 0.5f + vec2(0.5f);

		vec3 gbuffer_sample_positionVS = getPositionVS(ndc.xy, ubo.invProj);

		float sample_depth = ray_positionVS.z;
		float gbuffer_depth = gbuffer_sample_positionVS.z;

		if (gbuffer_depth > sample_depth)
		{
			vec3 color = texture(lbufferDiffuse, ndc.xy).rgb + texture(lbufferSpecular, ndc.xy).rgb;

			outSSR.rgb = color;
			outSSR.a = 1.0f;

			break;
		}
	}
}
