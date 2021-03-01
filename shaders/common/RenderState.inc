#ifndef RENDER_STATE_H_
#define RENDER_STATE_H_

layout(set = 0, binding = 0) uniform RenderState {
	mat4 view;
	mat4 iview;
	mat4 projection;
	mat4 iprojection;
	mat4 viewOld;
	vec4 cameraParams; // (zNear, zFar, 1.0f / zNear, 1.0f / zFar)
	vec3 cameraPosWS;
	float lerpUserValues;
	float userMetalness;
	float userRoughness;
	float currentTime;
} ubo;

float getLinearDepth(float depth)
{
	// TODO: figure out why is this different from the formula:
	// linear_depth = (zNear * zFar) / (zFar + depth * (zFar - zNear));
	vec4 ndc = vec4(0.0f, 0.0f, depth, 1.0f);

	vec4 viewPos = ubo.iprojection * ndc;
	return viewPos.z / viewPos.w;
}

#endif // RENDER_STATE_H_
