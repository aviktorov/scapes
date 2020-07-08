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

void main()
{
	vec3 normalVS = texture(gbufferBaseColor, fragTexCoord).xyz;
	normalVS.z = sqrt(1.0f - dot(normalVS.xy, normalVS.xy));
	normalVS = normalize(normalVS);

	float linear_depth = texture(gbufferDepth, fragTexCoord).r;
	float normalized_depth = (linear_depth - ubo.cameraParams.x) / (ubo.cameraParams.y - ubo.cameraParams.x);

	vec3 viewVS = normalize(vec3(ubo.invProj * vec4(fragTexCoord.x, fragTexCoord.y, normalized_depth, 1.0f)));

	vec2 shading = texture(gbufferShading, fragTexCoord).rg;

	SurfaceMaterial material;
	material.albedo = texture(gbufferBaseColor, fragTexCoord).rgb;
	material.roughness = shading.r;
	material.metalness = shading.g;
	material.ao = 1.0f;
	material.f0 = lerp(vec3(0.04f), material.albedo, material.metalness);

	outDiffuse.rgb = SkyLight_Diffuse(normalVS, viewVS, material);
	outSpecular.rgb = SkyLight_Specular(normalVS, viewVS, material);

	outDiffuse.a = 1.0f;
	outSpecular.a = 1.0f;
}
