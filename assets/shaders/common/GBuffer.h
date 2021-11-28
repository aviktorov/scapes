#ifndef GBUFFER_INC_
#define GBUFFER_INC_

struct GBuffer
{
	vec3 baseColor;
	vec3 normalVS;
	float roughness;
	float metalness;
	float depth;
};

GBuffer sampleGBuffer(
	in sampler2D texBaseColor,
	in sampler2D texNormal,
	in sampler2D texShading,
	in sampler2D texDepth,
	in vec2 uv
)
{
	GBuffer result;

	vec3 normal = texture(texNormal, uv).xyz;
	vec2 shading = texture(texShading, uv).rg;

	result.baseColor = texture(texBaseColor, uv).rgb;
	result.normalVS = normalize(normal);
	result.metalness = shading.r;
	result.roughness = shading.g;
	result.depth = texture(texDepth, uv).r;

	return result;
}

vec3 getGBufferNormalVS(in sampler2D texNormal, vec2 uv)
{
	return normalize(texture(texNormal, uv).xyz);
}

vec3 getGBufferPositionVS(vec2 uv, float depth, mat4 iprojection)
{
	vec4 ndc = vec4(uv * 2.0f - vec2(1.0f), depth, 1.0f);

	vec4 positionVS = iprojection * ndc;
	return positionVS.xyz / positionVS.w;
}

vec3 getGBufferPositionVS(in sampler2D texDepth, vec2 uv, mat4 iprojection)
{
	float depth = texture(texDepth, uv).r;
	return getGBufferPositionVS(uv, depth, iprojection);
}

vec3 getGBufferDirectionVS(in sampler2D texDepth, vec2 uv, mat4 iprojection)
{
	return normalize(getGBufferPositionVS(texDepth, uv, iprojection));
}

#endif // GBUFFER_INC_
