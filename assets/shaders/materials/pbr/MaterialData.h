#ifndef PBR_MATERIAL_DATA_H_
#define PBR_MATERIAL_DATA_H_

struct MaterialTextures
{
	vec4 baseColor;
	vec3 normalVS;
	float roughness;
	float metalness;
};

layout(set = PBR_MATERIAL_SET, binding = 0) uniform sampler2D texMaterialBaseColor;
layout(set = PBR_MATERIAL_SET, binding = 1) uniform sampler2D texMaterialNormal;
layout(set = PBR_MATERIAL_SET, binding = 2) uniform sampler2D texMaterialRoughness;
layout(set = PBR_MATERIAL_SET, binding = 3) uniform sampler2D texMaterialMetalness;

MaterialTextures sampleMaterial(in vec2 uv, in mat3 TBN)
{
	MaterialTextures result;

	result.baseColor = texture(texMaterialBaseColor, uv);
	vec3 normalTS = texture(texMaterialNormal, uv).xyz * 2.0f - vec3(1.0f);

	result.normalVS = normalize(TBN * normalTS);
	result.roughness = texture(texMaterialRoughness, uv).r;
	result.metalness = texture(texMaterialMetalness, uv).r;

	return result;
}

#endif // PBR_MATERIAL_DATA_H_
