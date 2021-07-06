#ifndef CAMERA_H_
#define CAMERA_H_

layout(set = CAMERA_SET, binding = 0) uniform Camera
{
	mat4 view;
	mat4 iview;
	mat4 projection;
	mat4 iprojection;
	mat4 viewOld;
	vec4 parameters; // (zNear, zFar, 1.0f / zNear, 1.0f / zFar)
	vec3 positionWS;
} camera;

float getLinearDepth(float depth)
{
	// TODO: try this
	// linear_depth = (zNear * zFar) / (zFar + depth * (zNear - zFar));
	vec4 ndc = vec4(0.0f, 0.0f, depth, 1.0f);

	vec4 positionVS = camera.iprojection * ndc;
	return positionVS.z / positionVS.w;
}

vec2 getUV(vec3 positionVS)
{
	vec4 ndc = camera.projection * vec4(positionVS, 1.0f);
	ndc.xyz /= ndc.w;

	return ndc.xy * 0.5f + vec2(0.5f);
}

#endif // CAMERA_H_
