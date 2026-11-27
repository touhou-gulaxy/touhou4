#version 450

// Derived from Anime4K-v3.2-Upscale-Original-x2-Kernel-Y.
// Original algorithm: Copyright (c) 2019-2021 bloc97, MIT License.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) writeonly uniform image2D outputImage;
layout(binding = 1) uniform sampler2D sourceTexture;
uniform ivec2 sourceSize;
uniform ivec2 outputSize;
uniform float refineStrength;

float powerFunction(float x)
{
    const float x2 = x * x;
    const float x3 = x2 * x;
    const float x4 = x2 * x2;
    const float x5 = x2 * x3;
    return 11.68129591 * x5
         - 42.46906057 * x4
         + 60.28286266 * x3
         - 41.84451327 * x2
         + 14.05517353 * x
         - 1.081521930;
}

void main()
{
    const ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(pixel, outputSize))) {
        return;
    }

    const vec2 uv = (vec2(pixel) + 0.5) / vec2(outputSize);
    const vec2 stepUv = 1.0 / vec2(sourceSize);
    const float topX = texture(sourceTexture, uv - vec2(0.0, stepUv.y)).x;
    const float centerX = texture(sourceTexture, uv).x;
    const float bottomX = texture(sourceTexture, uv + vec2(0.0, stepUv.y)).x;
    const float topY = texture(sourceTexture, uv - vec2(0.0, stepUv.y)).y;
    const float bottomY = texture(sourceTexture, uv + vec2(0.0, stepUv.y)).y;

    const float xGradient = topX + 2.0 * centerX + bottomX;
    const float yGradient = bottomY - topY;
    const float sobelNorm = clamp(sqrt(xGradient * xGradient + yGradient * yGradient), 0.0, 1.0);
    const float detail = clamp(powerFunction(sobelNorm) * refineStrength, 0.0, 1.0);
    imageStore(outputImage, pixel, vec4(sobelNorm, detail, 0.0, 1.0));
}
