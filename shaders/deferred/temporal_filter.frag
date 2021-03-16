#version 450
#pragma shader_stage(fragment)

#define GBUFFER_SET 0
#include <deferred/gbuffer.inc>

layout(set = 1, binding = 0) uniform sampler2D textureColor;
layout(set = 2, binding = 0) uniform sampler2D textureColorOld;
layout(set = 3, binding = 0) uniform sampler2D textureSSRVelocity;

#include <common/brdf.inc>

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outTAA;

float calcAlpha(vec4 color, vec4 colorOld)
{
	return 1.0f / 30.0f;
}

vec2 getReprojectedUV(vec2 uv)
{
	vec2 motionVector = texture(textureSSRVelocity, uv).xy;
	return uv + motionVector;
}

void getNeighborsMinMax(in vec4 center, in vec2 uv, out vec4 colorMin, out vec4 colorMax)
{
	colorMin = center;
	colorMax = center;

	#define SAMPLE_NEIGHBOR(OFFSET) \
	{ \
		vec4 neighbor = textureOffset(textureColor, uv, OFFSET); \
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
	vec2 uvCurrent = fragTexCoord;
	vec2 uvOld = getReprojectedUV(uvCurrent);

	vec4 color = texture(textureColor, uvCurrent);

	if (uvOld.x < 0.0f || uvOld.x > 1.0f || uvOld.y < 0.0f || uvOld.y > 1.0f)
	{
		outTAA = color;
	}
	else
	{
		vec4 colorOld = texture(textureColorOld, uvOld);

		vec4 colorMin;
		vec4 colorMax;
		getNeighborsMinMax(color, uvCurrent, colorMin, colorMax);

		colorOld = clamp(colorOld, colorMin, colorMax);

		float alpha = calcAlpha(color, colorOld);
		outTAA = lerp(colorOld, color, alpha);
	}
}
