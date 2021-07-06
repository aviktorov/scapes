#ifndef APPLICATION_STATE_H_
#define APPLICATION_STATE_H_

layout(set = APPLICATION_STATE_SET, binding = 0) uniform ApplicationState
{
	float lerpUserValues;
	float userMetalness;
	float userRoughness;
	float time;
} applicationState;

#endif // APPLICATION_STATE_H_
