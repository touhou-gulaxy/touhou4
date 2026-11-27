#version 450
// Ported from Anime4K-v3.2-Upscale-CNN-x2-(S)-Depth-to-Space.
// Original algorithm and weights: Copyright (c) 2019-2021 bloc97, MIT License.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba16f, binding = 0) writeonly uniform image2D outputImage;
layout(binding = 1) uniform sampler2D sourceTexture;
layout(binding = 2) uniform sampler2D featureTexture;
uniform ivec2 sourceSize;
uniform ivec2 outputSize;

void main()
{
    const ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(pixel, outputSize))) {
        return;
    }

    const vec2 uv = (vec2(pixel) + 0.5) / vec2(outputSize);
    const vec2 sourcePosition = uv * vec2(sourceSize);
    const vec2 fraction = fract(sourcePosition);
    const ivec2 channel = ivec2(fraction * 2.0);
    const vec2 featureUv = (vec2(0.5) - fraction) / vec2(sourceSize) + uv;
    const vec4 features = texture(featureTexture, featureUv);
    const float residual = features[channel.y * 2 + channel.x];
    const vec4 base = texture(sourceTexture, uv);

    // Anime4K operates on opaque video. For a still image, preserve its
    // original alpha instead of adding the RGB residual to the alpha channel.
    imageStore(outputImage, pixel, vec4(base.rgb + vec3(residual), base.a));
}
