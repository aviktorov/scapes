#ifndef GBUFFER_INC_
#define GBUFFER_INC_

struct GBuffer
{
	vec3 basecolor;
	vec3 normal_vs;
	float roughness;
	float metalness;
	float depth;
};

GBuffer sampleGBuffer(
	in sampler2D tex_basecolor,
	in sampler2D tex_normal,
	in sampler2D tex_shading,
	in sampler2D tex_depth,
	in vec2 uv
)
{
	GBuffer result;

	vec3 normal = texture(tex_normal, uv).xyz;
	vec2 shading = texture(tex_shading, uv).rg;

	result.basecolor = texture(tex_basecolor, uv).rgb;
	result.normal_vs = normalize(normal);
	result.metalness = shading.r;
	result.roughness = shading.g;
	result.depth = texture(tex_depth, uv).r;

	return result;
}

GBuffer sampleGBuffer(
	in sampler2D tex_normal,
	in sampler2D tex_shading,
	in sampler2D tex_depth,
	in vec2 uv
)
{
	GBuffer result;

	vec3 normal = texture(tex_normal, uv).xyz;
	vec2 shading = texture(tex_shading, uv).rg;

	result.basecolor = vec3(1.0f);
	result.normal_vs = normalize(normal);
	result.metalness = shading.r;
	result.roughness = shading.g;
	result.depth = texture(tex_depth, uv).r;

	return result;
}

vec3 getGBufferNormalVS(in sampler2D tex_normal, vec2 uv)
{
	return normalize(texture(tex_normal, uv).xyz);
}

vec3 getGBufferPositionVS(float depth, vec2 uv, mat4 iprojection)
{
	vec4 ndc = vec4(uv * 2.0f - vec2(1.0f), depth, 1.0f);

	vec4 position_vs = iprojection * ndc;
	return position_vs.xyz / position_vs.w;
}

vec3 getGBufferPositionVS(in sampler2D tex_depth, vec2 uv, mat4 iprojection)
{
	float depth = texture(tex_depth, uv).r;
	return getGBufferPositionVS(depth, uv, iprojection);
}

vec3 getGBufferDirectionVS(float depth, vec2 uv, mat4 iprojection)
{
	return normalize(getGBufferPositionVS(depth, uv, iprojection));
}

vec3 getGBufferDirectionVS(in sampler2D tex_depth, vec2 uv, mat4 iprojection)
{
	return normalize(getGBufferPositionVS(tex_depth, uv, iprojection));
}

#endif // GBUFFER_INC_
