#version 450
#pragma shader_stage(vertex)

#define APPLICATION_STATE_SET 0
#include <shaders/common/ApplicationState.h>

#define CAMERA_SET 1
#include <shaders/common/Camera.h>

#define PBR_MATERIAL_SET 2
#include <shaders/materials/pbr/MaterialData.h>

layout(push_constant) uniform Node
{
	mat4 transform;
} node;

// Input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBinormal;
layout(location = 4) in vec3 inNormal;
layout(location = 5) in vec3 inColor;

// Output
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outTangentVS;
layout(location = 2) out vec3 outBinormalVS;
layout(location = 3) out vec3 outNormalVS;
layout(location = 4) out vec4 outPositionNDC;
layout(location = 5) out vec4 outPositionOldNDC;

void main()
{
	mat4 modelview = camera.view * node.transform;
	mat4 modelviewOld = camera.viewOld * node.transform; // TODO: old node transform

	outUV = inUV;
	outTangentVS = vec3(modelview * vec4(inTangent, 0.0f));
	outBinormalVS = vec3(modelview * vec4(inBinormal, 0.0f));
	outNormalVS = vec3(modelview * vec4(inNormal, 0.0f));
	outPositionNDC = vec4(camera.projection * modelview * vec4(inPosition, 1.0f));
	outPositionOldNDC = vec4(camera.projection * modelviewOld * vec4(inPosition, 1.0f));

	gl_Position = outPositionNDC;
}
