#version 450

#include <shaders/common/Common.h>

layout(set = 0, binding = 0) uniform sampler2D texColor;
layout(set = 1, binding = 0) uniform sampler2D texColorOld;
layout(set = 2, binding = 0) uniform sampler2D texVelocity;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outTAA;

float calcAlpha(vec4 color, vec4 colorOld)
{
	return 1.0f / 30.0f;
}

vec2 getReprojectedUV(vec2 uv)
{
	vec2 motionVector = texture(texVelocity, uv).xy;
	return uv + motionVector;
}

void getNeighborsMinMax(in vec4 center, in vec2 uv, out vec4 colorMin, out vec4 colorMax)
{
	colorMin = center;
	colorMax = center;

	#define SAMPLE_NEIGHBOR(OFFSET) \
	{ \
		vec4 neighbor = textureOffset(texColor, uv, OFFSET); \
		colorMin = min(colorMin, neighbor); \
		colorMax = max(colorMax, neighbor); \
	} \

	SAMPLE_NEIGHBOR(ivec2(-1, 0));
	SAMPLE_NEIGHBOR(ivec2(1, 0));
	SAMPLE_NEIGHBOR(ivec2(0, -1));
	SAMPLE_NEIGHBOR(ivec2(0, 1));
	SAMPLE_NEIGHBOR(ivec2(-1, 1));
	SAMPLE_NEIGHBOR(ivec2(1, 1));
	SAMPLE_NEIGHBOR(ivec2(1, -1));
	SAMPLE_NEIGHBOR(ivec2(1, 1));
}

/*
 */
void main()
{
	vec2 uvCurrent = inUV;
	vec2 uvOld = getReprojectedUV(uvCurrent);

	vec4 color = texture(texColor, uvCurrent);

	if (uvOld.x < 0.0f || uvOld.x > 1.0f || uvOld.y < 0.0f || uvOld.y > 1.0f)
	{
		outTAA = color;
	}
	else
	{
		vec4 colorOld = texture(texColorOld, uvOld);

		vec4 colorMin;
		vec4 colorMax;
		getNeighborsMinMax(color, uvCurrent, colorMin, colorMax);

		colorOld = clamp(colorOld, colorMin, colorMax);

		float alpha = calcAlpha(color, colorOld);
		outTAA = mix(colorOld, color, alpha);
	}
}
