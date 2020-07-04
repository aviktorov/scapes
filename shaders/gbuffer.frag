#version 450
#pragma shader_stage(fragment)

#include "RenderState.inc"
#include "brdf.inc"

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D roughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D metalnessSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragTangentVS;
layout(location = 3) in vec3 fragBinormalVS;
layout(location = 4) in vec3 fragNormalVS;
layout(location = 5) in vec3 fragPositionVS;

layout(location = 0) out vec4 outBaseColor;
layout(location = 1) out float outDepth;
layout(location = 2) out vec2 outNormal;
layout(location = 3) out vec2 outShading;

void main()
{
	vec3 normalTS = texture(normalSampler, fragTexCoord).xyz * 2.0f - vec3(1.0f);

	mat3 TSToVS;
	TSToVS[0] = normalize(fragTangentVS);
	TSToVS[1] = normalize(-fragBinormalVS);
	TSToVS[2] = normalize(fragNormalVS);

	vec3 normalVS = normalize(TSToVS * normalTS);
	vec3 albedo = texture(albedoSampler, fragTexCoord).rgb;
	float roughness = texture(roughnessSampler, fragTexCoord).r;
	float metalness = texture(metalnessSampler, fragTexCoord).r;

	outBaseColor.rgb = albedo;
	outBaseColor.a = 1.0f;

	outNormal = normalVS.xy * 0.5f + vec2(0.5f);
	outDepth = length(fragPositionVS);
	outShading = vec2(roughness, metalness);
}
