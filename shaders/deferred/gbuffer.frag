#version 450
#pragma shader_stage(fragment)

#include <common/RenderState.inc>
#include <common/brdf.inc>

layout(set = 1, binding = 0) uniform sampler2D baseColorSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D roughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D metalnessSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragTangentVS;
layout(location = 3) in vec3 fragBinormalVS;
layout(location = 4) in vec3 fragNormalVS;
layout(location = 5) in vec3 fragPositionVS;
layout(location = 6) in vec4 fragPositionNdc;
layout(location = 7) in vec4 fragPositionNdcOld;

layout(location = 0) out vec4 outBaseColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outShading;
layout(location = 3) out vec2 outVelocity;

void main()
{
	vec4 baseColor = texture(baseColorSampler, fragTexCoord);
	if (baseColor.a < 0.5f)
		discard;

	vec3 normalTS = texture(normalSampler, fragTexCoord).xyz * 2.0f - vec3(1.0f);

	mat3 TSToVS;
	TSToVS[0] = normalize(fragTangentVS);
	TSToVS[1] = normalize(-fragBinormalVS);
	TSToVS[2] = normalize(fragNormalVS);

	vec3 normalVS = normalize(TSToVS * normalTS);
	float roughness = texture(roughnessSampler, fragTexCoord).r;
	float metalness = texture(metalnessSampler, fragTexCoord).r;

	baseColor.rgb = lerp(baseColor.rgb, vec3(0.5f, 0.5f, 0.5f), ubo.lerpUserValues);
	roughness = lerp(roughness, ubo.userRoughness, ubo.lerpUserValues);
	metalness = lerp(metalness, ubo.userMetalness, ubo.lerpUserValues);

	vec2 velocity = (fragPositionNdcOld.xy / fragPositionNdcOld.w - fragPositionNdc.xy / fragPositionNdc.w) * 0.5f;

	outBaseColor.rgb = baseColor.rgb;
	outBaseColor.a = 1.0f;

	outNormal = normalVS.xyz;
	outShading = vec2(roughness, metalness);

	outVelocity = velocity;
}
