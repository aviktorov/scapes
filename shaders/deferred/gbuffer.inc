#ifndef GBUFFER_INC_
#define GBUFFER_INC_

layout(set = GBUFFER_SET, binding = 0) uniform sampler2D gbufferBaseColor;
layout(set = GBUFFER_SET, binding = 1) uniform sampler2D gbufferNormal;
layout(set = GBUFFER_SET, binding = 2) uniform sampler2D gbufferShading;
layout(set = GBUFFER_SET, binding = 3) uniform sampler2D gbufferDepth;

vec3 getNormalVS(vec2 uv)
{
	vec3 normalVS = texture(gbufferNormal, uv).xyz;
	return normalize(normalVS);
}

vec3 getPositionVS(vec2 uv, mat4 iprojection)
{
	float depth = texture(gbufferDepth, uv).r;
	vec4 ndc = vec4(uv * 2.0f - vec2(1.0f), depth, 1.0f);

	vec4 viewPos = iprojection * ndc;
	return viewPos.xyz / viewPos.w;
}

vec3 getDirectionVS(vec2 uv, mat4 iprojection)
{
	return normalize(getPositionVS(uv, iprojection));
}

#endif // GBUFFER_INC_
