#ifndef RENDER_STATE_H_
#define RENDER_STATE_H_

layout(set = 0, binding = 0) uniform RenderState {
	mat4 world;
	mat4 view;
	mat4 invView;
	mat4 proj;
	mat4 invProj;
	vec4 cameraParams; // (zNear, zFar, 1.0f / zNear, 1.0f / zFar)
	vec3 cameraPosWS;
	float lerpUserValues;
	float userMetalness;
	float userRoughness;
} ubo;

float getLinearDepth(float depth)
{
	// TODO: figure out why is this different from the formula:
	// linear_depth = (zNear * zFar) / (zFar + depth * (zFar - zNear));
	vec4 ndc = vec4(0.0f, 0.0f, depth, 1.0f);

	vec4 viewPos = ubo.invProj * ndc;
	return viewPos.z / viewPos.w;
}

#endif // RENDER_STATE_H_
