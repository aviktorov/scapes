#version 450
#pragma shader_stage(fragment)

#include <common/brdf.inc>

// TODO: add set define
#include <common/RenderState.inc>

#define GBUFFER_SET 1
#include <deferred/gbuffer.inc>

#define SKYLIGHT_SET 2
#include <lights/skylight.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;

vec3 getNormal()
{
	vec3 normalVS = texture(gbufferNormal, fragTexCoord).xyz * 2.0f - vec3(1.0f);
	normalVS.z = sqrt(max(0.0f, 1.0f - dot(normalVS.xy, normalVS.xy)));

	return normalize(vec3(ubo.invView * vec4(normalVS, 0.0f)));
}

vec3 getView()
{
	float depth = texture(gbufferDepth, fragTexCoord).r;

	vec4 viewPos = ubo.invProj * vec4(fragTexCoord * 2.0f - vec2(1.0f), depth, 1.0f);
	viewPos.xyz /= viewPos.w;

	return normalize(vec3(ubo.invView * vec4(-viewPos.xyz, 0.0f)));
}

void main()
{
	vec3 normalWS = getNormal();
	vec3 viewWS = getView();

	vec2 shading = texture(gbufferShading, fragTexCoord).rg;

	SurfaceMaterial material;
	material.albedo = texture(gbufferBaseColor, fragTexCoord).rgb;
	material.roughness = shading.r;
	material.metalness = shading.g;
	material.ao = 1.0f;

	material.albedo = lerp(material.albedo, vec3(0.5f, 0.5f, 0.5f), ubo.lerpUserValues);
	material.roughness = lerp(material.roughness, ubo.userRoughness, ubo.lerpUserValues);
	material.metalness = lerp(material.metalness, ubo.userMetalness, ubo.lerpUserValues);

	material.f0 = lerp(vec3(0.04f), material.albedo, material.metalness);

	outDiffuse.rgb = SkyLight_Diffuse(normalWS, viewWS, material);
	outSpecular.rgb = SkyLight_Specular(normalWS, viewWS, material);

	outDiffuse.a = 1.0f;
	outSpecular.a = 1.0f;
}
