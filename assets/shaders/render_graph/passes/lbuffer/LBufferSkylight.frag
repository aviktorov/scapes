#version 450

// Includes
#include <shaders/common/GBuffer.h>

// Bindings
#define RENDER_GRAPH_CAMERA_SET 0
#include <shaders/render_graph/common/Groups.h>

layout(set = 1, binding = 0) uniform sampler2D tex_gbuffer_basecolor;
layout(set = 2, binding = 0) uniform sampler2D tex_gbuffer_normal;
layout(set = 3, binding = 0) uniform sampler2D tex_gbuffer_shading;
layout(set = 4, binding = 0) uniform sampler2D tex_gbuffer_depth;

#define RENDER_GRAPH_SKYLIGHT_SET 5
#include <shaders/render_graph/common/Skylight.h>

// Input
layout(location = 0) in vec2 in_uv;

// Output
layout(location = 0) out vec4 out_lbuffer_diffuse;
layout(location = 1) out vec4 out_lbuffer_specular;

//
void main()
{
	GBuffer gbuffer = sampleGBuffer(tex_gbuffer_basecolor, tex_gbuffer_normal, tex_gbuffer_shading, tex_gbuffer_depth, in_uv);
	vec3 direction_vs = -getGBufferDirectionVS(gbuffer.depth, in_uv, camera.iprojection);

	vec3 normal_ws = normalize(vec3(camera.iview * vec4(gbuffer.normal_vs, 0.0f)));
	vec3 direction_ws = normalize(vec3(camera.iview * vec4(direction_vs, 0.0f)));

	Material material;
	material.albedo = gbuffer.basecolor;
	material.roughness = gbuffer.roughness;
	material.metalness = gbuffer.metalness;
	material.ao = 1.0f;
	material.f0 = mix(vec3(0.04f), material.albedo, material.metalness);

	out_lbuffer_diffuse.rgb = SkyLight_Diffuse(normal_ws, direction_ws, material);
	out_lbuffer_specular.rgb = SkyLight_Specular(normal_ws, direction_ws, material);

	out_lbuffer_diffuse.a = 1.0f;
	out_lbuffer_specular.a = 1.0f;
}
