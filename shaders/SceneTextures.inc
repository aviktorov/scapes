#ifndef SCENE_TEXTURES_H_
#define SCENE_TEXTURES_H_

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D aoSampler;
layout(set = 1, binding = 3) uniform sampler2D shadingSampler;
layout(set = 1, binding = 4) uniform sampler2D emissionSampler;

#endif // SCENE_TEXTURES_H_
