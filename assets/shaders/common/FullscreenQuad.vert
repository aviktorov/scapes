#version 450
#pragma shader_stage(vertex)

// Input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec3 inBinormal;
layout(location = 4) in vec3 inNormal;
layout(location = 5) in vec4 inColor;

// Output
layout(location = 0) out vec2 outUV;

void main() {
	gl_Position = vec4(inPosition.xyz, 1.0f);
	outUV = inUV;
}
