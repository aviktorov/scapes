#ifndef COMMON_H_
#define COMMON_H_

const float PI =  3.141592653589798979f;
const float PI2 = 6.283185307179586477f;
const float iPI = 0.318309886183790672f;
const float EPSILON = 1e-6f;

#define saturate(X) clamp(X, 0.0f, 1.0f)
#define sqr(X) ((X)*(X))

vec3 pow(vec3 v, float power)
{
	vec3 ret;
	ret.x = pow(v.x, power);
	ret.y = pow(v.y, power);
	ret.z = pow(v.z, power);
	return ret;
}

int textureMips(in sampler2D tex)
{
	ivec2 size = textureSize(tex, 0);
	return 1 + int(floor(log2(max(size.x, size.y))));
}

vec2 hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), float(bitfieldReverse(i)) * 2.3283064365386963e-10);
}

vec3 importanceSamplingGGX(vec2 Xi, vec3 normal, float roughness)
{
	float alpha = saturate(roughness * roughness);
	float alpha2 = saturate(sqr(alpha));

	Xi.y = saturate(Xi.y - EPSILON);

	float phi = PI2 * Xi.x;
	float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (alpha2 - 1.0f) * Xi.y));
	float sinTheta = sqrt(1.0f - sqr(cosTheta));

	// from spherical coordinates to cartesian coordinates
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	vec3 up = abs(normal.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
	vec3 tangent = normalize(cross(up, normal));
	vec3 binormal = cross(normal, tangent);

	vec3 direction = tangent * H.x + binormal * H.y + normal * H.z;
	return normalize(direction);
}

#endif // COMMON_H_
