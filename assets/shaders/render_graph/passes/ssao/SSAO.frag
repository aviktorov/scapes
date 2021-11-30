#version 450

// Includes
#include <shaders/common/Common.h>
#include <shaders/common/GBuffer.h>

// Bindings
#define RENDER_GRAPH_CAMERA_SET 0
#define RENDER_GRAPH_SSAO_SET 1
#include <shaders/render_graph/common/ParameterGroups.h>

layout(set = 2, binding = 0) uniform sampler2D tex_gbuffer_normal;
layout(set = 2, binding = 1) uniform sampler2D tex_gbuffer_depth;
layout(set = 2, binding = 2) uniform sampler2D tex_ssao_noise;

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out float out_ssao;

//
float getOcclusion(vec3 origin_vs, mat3 tbn, float radius, float bias, int num_samples)
{
	float occlusion = 0.0f;
	float originDepth = origin_vs.z;

	for (int i = 0; i < num_samples; ++i)
	{
		vec3 offset = ssao.samples[i].xyz;
		vec3 sample_position_vs = origin_vs + tbn * offset * radius;

		vec2 sample_uv = getUV(sample_position_vs, camera.projection);

		vec3 gbuffer_position_vs = getGBufferPositionVS(tex_gbuffer_depth, sample_uv, camera.iprojection);

		float sampleDepth = sample_position_vs.z;
		float gbufferDepth = gbuffer_position_vs.z - bias;

		float rangeCheck = smoothstep(1.0f, 0.0f, abs(originDepth - gbufferDepth) / radius);

		float occluded = (gbufferDepth > sampleDepth) ? 1.0f : 0.0f;
		occlusion += occluded * rangeCheck;
	}

	return occlusion;
}

void main()
{
	vec3 origin_vs = getGBufferPositionVS(tex_gbuffer_depth, in_uv, camera.iprojection);

	vec2 noise_uv = in_uv * textureSize(tex_gbuffer_depth, 0) / textureSize(tex_ssao_noise, 0).xy;
	vec3 random_direction = vec3(texture(tex_ssao_noise, noise_uv).xy, 0.0f);

	vec3 normal_vs = getGBufferNormalVS(tex_gbuffer_normal, in_uv);
	vec3 tangent_vs = normalize(random_direction - normal_vs * dot(random_direction, normal_vs));
	vec3 binormal_vs = cross(normal_vs, tangent_vs);

	mat3 tbn = mat3(tangent_vs, binormal_vs, normal_vs);

	float occlusion = getOcclusion(origin_vs, tbn, ssao.radius, 0.01f, ssao.num_samples);

	out_ssao = 1.0f - occlusion / ssao.num_samples;
	out_ssao = pow(out_ssao, ssao.intensity);
}
