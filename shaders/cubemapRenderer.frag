#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat3 faces[6];
} ubo;

layout(binding = 1) uniform sampler2D environmentSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragFacePositions[6];

layout(location = 0) out vec4 outColor;

const float PI = 3.141592653589798979f;
const float iPI = 0.31830988618379f;

//////////////// Equirectangular world

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.y, v.x), asin(v.z));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main() {

	for (int i = 0; i < 6; i++)
	{
		vec3 dir = normalize(fragFacePositions[i]);
		vec2 uv = SampleSphericalMap(dir);
		vec3 color = texture(environmentSampler, uv).rgb;
		outColor = vec4(color, 1.0f);
	}
}
