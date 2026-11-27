#version 450

// Derived from Anime4K-v3.2-Upscale-Original-x2-Kernel-Y.
// Original algorithm: Copyright (c) 2019-2021 bloc97, MIT License.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) writeonly uniform image2D outputImage;
layout(binding = 1) uniform sampler2D detailTexture;
layout(binding = 2) uniform sampler2D directionTexture;
uniform ivec2 sourceSize;
uniform ivec2 outputSize;

void main()
{
    const ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(pixel, outputSize))) {
        return;
    }

    const vec2 uv = (vec2(pixel) + 0.5) / vec2(outputSize);
    if (texture(detailTexture, uv).y < 0.1) {
        imageStore(outputImage, pixel, vec4(0.0));
        return;
    }

    const vec2 stepUv = 1.0 / vec2(sourceSize);
    const vec2 top = texture(directionTexture, uv - vec2(0.0, stepUv.y)).xy;
    const vec2 center = texture(directionTexture, uv).xy;
    const vec2 bottom = texture(directionTexture, uv + vec2(0.0, stepUv.y)).xy;
    float xGradient = top.x + 2.0 * center.x + bottom.x;
    float yGradient = bottom.y - top.y;
    float norm = sqrt(xGradient * xGradient + yGradient * yGradient);
    if (norm <= 0.001) {
        xGradient = 0.0;
        yGradient = 0.0;
        norm = 1.0;
    }
    imageStore(outputImage, pixel, vec4(xGradient / norm, yGradient / norm, 0.0, 1.0));
}
