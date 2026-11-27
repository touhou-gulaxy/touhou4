#version 450

// Derived from Anime4K-v3.2-Upscale-Original-x2-Apply.
// Original algorithm: Copyright (c) 2019-2021 bloc97, MIT License.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) writeonly uniform image2D outputImage;
layout(binding = 1) uniform sampler2D sourceTexture;
layout(binding = 2) uniform sampler2D detailTexture;
layout(binding = 3) uniform sampler2D directionTexture;
uniform ivec2 sourceSize;
uniform ivec2 outputSize;

void main()
{
    const ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(pixel, outputSize))) {
        return;
    }

    const vec2 uv = (vec2(pixel) + 0.5) / vec2(outputSize);
    const vec2 stepUv = 1.0 / vec2(sourceSize);
    const vec4 center = texture(sourceTexture, uv);
    const float detail = texture(detailTexture, uv).y;
    if (detail < 0.1) {
        imageStore(outputImage, pixel, center);
        return;
    }

    const vec2 direction = texture(directionTexture, uv).xy;
    if (abs(direction.x + direction.y) <= 0.0001) {
        imageStore(outputImage, pixel, center);
        return;
    }

    const float xPosition = -sign(direction.x);
    const float yPosition = -sign(direction.y);
    const vec4 xValue = texture(sourceTexture, uv + vec2(stepUv.x * xPosition, 0.0));
    const vec4 yValue = texture(sourceTexture, uv + vec2(0.0, stepUv.y * yPosition));
    const float xyRatio = abs(direction.x) / (abs(direction.x) + abs(direction.y));
    const vec4 average = xyRatio * xValue + (1.0 - xyRatio) * yValue;
    imageStore(outputImage, pixel, mix(center, average, detail));
}
