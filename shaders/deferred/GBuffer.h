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

#ifdef GBUFFER_WRITE
	layout(location = 0) out vec4 outGBufferBaseColor;
	layout(location = 1) out vec3 outGBufferNormal;
	layout(location = 2) out vec2 outGBufferShading;
	layout(location = 3) out vec2 outGBufferVelocity;
#else
	layout(set = GBUFFER_SET, binding = 0) uniform sampler2D texGBufferBaseColor;
	layout(set = GBUFFER_SET, binding = 1) uniform sampler2D texGBufferNormal;
	layout(set = GBUFFER_SET, binding = 2) uniform sampler2D texGBufferShading;
	layout(set = GBUFFER_SET, binding = 3) uniform sampler2D texGBufferDepth;
#endif

#ifdef GBUFFER_WRITE
	void writeGBuffer(vec4 baseColor, vec3 normalVS, float roughness, float metalness, vec2 velocity)
	{
		outGBufferBaseColor.rgb = baseColor.rgb;
		outGBufferBaseColor.a = 0.0f;

		outGBufferNormal = normalVS.xyz;
		outGBufferShading = vec2(roughness, metalness);

		outGBufferVelocity = velocity;
	}
#else
	GBuffer sampleGBuffer(in vec2 uv)
	{
		GBuffer result;

		vec3 normal = texture(texGBufferNormal, uv).xyz;
		vec2 shading = texture(texGBufferShading, uv).rg;

		result.baseColor = texture(texGBufferBaseColor, uv).rgb;
		result.normalVS = normalize(normal);
		result.roughness = shading.r;
		result.metalness = shading.g;
		result.depth = texture(texGBufferDepth, uv).r;

		return result;
	}

	vec3 getGBufferNormalVS(vec2 uv)
	{
		return normalize(texture(texGBufferNormal, uv).xyz);
	}

	vec3 getGBufferPositionVS(vec2 uv, float depth, mat4 iprojection)
	{
		vec4 ndc = vec4(uv * 2.0f - vec2(1.0f), depth, 1.0f);

		vec4 positionVS = iprojection * ndc;
		return positionVS.xyz / positionVS.w;
	}

	vec3 getGBufferPositionVS(vec2 uv, mat4 iprojection)
	{
		float depth = texture(texGBufferDepth, uv).r;
		return getGBufferPositionVS(uv, depth, iprojection);
	}

	vec3 getGBufferDirectionVS(vec2 uv, mat4 iprojection)
	{
		return normalize(getGBufferPositionVS(uv, iprojection));
	}
#endif // GBUFFER_WRITE

#endif // GBUFFER_INC_
