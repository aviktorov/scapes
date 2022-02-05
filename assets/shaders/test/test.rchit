#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 payload_color;
hitAttributeEXT vec3 attribs;

void main()
{
	const vec3 barycentric_coords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	payload_color = barycentric_coords;
}
