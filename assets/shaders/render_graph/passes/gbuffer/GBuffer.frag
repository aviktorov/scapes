#version 450

// Bindings
#define RENDER_GRAPH_APPLICATION_SET 0
#define RENDER_GRAPH_CAMERA_SET 1
#include <shaders/render_graph/common/Groups.h>

#define RENDER_GRAPH_MATERIAL_TEXTURES_SET 2
#include <shaders/render_graph/common/MaterialTextures.h>

// Input
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_tangent_vs;
layout(location = 2) in vec3 in_binormal_vs;
layout(location = 3) in vec3 in_normal_vs;
layout(location = 4) in vec4 in_position_ndc;
layout(location = 5) in vec4 in_position_old_ndc;

// Output
layout(location = 0) out vec4 out_gbuffer_basecolor;
layout(location = 1) out vec3 out_gbuffer_normal;
layout(location = 2) out vec2 out_gbuffer_shading;
layout(location = 3) out vec2 out_gbuffer_velocity;

//
void main()
{
	mat3 tbn;
	tbn[0] = normalize(in_tangent_vs);
	tbn[1] = normalize(-in_binormal_vs);
	tbn[2] = normalize(in_normal_vs);

	MaterialTextures material = sampleMaterial(in_uv, tbn);

	if (material.basecolor.a < 0.5f)
		discard;

	material.basecolor.rgb = mix(material.basecolor.rgb, vec3(0.5f, 0.5f, 0.5f), application.override_basecolor);
	material.roughness = mix(material.roughness, application.user_roughness, application.override_shading);
	material.metalness = mix(material.metalness, application.user_metalness, application.override_shading);

	out_gbuffer_basecolor.rgb = material.basecolor.rgb;
	out_gbuffer_basecolor.a = 0.0f;

	out_gbuffer_normal = material.normal_vs.xyz;
	out_gbuffer_shading = vec2(material.metalness, material.roughness);

	out_gbuffer_velocity = (in_position_old_ndc.xy / in_position_old_ndc.w - in_position_ndc.xy / in_position_ndc.w) * 0.5f;
}
