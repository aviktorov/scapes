#version 450
#pragma shader_stage(fragment)

layout(binding = 0) uniform UniformBufferObject {
	mat4 faces[6];
} ubo;

layout(binding = 1) uniform sampler2D environmentSampler;

layout(location = 0) in vec4 fragFacePositions[6];

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;
layout(location = 2) out vec4 outColor2;
layout(location = 3) out vec4 outColor3;
layout(location = 4) out vec4 outColor4;
layout(location = 5) out vec4 outColor5;

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

vec4 fillFace(int index)
{
	vec3 dir = normalize(fragFacePositions[index].xyz);
	vec2 uv = SampleSphericalMap(dir);
	vec4 color;
	color.rgb = texture(environmentSampler, uv).rgb;
	color.a = 1.0f;
 
	return color;
}

void main()
{
	outColor0 = fillFace(0);
	outColor1 = fillFace(1);
	outColor2 = fillFace(2);
	outColor3 = fillFace(3);
	outColor4 = fillFace(4);
	outColor5 = fillFace(5);
}
