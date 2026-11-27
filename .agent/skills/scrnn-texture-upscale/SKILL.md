---
name: scrnn-texture-upscale
description: Upscale PNG or DDS textures with the project SCRNN/Anime4K tool, including Stellaris event-room preparation, alpha preservation, build/run/clean handling, and output validation. Use when enlarging project artwork or replacing a supersampled event picture with a room texture.
---

# SCRNN Texture Upscale

Use the project-local tool at `utils/SCRNN_upscale`; do not depend on the
original external `Anime4KCompute` directory or hard-coded temporary build
paths.

## Command Shape

```bash
utils/SCRNN_upscale/run.sh <input-texture> <output-texture> \
  --mode original \
  --scale 4 \
  --refine-strength 0.5
```

`run.sh` builds only when `build/scrnn-upscale` is missing. After changing C++
or shader sources, run `build.sh` explicitly; after cleaner verification, run
`clean.sh` and then `run.sh` to test cold-start behavior.

```bash
utils/SCRNN_upscale/build.sh
utils/SCRNN_upscale/clean.sh
```

## Format Invariants

- Compute shaders live in `utils/SCRNN_upscale/shaders` and use the `.glsl`
  suffix. `src/main.cpp` names each shader explicitly, so rename both filename
  and reference together.
- Input may be PNG or DDS. Supported DDS input includes uncompressed 16/24/32-bit
  RGB(A), DXT1/3/5, and DX10 BC1/BC2/BC3 plus RGBA8/BGRA8.
- Only the top DDS mip is processed. DDS output is uncompressed 32-bit BGRA with
  one mip; do not claim that compressed input remains compressed.
- Alpha is preserved. `--mode original` runs Anime4K Original; `cnn-s` runs the
  CNN-S pass. `--scale 4` chains two 2x passes.
- `--dump-stages` writes PNG diagnostics beside the requested output.

## Stellaris Event Rooms

For a 238x85 source that must become a 952x340 room:

```bash
utils/SCRNN_upscale/run.sh source.png /tmp/room-upscaled.png \
  --mode original --scale 4
magick /tmp/room-upscaled.png -resize 1020x450! /tmp/room-1020.png
magick /tmp/room-1020.png -gravity center -crop 952x340+0+0 +repage \
  gfx/portraits/city_sets/spth_<key>_room.dds
```

Keep the center fixed when cropping. If the source is already larger, adjust
the scale pass rather than distorting unrelated art.

## Validation

Before reporting completion:

1. Run the executable with a real PNG or DDS input.
2. Check the output with `identify -format '%f %wx%h %[channels]\n'`.
3. Confirm expected dimensions, RGBA channels, and that the file starts with
   `DDS ` when the output is DDS.
4. Visually inspect a converted preview for transparent or DXT5 artwork.
5. Run `bash -n` on changed scripts and `git diff --check`.

If a shader fails to compile, verify the `.glsl` files still contain
`#version` and that `run.sh` did not reuse a stale binary after source edits.
