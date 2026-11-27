#version 450

// Derived from Anime4K-v3.2-Upscale-Original-x2-Luma.
// Original algorithm: Copyright (c) 2019-2021 bloc97, MIT License.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) writeonly uniform image2D outputImage;
layout(binding = 1) uniform sampler2D sourceTexture;
uniform ivec2 sourceSize;

void main()
{
    const ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(pixel, sourceSize))) {
        return;
    }

    const vec2 uv = (vec2(pixel) + 0.5) / vec2(sourceSize);
    const vec4 color = texture(sourceTexture, uv);
    const float luma = dot(vec3(0.299, 0.587, 0.114), color.rgb);
    imageStore(outputImage, pixel, vec4(luma, 0.0, 0.0, 1.0));
}
