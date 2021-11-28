#version 450

// Bindings
#define RENDER_GRAPH_APPLICATION_SET 0
#define RENDER_GRAPH_CAMERA_SET 1
#include <shaders/render_graph/common/ParameterGroups.h>

layout(push_constant) uniform Node
{
	mat4 transform;
} node;

// Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_binormal;
layout(location = 4) in vec3 in_normal;
layout(location = 5) in vec3 in_color;

// Output
layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_tangent_vs;
layout(location = 2) out vec3 out_binormal_vs;
layout(location = 3) out vec3 out_normal_vs;
layout(location = 4) out vec4 out_position_ndc;
layout(location = 5) out vec4 out_position_old_ndc;

//
void main()
{
	mat4 modelview = camera.view * node.transform;
	mat4 modelview_old = camera.view_old * node.transform; // TODO: old node transform

	out_uv = in_uv;
	out_tangent_vs = vec3(modelview * vec4(in_tangent.xyz, 0.0f));
	out_binormal_vs = vec3(modelview * vec4(in_binormal, 0.0f));
	out_normal_vs = vec3(modelview * vec4(in_normal, 0.0f));
	out_position_ndc = vec4(camera.projection * modelview * vec4(in_position, 1.0f));
	out_position_old_ndc = vec4(camera.projection * modelview_old * vec4(in_position, 1.0f));

	gl_Position = out_position_ndc;
}
