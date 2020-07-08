#ifndef GBUFFER_INC_
#define GBUFFER_INC_

layout(set = GBUFFER_SET, binding = 0) uniform sampler2D gbufferBaseColor;
layout(set = GBUFFER_SET, binding = 1) uniform sampler2D gbufferDepth;
layout(set = GBUFFER_SET, binding = 2) uniform sampler2D gbufferNormal;
layout(set = GBUFFER_SET, binding = 3) uniform sampler2D gbufferShading;

#endif // GBUFFER_INC_
