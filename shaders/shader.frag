#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 world;
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
} ubo;

layout(binding = 1) uniform sampler2D albedoSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D aoSampler;
layout(binding = 4) uniform sampler2D shadingSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragTangentWS;
layout(location = 3) in vec3 fragBinormalWS;
layout(location = 4) in vec3 fragNormalWS;
layout(location = 5) in vec3 fragPositionWS;

layout(location = 0) out vec4 outColor;

struct Surface
{
	vec3 light;
	vec3 view;
	vec3 normal;
};

struct Material
{
	float shininess;
};

vec3 DiffuseLambertBRDF(Surface surface, Material material)
{
	float dotNL = max(dot(surface.normal, surface.light), 0.0f);
	return vec3(1.0f, 1.0f, 1.0f) * dotNL;
}

vec3 DiffuseHalfLambertBRDF(Surface surface, Material material)
{
	float dotNL = max(dot(surface.normal, surface.light), 0.0f);
	float halfLambert = (dotNL + 1) * 0.5f;
	return vec3(1.0f, 1.0f, 1.0f) * halfLambert * halfLambert;
}

vec3 SpecularBlinnPhongBRDF(Surface surface, Material material)
{
	vec3 h = normalize(surface.light + surface.view);
	float dotNH = max(dot(surface.normal, h), 0.0f);
	return vec3(1.0f, 1.0f, 1.0f) * pow(dotNH, material.shininess);
}

vec3 SpecularPhongBRDF(Surface surface, Material material)
{
	vec3 r = reflect(-surface.light, surface.normal);
	float dotRV = max(dot(r, surface.view), 0.0f);
	return vec3(1.0f, 1.0f, 1.0f) * pow(dotRV, material.shininess / 4.0f);
}

vec3 BRDF(Surface surface, Material material)
{
	vec3 diffuse = DiffuseHalfLambertBRDF(surface, material);
	vec3 specular = SpecularBlinnPhongBRDF(surface, material);

	return diffuse + specular;
}

float lerp(float a, float b, float t)
{
	return a * (1.0f - t) + b * t;
}

void main() {
	vec3 lightPos = ubo.cameraPos;
	vec3 lightDirWS = normalize(lightPos - fragPositionWS);
	vec3 cameraDirWS = normalize(ubo.cameraPos - fragPositionWS);

	vec3 normal = texture(normalSampler, fragTexCoord).xyz * 2.0f - vec3(1.0f, 1.0f, 1.0f);

	mat3 m;
	m[0] = fragTangentWS;
	m[1] = fragBinormalWS;
	m[2] = fragNormalWS;

	Surface surface;
	surface.light = lightDirWS;
	surface.view = cameraDirWS;
	surface.normal = normalize(m * normal);

	Material material;
	material.shininess = 10.0f;

	vec3 brdf = BRDF(surface, material);

	outColor = vec4(brdf, 1.0f) * texture(albedoSampler, fragTexCoord) * texture(aoSampler, fragTexCoord);
}
