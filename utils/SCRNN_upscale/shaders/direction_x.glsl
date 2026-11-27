#version 450

// Derived from Anime4K-v3.2-Upscale-Original-x2-Kernel-X.
// Original algorithm: Copyright (c) 2019-2021 bloc97, MIT License.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) writeonly uniform image2D outputImage;
layout(binding = 1) uniform sampler2D sourceTexture;
uniform ivec2 sourceSize;
uniform ivec2 outputSize;

void main()
{
    const ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(pixel, outputSize))) {
        return;
    }

    const vec2 uv = (vec2(pixel) + 0.5) / vec2(outputSize);
    if (texture(sourceTexture, uv).y < 0.1) {
        imageStore(outputImage, pixel, vec4(0.0));
        return;
    }

    const vec2 stepUv = 1.0 / vec2(sourceSize);
    const float left = texture(sourceTexture, uv - vec2(stepUv.x, 0.0)).x;
    const float center = texture(sourceTexture, uv).x;
    const float right = texture(sourceTexture, uv + vec2(stepUv.x, 0.0)).x;
    const float xGradient = right - left;
    const float yGradient = left + 2.0 * center + right;
    imageStore(outputImage, pixel, vec4(xGradient, yGradient, 0.0, 1.0));
}
