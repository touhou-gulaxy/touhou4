# SCRNN Upscale

Anime4K compute-shader upscaler used for project texture preparation. The
implementation is derived from `bloc97/Anime4K`; see `LICENSE-Anime4K`.

## Directory layout

```text
utils/SCRNN_upscale/
├── CMakeLists.txt
├── build.sh
├── run.sh
├── clean.sh
├── shaders/*.glsl
└── src/main.cpp
```

All CMake output, including the `scrnn-upscale` executable, is written to
`utils/SCRNN_upscale/build`.

## Build

```bash
utils/SCRNN_upscale/build.sh
```

The binary is `utils/SCRNN_upscale/build/scrnn-upscale`.

## Run

The input and output texture paths are positional arguments:

```bash
utils/SCRNN_upscale/run.sh input.dds output.dds --mode original --scale 4
```

`run.sh` builds the binary first when it does not exist.

PNG is also supported:

```bash
utils/SCRNN_upscale/run.sh input.png output.png --mode cnn-s --scale 2
```

The legacy `--input/-i` and `--output/-o` forms remain available.

Options:

- `--mode original|cnn-s`: select the Anime4K upscaler.
- `--scale 2|4`: run one or two 2x passes.
- `--refine-strength 0.5`: set the original-mode edge refinement strength.
- `--dump-stages`: write intermediate PNG debug textures next to the output.

## Texture formats

DDS input supports:

- Uncompressed RGB/RGBA with 16, 24, or 32 bits per pixel.
- Legacy `DXT1`, `DXT3`, and `DXT5`.
- DX10 `BC1`, `BC2`, `BC3`, `R8G8B8A8_UNORM`, and `B8G8R8A8_UNORM`.

DDS output is uncompressed 32-bit BGRA (`A8R8G8B8`) with one mip level. The
tool writes the top mip level of compressed input textures and preserves alpha.

## Clean

```bash
utils/SCRNN_upscale/clean.sh
```

This removes only `utils/SCRNN_upscale/build`.
