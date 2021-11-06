#version 450

#include <shaders/materials/pbr/BRDF.h>

#define CAMERA_SET 0
#include <shaders/common/Camera.h>

#define GBUFFER_SET 1
#include <shaders/deferred/GBuffer.h>

#define SKYLIGHT_SET 2
#include <shaders/common/Skylight.h>

// Input
layout(location = 0) in vec2 inUV;

// Output
layout(location = 0) out vec4 outLBufferDiffuse;
layout(location = 1) out vec4 outLBufferSpecular;

void main()
{
	GBuffer gbuffer = sampleGBuffer(inUV);
	vec3 directionVS = -getGBufferDirectionVS(inUV, camera.iprojection);

	vec3 normalWS = normalize(vec3(camera.iview * vec4(gbuffer.normalVS, 0.0f)));
	vec3 directionWS = normalize(vec3(camera.iview * vec4(directionVS, 0.0f)));

	Material material;
	material.albedo = gbuffer.baseColor;
	material.roughness = gbuffer.roughness;
	material.metalness = gbuffer.metalness;
	material.ao = 1.0f;
	material.f0 = mix(vec3(0.04f), material.albedo, material.metalness);

	outLBufferDiffuse.rgb = SkyLight_Diffuse(normalWS, directionWS, material);
	outLBufferSpecular.rgb = SkyLight_Specular(normalWS, directionWS, material);

	outLBufferDiffuse.a = 1.0f;
	outLBufferSpecular.a = 1.0f;
}
