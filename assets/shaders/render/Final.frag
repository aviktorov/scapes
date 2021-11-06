#version 450

layout(set = 0, binding = 0) uniform sampler2D texLDRColor;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outLDRColor;

/*
 */
void main()
{
	vec4 color = texture(texLDRColor, inUV);
	color.rgb = pow(color.rgb, vec3(1.0f / 2.2f));

	outLDRColor = color;
}
