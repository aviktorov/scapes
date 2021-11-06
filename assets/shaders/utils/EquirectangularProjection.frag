#version 450

layout(set = 0, binding = 1) uniform sampler2D texEnvironmentRequirectangular;

layout(location = 0) in vec4 inFacePositionVS[6];

layout(location = 0) out vec4 outHDRColor0;
layout(location = 1) out vec4 outHDRColor1;
layout(location = 2) out vec4 outHDRColor2;
layout(location = 3) out vec4 outHDRColor3;
layout(location = 4) out vec4 outHDRColor4;
layout(location = 5) out vec4 outHDRColor5;

const float PI = 3.141592653589798979f;
const float iPI = 0.31830988618379f;

const vec2 invAtan = vec2(0.1591f, 0.3183f);
vec2 sampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.y, v.x), asin(v.z));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

vec4 fillFace(int index)
{
	vec3 direction = normalize(inFacePositionVS[index].xyz);
	vec2 uv = sampleSphericalMap(direction);

	vec4 color;
	color.rgb = texture(texEnvironmentRequirectangular, uv).rgb;
	color.a = 1.0f;
 
	return color;
}

void main()
{
	outHDRColor0 = fillFace(0);
	outHDRColor1 = fillFace(1);
	outHDRColor2 = fillFace(2);
	outHDRColor3 = fillFace(3);
	outHDRColor4 = fillFace(4);
	outHDRColor5 = fillFace(5);
}
