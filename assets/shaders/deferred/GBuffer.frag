#version 450
#pragma shader_stage(fragment)

#define APPLICATION_STATE_SET 0
#include <common/ApplicationState.h>

#define CAMERA_SET 1
#include <common/Camera.h>

#define PBR_MATERIAL_SET 2
#include <materials/pbr/MaterialData.h>

// Input
layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inTangentVS;
layout(location = 2) in vec3 inBinormalVS;
layout(location = 3) in vec3 inNormalVS;
layout(location = 4) in vec4 inPositionNDC;
layout(location = 5) in vec4 inPositionOldNDC;

// Output
#define GBUFFER_WRITE
#include <deferred/GBuffer.h>

void main()
{
	mat3 TBN;
	TBN[0] = normalize(inTangentVS);
	TBN[1] = normalize(-inBinormalVS);
	TBN[2] = normalize(inNormalVS);

	MaterialTextures material = sampleMaterial(inUV, TBN);

	if (material.baseColor.a < 0.5f)
		discard;

	material.baseColor.rgb = mix(material.baseColor.rgb, vec3(0.5f, 0.5f, 0.5f), applicationState.lerpUserValues);
	material.roughness = mix(material.roughness, applicationState.userRoughness, applicationState.lerpUserValues);
	material.metalness = mix(material.metalness, applicationState.userMetalness, applicationState.lerpUserValues);

	vec2 velocity = (inPositionOldNDC.xy / inPositionOldNDC.w - inPositionNDC.xy / inPositionNDC.w) * 0.5f;

	writeGBuffer(material.baseColor, material.normalVS, material.roughness, material.metalness, velocity);
}
