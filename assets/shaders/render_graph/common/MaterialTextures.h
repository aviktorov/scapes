#ifndef MATERIAL_TEXTURES_H_
#define MATERIAL_TEXTURES_H_

struct MaterialTextures
{
	vec4 basecolor;
	vec3 normal_vs;
	float roughness;
	float metalness;
};

layout(set = RENDER_GRAPH_MATERIAL_TEXTURES_SET, binding = 0) uniform sampler2D tex_material_basecolor;
layout(set = RENDER_GRAPH_MATERIAL_TEXTURES_SET, binding = 1) uniform sampler2D tex_material_normal;
layout(set = RENDER_GRAPH_MATERIAL_TEXTURES_SET, binding = 2) uniform sampler2D tex_material_roughness;
layout(set = RENDER_GRAPH_MATERIAL_TEXTURES_SET, binding = 3) uniform sampler2D tex_material_metalness;

MaterialTextures sampleMaterial(in vec2 uv, in mat3 tbn)
{
	MaterialTextures result;

	result.basecolor = texture(tex_material_basecolor, uv);
	vec3 normal_ts = texture(tex_material_normal, uv).xyz * 2.0f - vec3(1.0f);

	result.normal_vs = normalize(tbn * normal_ts);
	result.roughness = texture(tex_material_roughness, uv).r;
	result.metalness = texture(tex_material_metalness, uv).r;

	return result;
}

#endif // MATERIAL_TEXTURES_H_
