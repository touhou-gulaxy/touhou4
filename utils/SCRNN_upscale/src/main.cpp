#include <epoxy/egl.h>
#include <epoxy/gl.h>
#include <png.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Image {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

struct Options {
    fs::path input;
    fs::path output;
    fs::path shaderDir = SCRNN_SHADER_DIR;
    std::string mode = "original";
    int scale = 2;
    float refineStrength = 0.5f;
    bool dumpStages = false;
};

constexpr std::uint32_t fourCc(char a, char b, char c, char d)
{
    return static_cast<std::uint32_t>(static_cast<unsigned char>(a))
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 8)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 16)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(d)) << 24);
}

constexpr std::uint32_t ddsFourCcDxt1 = fourCc('D', 'X', 'T', '1');
constexpr std::uint32_t ddsFourCcDxt3 = fourCc('D', 'X', 'T', '3');
constexpr std::uint32_t ddsFourCcDxt5 = fourCc('D', 'X', 'T', '5');
constexpr std::uint32_t ddsFourCcDx10 = fourCc('D', 'X', '1', '0');

constexpr std::uint32_t ddsPixelFormatFourCc = 0x4;
constexpr std::uint32_t ddsPixelFormatRgb = 0x40;
constexpr std::uint32_t ddsPixelFormatAlphaPixels = 0x1;

constexpr std::uint32_t dxgiR8g8b8a8Unorm = 28;
constexpr std::uint32_t dxgiB8g8r8a8Unorm = 87;
constexpr std::uint32_t dxgiBc1Unorm = 71;
constexpr std::uint32_t dxgiBc1UnormSrgb = 72;
constexpr std::uint32_t dxgiBc2Unorm = 74;
constexpr std::uint32_t dxgiBc2UnormSrgb = 75;
constexpr std::uint32_t dxgiBc3Unorm = 77;
constexpr std::uint32_t dxgiBc3UnormSrgb = 78;

std::string readTextFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("cannot open " + path.string());
    }
    return std::string(
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> readBinaryFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("cannot open " + path.string());
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0) {
        throw std::runtime_error("cannot determine file size: " + path.string());
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    if (!data.empty()
        && !stream.read(
            reinterpret_cast<char*>(data.data()),
            static_cast<std::streamsize>(data.size()))) {
        throw std::runtime_error("read failed: " + path.string());
    }
    return data;
}

std::uint16_t readU16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0])
        | (static_cast<std::uint16_t>(data[1]) << 8);
}

std::uint32_t readU32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
}

void writeU32(std::uint8_t* data, std::uint32_t value)
{
    data[0] = static_cast<std::uint8_t>(value & 0xff);
    data[1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
    data[2] = static_cast<std::uint8_t>((value >> 16) & 0xff);
    data[3] = static_cast<std::uint8_t>((value >> 24) & 0xff);
}

std::uint8_t unpackChannel(std::uint32_t pixel, std::uint32_t mask, std::uint8_t fallback)
{
    if (mask == 0) {
        return fallback;
    }
    unsigned shift = 0;
    while (shift < 32 && ((mask >> shift) & 1u) == 0) {
        ++shift;
    }
    if (shift == 32) {
        return fallback;
    }
    const std::uint32_t maxValue = mask >> shift;
    const std::uint32_t value = (pixel & mask) >> shift;
    return static_cast<std::uint8_t>(
        (value * 255u + maxValue / 2u) / maxValue);
}

std::array<std::uint8_t, 3> decodeRgb565(std::uint16_t color)
{
    const std::uint32_t red = (color >> 11) & 0x1f;
    const std::uint32_t green = (color >> 5) & 0x3f;
    const std::uint32_t blue = color & 0x1f;
    return {
        static_cast<std::uint8_t>((red * 255u + 15u) / 31u),
        static_cast<std::uint8_t>((green * 255u + 31u) / 63u),
        static_cast<std::uint8_t>((blue * 255u + 15u) / 31u),
    };
}

std::array<std::array<std::uint8_t, 4>, 4> decodeColorBlock(
    const std::uint8_t* block,
    bool forceFourColor)
{
    const std::uint16_t color0 = readU16(block);
    const std::uint16_t color1 = readU16(block + 2);
    std::array<std::array<std::uint8_t, 4>, 4> colors{};
    const auto rgb0 = decodeRgb565(color0);
    const auto rgb1 = decodeRgb565(color1);
    for (std::size_t channel = 0; channel < 3; ++channel) {
        colors[0][channel] = rgb0[channel];
        colors[1][channel] = rgb1[channel];
    }
    colors[0][3] = 255;
    colors[1][3] = 255;

    if (forceFourColor || color0 > color1) {
        for (std::size_t channel = 0; channel < 3; ++channel) {
            colors[2][channel] = static_cast<std::uint8_t>(
                (2u * colors[0][channel] + colors[1][channel]) / 3u);
            colors[3][channel] = static_cast<std::uint8_t>(
                (colors[0][channel] + 2u * colors[1][channel]) / 3u);
        }
        colors[2][3] = 255;
        colors[3][3] = 255;
    } else {
        for (std::size_t channel = 0; channel < 3; ++channel) {
            colors[2][channel] = static_cast<std::uint8_t>(
                (colors[0][channel] + colors[1][channel]) / 2u);
        }
        colors[2][3] = 255;
        colors[3][3] = 0;
    }
    return colors;
}

void decodeDxtColorBlock(
    const std::uint8_t* block,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t blockX,
    std::uint32_t blockY,
    bool forceFourColor,
    std::vector<std::uint8_t>& rgba)
{
    const auto colors = decodeColorBlock(block, forceFourColor);
    const std::uint32_t indices = readU32(block + 4);
    for (std::uint32_t pixelY = 0; pixelY < 4; ++pixelY) {
        for (std::uint32_t pixelX = 0; pixelX < 4; ++pixelX) {
            const std::uint32_t targetX = blockX * 4 + pixelX;
            const std::uint32_t targetY = blockY * 4 + pixelY;
            if (targetX >= width || targetY >= height) {
                continue;
            }
            const std::uint32_t pixelIndex = pixelY * 4 + pixelX;
            const std::uint32_t colorIndex = (indices >> (pixelIndex * 2)) & 0x3;
            const std::size_t offset =
                (static_cast<std::size_t>(targetY) * width + targetX) * 4;
            std::copy(
                colors[colorIndex].begin(),
                colors[colorIndex].end(),
                rgba.begin() + static_cast<std::ptrdiff_t>(offset));
        }
    }
}

void decodeDxt3Block(
    const std::uint8_t* block,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t blockX,
    std::uint32_t blockY,
    std::vector<std::uint8_t>& rgba)
{
    decodeDxtColorBlock(block + 8, width, height, blockX, blockY, true, rgba);
    for (std::uint32_t pixelIndex = 0; pixelIndex < 16; ++pixelIndex) {
        const std::uint32_t targetX = blockX * 4 + pixelIndex % 4;
        const std::uint32_t targetY = blockY * 4 + pixelIndex / 4;
        if (targetX >= width || targetY >= height) {
            continue;
        }
        const std::uint8_t packed = block[pixelIndex / 2];
        const std::uint8_t alpha = static_cast<std::uint8_t>(
            pixelIndex % 2 == 0 ? (packed & 0xf) << 4 : packed & 0xf0);
        const std::size_t offset =
            (static_cast<std::size_t>(targetY) * width + targetX) * 4;
        rgba[offset + 3] = alpha;
    }
}

void decodeDxt5Block(
    const std::uint8_t* block,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t blockX,
    std::uint32_t blockY,
    std::vector<std::uint8_t>& rgba)
{
    decodeDxtColorBlock(block + 8, width, height, blockX, blockY, true, rgba);
    const std::uint8_t alpha0 = block[0];
    const std::uint8_t alpha1 = block[1];
    std::array<std::uint8_t, 8> alphas{};
    alphas[0] = alpha0;
    alphas[1] = alpha1;
    if (alpha0 > alpha1) {
        for (unsigned index = 1; index <= 6; ++index) {
            alphas[index + 1] = static_cast<std::uint8_t>(
                ((7u - index) * alpha0 + index * alpha1) / 7u);
        }
    } else {
        for (unsigned index = 1; index <= 4; ++index) {
            alphas[index + 1] = static_cast<std::uint8_t>(
                ((5u - index) * alpha0 + index * alpha1) / 5u);
        }
        alphas[6] = 0;
        alphas[7] = 255;
    }

    std::uint64_t alphaIndices = 0;
    for (unsigned byte = 0; byte < 6; ++byte) {
        alphaIndices |= static_cast<std::uint64_t>(block[2 + byte]) << (byte * 8);
    }
    for (std::uint32_t pixelIndex = 0; pixelIndex < 16; ++pixelIndex) {
        const std::uint32_t targetX = blockX * 4 + pixelIndex % 4;
        const std::uint32_t targetY = blockY * 4 + pixelIndex / 4;
        if (targetX >= width || targetY >= height) {
            continue;
        }
        const std::uint32_t alphaIndex =
            static_cast<std::uint32_t>((alphaIndices >> (pixelIndex * 3)) & 0x7);
        const std::size_t offset =
            (static_cast<std::size_t>(targetY) * width + targetX) * 4;
        rgba[offset + 3] = alphas[alphaIndex];
    }
}

Image decodeDds(
    const fs::path& path,
    const std::vector<std::uint8_t>& data,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t flags,
    std::uint32_t fourCC,
    std::uint32_t rgbBits,
    std::uint32_t redMask,
    std::uint32_t greenMask,
    std::uint32_t blueMask,
    std::uint32_t alphaMask,
    std::size_t payloadOffset)
{
    Image image;
    image.width = width;
    image.height = height;
    if (width == 0 || height == 0) {
        throw std::runtime_error("DDS texture is empty: " + path.string());
    }
    const std::uint64_t pixelCount =
        static_cast<std::uint64_t>(width) * height;
    if (pixelCount > std::numeric_limits<std::size_t>::max() / 4) {
        throw std::runtime_error("DDS dimensions are too large: " + path.string());
    }
    image.rgba.assign(static_cast<std::size_t>(pixelCount) * 4, 255);

    const bool isDxt1 = fourCC == ddsFourCcDxt1 || fourCC == dxgiBc1Unorm
        || fourCC == dxgiBc1UnormSrgb;
    const bool isDxt3 = fourCC == ddsFourCcDxt3 || fourCC == dxgiBc2Unorm
        || fourCC == dxgiBc2UnormSrgb;
    const bool isDxt5 = fourCC == ddsFourCcDxt5 || fourCC == dxgiBc3Unorm
        || fourCC == dxgiBc3UnormSrgb;
    if (isDxt1 || isDxt3 || isDxt5) {
        const std::uint32_t blockBytes = isDxt1 ? 8 : 16;
        const std::uint32_t blocksX = (width + 3) / 4;
        const std::uint32_t blocksY = (height + 3) / 4;
        const std::uint64_t required =
            static_cast<std::uint64_t>(blocksX) * blocksY * blockBytes;
        if (payloadOffset > data.size() || required > data.size() - payloadOffset) {
            throw std::runtime_error("truncated DDS texture: " + path.string());
        }
        for (std::uint32_t blockY = 0; blockY < blocksY; ++blockY) {
            for (std::uint32_t blockX = 0; blockX < blocksX; ++blockX) {
                const std::size_t blockIndex =
                    static_cast<std::size_t>(blockY) * blocksX + blockX;
                const std::uint8_t* block =
                    data.data() + payloadOffset + blockIndex * blockBytes;
                if (isDxt1) {
                    decodeDxtColorBlock(
                        block,
                        width,
                        height,
                        blockX,
                        blockY,
                        false,
                        image.rgba);
                } else if (isDxt3) {
                    decodeDxt3Block(block, width, height, blockX, blockY, image.rgba);
                } else {
                    decodeDxt5Block(block, width, height, blockX, blockY, image.rgba);
                }
            }
        }
        return image;
    }

    const bool isUncompressed = (flags & ddsPixelFormatRgb) != 0;
    if (!isUncompressed || (rgbBits != 16 && rgbBits != 24 && rgbBits != 32)) {
        throw std::runtime_error(
            "unsupported DDS pixel format in " + path.string());
    }
    const std::uint32_t bytesPerPixel = (rgbBits + 7) / 8;
    const std::uint64_t rowBytes64 =
        static_cast<std::uint64_t>(width) * bytesPerPixel;
    if (rowBytes64 > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("DDS row is too large: " + path.string());
    }
    const std::size_t rowBytes = static_cast<std::size_t>(rowBytes64);
    const std::uint32_t declaredPitch = readU32(data.data() + 4 + 16);
    const std::size_t pitch =
        declaredPitch >= rowBytes ? declaredPitch : rowBytes;
    if (payloadOffset > data.size()) {
        throw std::runtime_error("truncated DDS texture: " + path.string());
    }
    const std::uint64_t required =
        static_cast<std::uint64_t>(pitch) * (height - 1) + rowBytes;
    if (required > data.size() - payloadOffset) {
        throw std::runtime_error("truncated DDS texture: " + path.string());
    }

    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* row =
            data.data() + payloadOffset + static_cast<std::size_t>(y) * pitch;
        for (std::uint32_t x = 0; x < width; ++x) {
            std::uint32_t pixel = 0;
            for (std::uint32_t byte = 0; byte < bytesPerPixel; ++byte) {
                pixel |= static_cast<std::uint32_t>(row[x * bytesPerPixel + byte])
                    << (byte * 8);
            }
            const std::size_t offset =
                (static_cast<std::size_t>(y) * width + x) * 4;
            image.rgba[offset + 0] = unpackChannel(pixel, redMask, 0);
            image.rgba[offset + 1] = unpackChannel(pixel, greenMask, 0);
            image.rgba[offset + 2] = unpackChannel(pixel, blueMask, 0);
            image.rgba[offset + 3] = unpackChannel(pixel, alphaMask, 255);
        }
    }
    return image;
}

Image loadDds(const fs::path& path)
{
    const std::vector<std::uint8_t> data = readBinaryFile(path);
    constexpr std::size_t headerSize = 128;
    if (data.size() < headerSize
        || std::memcmp(data.data(), "DDS ", 4) != 0
        || readU32(data.data() + 4) != 124) {
        throw std::runtime_error("invalid DDS header: " + path.string());
    }
    const std::uint8_t* header = data.data() + 4;
    const std::uint32_t height = readU32(header + 8);
    const std::uint32_t width = readU32(header + 12);
    std::uint32_t pixelFlags = readU32(header + 76);
    std::uint32_t fourCC = readU32(header + 80);
    std::uint32_t rgbBits = readU32(header + 84);
    std::uint32_t redMask = readU32(header + 88);
    std::uint32_t greenMask = readU32(header + 92);
    std::uint32_t blueMask = readU32(header + 96);
    std::uint32_t alphaMask = readU32(header + 100);
    std::size_t payloadOffset = headerSize;

    if ((pixelFlags & ddsPixelFormatFourCc) != 0 && fourCC == ddsFourCcDx10) {
        if (data.size() < 148) {
            throw std::runtime_error("truncated DDS DX10 header: " + path.string());
        }
        const std::uint32_t dxgiFormat = readU32(data.data() + 128);
        payloadOffset = 148;
        if (dxgiFormat == dxgiB8g8r8a8Unorm) {
            fourCC = 0;
            pixelFlags = ddsPixelFormatRgb | ddsPixelFormatAlphaPixels;
            rgbBits = 32;
            redMask = 0x00ff0000;
            greenMask = 0x0000ff00;
            blueMask = 0x000000ff;
            alphaMask = 0xff000000;
        } else if (dxgiFormat == dxgiR8g8b8a8Unorm) {
            fourCC = 0;
            pixelFlags = ddsPixelFormatRgb | ddsPixelFormatAlphaPixels;
            rgbBits = 32;
            redMask = 0x000000ff;
            greenMask = 0x0000ff00;
            blueMask = 0x00ff0000;
            alphaMask = 0xff000000;
        } else {
            fourCC = dxgiFormat;
        }
    }

    return decodeDds(
        path,
        data,
        width,
        height,
        pixelFlags,
        fourCC,
        rgbBits,
        redMask,
        greenMask,
        blueMask,
        alphaMask,
        payloadOffset);
}

Image loadPng(const fs::path& path)
{
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&image, path.c_str())) {
        throw std::runtime_error("PNG read failed: " + path.string());
    }
    image.format = PNG_FORMAT_RGBA;
    Image result;
    result.width = image.width;
    result.height = image.height;
    result.rgba.resize(PNG_IMAGE_SIZE(image));
    if (!png_image_finish_read(&image, nullptr, result.rgba.data(), 0, nullptr)) {
        png_image_free(&image);
        throw std::runtime_error("PNG decode failed: " + path.string());
    }
    png_image_free(&image);
    return result;
}

bool hasPrefix(
    const std::vector<std::uint8_t>& data,
    const std::array<std::uint8_t, 8>& prefix)
{
    return data.size() >= prefix.size()
        && std::equal(prefix.begin(), prefix.end(), data.begin());
}

Image loadTexture(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("cannot open " + path.string());
    }
    std::array<std::uint8_t, 8> prefix{};
    stream.read(reinterpret_cast<char*>(prefix.data()), prefix.size());
    const std::streamsize bytesRead = stream.gcount();
    stream.close();
    const std::vector<std::uint8_t> header(prefix.begin(), prefix.begin() + bytesRead);
    const std::array<std::uint8_t, 8> pngMagic = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
    };
    if (hasPrefix(header, pngMagic)) {
        return loadPng(path);
    }
    if (header.size() >= 4 && std::memcmp(header.data(), "DDS ", 4) == 0) {
        return loadDds(path);
    }
    throw std::runtime_error(
        "unsupported input texture format; expected PNG or DDS: "
        + path.string());
}

void writePng(
    const fs::path& path,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<std::uint8_t>& rgba)
{
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = width;
    image.height = height;
    image.format = PNG_FORMAT_RGBA;
    if (!png_image_write_to_file(&image, path.c_str(), 0, rgba.data(), 0, nullptr)) {
        throw std::runtime_error("PNG write failed: " + path.string());
    }
}

void writeDds(
    const fs::path& path,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<std::uint8_t>& rgba)
{
    const std::uint64_t rowBytes64 = static_cast<std::uint64_t>(width) * 4;
    const std::uint64_t pixelBytes64 =
        rowBytes64 * static_cast<std::uint64_t>(height);
    if (rowBytes64 > std::numeric_limits<std::uint32_t>::max()
        || pixelBytes64 > std::numeric_limits<std::size_t>::max()
        || rgba.size() < static_cast<std::size_t>(pixelBytes64)) {
        throw std::runtime_error("invalid DDS output dimensions");
    }

    std::vector<std::uint8_t> output;
    output.reserve(128 + static_cast<std::size_t>(pixelBytes64));
    output.insert(output.end(), {'D', 'D', 'S', ' '});
    std::array<std::uint8_t, 124> header{};
    writeU32(header.data() + 0, 124);
    writeU32(header.data() + 4, 0x0000100f);
    writeU32(header.data() + 8, height);
    writeU32(header.data() + 12, width);
    writeU32(header.data() + 16, static_cast<std::uint32_t>(rowBytes64));
    writeU32(header.data() + 24, 1);
    writeU32(header.data() + 72, 32);
    writeU32(
        header.data() + 76,
        ddsPixelFormatRgb | ddsPixelFormatAlphaPixels);
    writeU32(header.data() + 84, 32);
    writeU32(header.data() + 88, 0x00ff0000);
    writeU32(header.data() + 92, 0x0000ff00);
    writeU32(header.data() + 96, 0x000000ff);
    writeU32(header.data() + 100, 0xff000000);
    writeU32(header.data() + 104, 0x00001000);
    output.insert(output.end(), header.begin(), header.end());

    for (std::size_t index = 0; index < rgba.size(); index += 4) {
        output.push_back(rgba[index + 2]);
        output.push_back(rgba[index + 1]);
        output.push_back(rgba[index + 0]);
        output.push_back(rgba[index + 3]);
    }
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("cannot open output texture: " + path.string());
    }
    stream.write(
        reinterpret_cast<const char*>(output.data()),
        static_cast<std::streamsize>(output.size()));
    if (!stream) {
        throw std::runtime_error("DDS write failed: " + path.string());
    }
}

void writeTexture(
    const fs::path& path,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<std::uint8_t>& rgba)
{
    std::string extension = path.extension().string();
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    if (extension == ".png") {
        writePng(path, width, height, rgba);
        return;
    }
    if (extension == ".dds") {
        writeDds(path, width, height, rgba);
        return;
    }
    throw std::runtime_error(
        "unsupported output texture format; use .png or .dds: "
        + path.string());
}

GLuint compileShader(GLenum type, const std::string& source, const fs::path& path)
{
    GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("shader compile failed: " + path.string() + "\n" + log);
    }
    return shader;
}

GLuint createProgram(const fs::path& computePath)
{
    GLuint shader = compileShader(GL_COMPUTE_SHADER, readTextFile(computePath), computePath);
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error("program link failed: " + computePath.string() + "\n" + log);
    }
    return program;
}

GLuint makeTexture(GLsizei width, GLsizei height, GLenum internalFormat)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(internalFormat),
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
    return texture;
}

GLuint uploadImage(const Image& image)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        static_cast<GLsizei>(image.width),
        static_cast<GLsizei>(image.height),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.rgba.data());
    return texture;
}

void bindTexture(GLuint texture, int unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void bindImage(GLuint texture, int unit)
{
    glBindImageTexture(unit, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
}

void setUniform2i(GLuint program, const char* name, int x, int y)
{
    const GLint location = glGetUniformLocation(program, name);
    if (location >= 0) {
        glUniform2i(location, x, y);
    }
}

void setUniform1f(GLuint program, const char* name, float value)
{
    const GLint location = glGetUniformLocation(program, name);
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void dispatchCompute(int width, int height)
{
    glDispatchCompute(
        static_cast<GLuint>((width + 7) / 8),
        static_cast<GLuint>((height + 7) / 8),
        1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

std::vector<float> readFloatTexture(GLuint texture, int width, int height)
{
    std::vector<float> pixels(static_cast<std::size_t>(width) * height * 4);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        GL_FLOAT,
        pixels.data());
    return pixels;
}

void dumpGray(
    const fs::path& path,
    int width,
    int height,
    const std::vector<float>& values,
    int channel,
    float multiplier = 1.0f)
{
    std::vector<std::uint8_t> rgba(
        static_cast<std::size_t>(width) * height * 4,
        255);
    for (std::size_t i = 0; i < rgba.size() / 4; ++i) {
        const float value = std::clamp(values[i * 4 + channel] * multiplier, 0.0f, 1.0f);
        const auto byte = static_cast<std::uint8_t>(std::lround(value * 255.0f));
        rgba[i * 4 + 0] = byte;
        rgba[i * 4 + 1] = byte;
        rgba[i * 4 + 2] = byte;
    }
    writePng(path, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), rgba);
}

void dumpFeature(
    const fs::path& path,
    int width,
    int height,
    const std::vector<float>& values)
{
    std::vector<std::uint8_t> rgba(
        static_cast<std::size_t>(width) * height * 4,
        255);
    for (std::size_t i = 0; i < rgba.size() / 4; ++i) {
        for (int channel = 0; channel < 3; ++channel) {
            const float value = 0.5f + 0.5f * std::clamp(
                values[i * 4 + channel],
                -1.0f,
                1.0f);
            rgba[i * 4 + channel] = static_cast<std::uint8_t>(
                std::lround(value * 255.0f));
        }
    }
    writePng(path, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), rgba);
}

class EglContext {
public:
    EglContext()
    {
        const auto getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
            eglGetProcAddress("eglGetPlatformDisplayEXT"));
        if (getPlatformDisplay == nullptr) {
            throw std::runtime_error("eglGetPlatformDisplayEXT is not available");
        }
        display_ = getPlatformDisplay(
            EGL_PLATFORM_SURFACELESS_MESA,
            EGL_DEFAULT_DISPLAY,
            nullptr);
        if (display_ == EGL_NO_DISPLAY) {
            throw std::runtime_error("failed to get EGL surfaceless display");
        }
        if (!eglInitialize(display_, nullptr, nullptr)) {
            throw std::runtime_error("eglInitialize failed");
        }
        if (!eglBindAPI(EGL_OPENGL_API)) {
            throw std::runtime_error("eglBindAPI(OpenGL) failed");
        }

        const EGLint configAttributes[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_NONE,
        };
        EGLint configCount = 0;
        if (!eglChooseConfig(
                display_,
                configAttributes,
                &config_,
                1,
                &configCount) ||
            configCount == 0) {
            throw std::runtime_error("no suitable EGL config");
        }

        const EGLint contextAttributes[] = {
            EGL_CONTEXT_MAJOR_VERSION, 4,
            EGL_CONTEXT_MINOR_VERSION, 5,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
            EGL_NONE,
        };
        context_ = eglCreateContext(
            display_,
            config_,
            EGL_NO_CONTEXT,
            contextAttributes);
        if (context_ == EGL_NO_CONTEXT) {
            throw std::runtime_error("failed to create OpenGL 4.5 context");
        }
        if (!eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
            throw std::runtime_error("eglMakeCurrent failed");
        }
    }

    ~EglContext()
    {
        if (display_ != EGL_NO_DISPLAY) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context_ != EGL_NO_CONTEXT) {
                eglDestroyContext(display_, context_);
            }
            eglTerminate(display_);
        }
    }

    EglContext(const EglContext&) = delete;
    EglContext& operator=(const EglContext&) = delete;

private:
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLContext context_ = EGL_NO_CONTEXT;
};

Image runAnime4KPass(
    const Image& input,
    GLuint inputTexture,
    const fs::path& shaderDir,
    float refineStrength,
    bool dumpStages,
    const fs::path& outputPath)
{
    const int width = static_cast<int>(input.width);
    const int height = static_cast<int>(input.height);
    const int outputWidth = width * 2;
    const int outputHeight = height * 2;

    GLuint lumaProgram = createProgram(shaderDir / "luma.glsl");
    GLuint gradientXProgram = createProgram(shaderDir / "gradient_x.glsl");
    GLuint gradientYProgram = createProgram(shaderDir / "gradient_y.glsl");
    GLuint directionXProgram = createProgram(shaderDir / "direction_x.glsl");
    GLuint directionYProgram = createProgram(shaderDir / "direction_y.glsl");
    GLuint applyProgram = createProgram(shaderDir / "apply.glsl");

    GLuint lumaTexture = makeTexture(width, height, GL_RGBA16F);
    GLuint gradientXTexture = makeTexture(outputWidth, outputHeight, GL_RGBA16F);
    GLuint gradientYTexture = makeTexture(outputWidth, outputHeight, GL_RGBA16F);
    GLuint directionXTexture = makeTexture(outputWidth, outputHeight, GL_RGBA16F);
    GLuint directionYTexture = makeTexture(outputWidth, outputHeight, GL_RGBA16F);
    GLuint outputTexture = makeTexture(outputWidth, outputHeight, GL_RGBA16F);

    glDisable(GL_BLEND);

    glUseProgram(lumaProgram);
    bindTexture(inputTexture, 1);
    bindImage(lumaTexture, 0);
    setUniform2i(lumaProgram, "sourceSize", width, height);
    dispatchCompute(width, height);

    glUseProgram(gradientXProgram);
    bindTexture(lumaTexture, 1);
    bindImage(gradientXTexture, 0);
    setUniform2i(gradientXProgram, "sourceSize", width, height);
    setUniform2i(gradientXProgram, "outputSize", outputWidth, outputHeight);
    dispatchCompute(outputWidth, outputHeight);

    glUseProgram(gradientYProgram);
    bindTexture(gradientXTexture, 1);
    bindImage(gradientYTexture, 0);
    setUniform2i(gradientYProgram, "sourceSize", width, height);
    setUniform2i(gradientYProgram, "outputSize", outputWidth, outputHeight);
    setUniform1f(gradientYProgram, "refineStrength", refineStrength);
    dispatchCompute(outputWidth, outputHeight);

    glUseProgram(directionXProgram);
    bindTexture(gradientYTexture, 1);
    bindImage(directionXTexture, 0);
    setUniform2i(directionXProgram, "sourceSize", width, height);
    setUniform2i(directionXProgram, "outputSize", outputWidth, outputHeight);
    dispatchCompute(outputWidth, outputHeight);

    glUseProgram(directionYProgram);
    bindTexture(gradientYTexture, 1);
    bindTexture(directionXTexture, 2);
    bindImage(directionYTexture, 0);
    setUniform2i(directionYProgram, "sourceSize", width, height);
    setUniform2i(directionYProgram, "outputSize", outputWidth, outputHeight);
    dispatchCompute(outputWidth, outputHeight);

    glUseProgram(applyProgram);
    bindTexture(inputTexture, 1);
    bindTexture(gradientYTexture, 2);
    bindTexture(directionYTexture, 3);
    bindImage(outputTexture, 0);
    setUniform2i(applyProgram, "sourceSize", width, height);
    setUniform2i(applyProgram, "outputSize", outputWidth, outputHeight);
    dispatchCompute(outputWidth, outputHeight);

    if (dumpStages) {
        const fs::path stem = outputPath.parent_path() / outputPath.stem();
        dumpGray(stem.string() + "_stage1_luma.png", width, height, readFloatTexture(lumaTexture, width, height), 0);
        dumpGray(stem.string() + "_stage3_detail.png", outputWidth, outputHeight, readFloatTexture(gradientYTexture, outputWidth, outputHeight), 1);
        dumpGray(
            stem.string() + "_stage5_direction_x.png",
            outputWidth,
            outputHeight,
            readFloatTexture(directionYTexture, outputWidth, outputHeight),
            0,
            2.0f);
        dumpGray(
            stem.string() + "_stage5_direction_y.png",
            outputWidth,
            outputHeight,
            readFloatTexture(directionYTexture, outputWidth, outputHeight),
            1,
            2.0f);
    }

    const std::vector<float> output = readFloatTexture(
        outputTexture,
        outputWidth,
        outputHeight);
    Image result;
    result.width = static_cast<std::uint32_t>(outputWidth);
    result.height = static_cast<std::uint32_t>(outputHeight);
    result.rgba.resize(output.size());
    for (std::size_t i = 0; i < output.size(); ++i) {
        result.rgba[i] = static_cast<std::uint8_t>(
            std::lround(std::clamp(output[i], 0.0f, 1.0f) * 255.0f));
    }

    glDeleteTextures(1, &outputTexture);
    glDeleteTextures(1, &directionYTexture);
    glDeleteTextures(1, &directionXTexture);
    glDeleteTextures(1, &gradientYTexture);
    glDeleteTextures(1, &gradientXTexture);
    glDeleteTextures(1, &lumaTexture);
    glDeleteProgram(applyProgram);
    glDeleteProgram(directionYProgram);
    glDeleteProgram(directionXProgram);
    glDeleteProgram(gradientYProgram);
    glDeleteProgram(gradientXProgram);
    glDeleteProgram(lumaProgram);
    return result;
}

Image runCnnPass(
    const Image& input,
    GLuint inputTexture,
    const fs::path& shaderDir,
    bool dumpStages,
    const fs::path& outputPath)
{
    const int width = static_cast<int>(input.width);
    const int height = static_cast<int>(input.height);
    const int outputWidth = width * 2;
    const int outputHeight = height * 2;

    GLuint conv0Program = createProgram(shaderDir / "cnn_conv0.glsl");
    GLuint conv1Program = createProgram(shaderDir / "cnn_conv1.glsl");
    GLuint conv2Program = createProgram(shaderDir / "cnn_conv2.glsl");
    GLuint conv3Program = createProgram(shaderDir / "cnn_conv3.glsl");
    GLuint depthProgram = createProgram(shaderDir / "cnn_depth_to_space.glsl");

    GLuint conv0Texture = makeTexture(width, height, GL_RGBA16F);
    GLuint conv1Texture = makeTexture(width, height, GL_RGBA16F);
    GLuint conv2Texture = makeTexture(width, height, GL_RGBA16F);
    GLuint conv3Texture = makeTexture(width, height, GL_RGBA16F);
    GLuint outputTexture = makeTexture(outputWidth, outputHeight, GL_RGBA16F);

    glDisable(GL_BLEND);

    glUseProgram(conv0Program);
    bindTexture(inputTexture, 1);
    bindImage(conv0Texture, 0);
    setUniform2i(conv0Program, "sourceSize", width, height);
    setUniform2i(conv0Program, "outputSize", width, height);
    dispatchCompute(width, height);

    glUseProgram(conv1Program);
    bindTexture(conv0Texture, 1);
    bindImage(conv1Texture, 0);
    setUniform2i(conv1Program, "sourceSize", width, height);
    setUniform2i(conv1Program, "outputSize", width, height);
    dispatchCompute(width, height);

    glUseProgram(conv2Program);
    bindTexture(conv1Texture, 1);
    bindImage(conv2Texture, 0);
    setUniform2i(conv2Program, "sourceSize", width, height);
    setUniform2i(conv2Program, "outputSize", width, height);
    dispatchCompute(width, height);

    glUseProgram(conv3Program);
    bindTexture(conv2Texture, 1);
    bindImage(conv3Texture, 0);
    setUniform2i(conv3Program, "sourceSize", width, height);
    setUniform2i(conv3Program, "outputSize", width, height);
    dispatchCompute(width, height);

    glUseProgram(depthProgram);
    bindTexture(inputTexture, 1);
    bindTexture(conv3Texture, 2);
    bindImage(outputTexture, 0);
    setUniform2i(depthProgram, "sourceSize", width, height);
    setUniform2i(depthProgram, "outputSize", outputWidth, outputHeight);
    dispatchCompute(outputWidth, outputHeight);

    if (dumpStages) {
        const fs::path stem = outputPath.parent_path() / outputPath.stem();
        dumpFeature(
            stem.string() + "_cnn_conv0.png",
            width,
            height,
            readFloatTexture(conv0Texture, width, height));
        dumpFeature(
            stem.string() + "_cnn_conv1.png",
            width,
            height,
            readFloatTexture(conv1Texture, width, height));
        dumpFeature(
            stem.string() + "_cnn_conv2.png",
            width,
            height,
            readFloatTexture(conv2Texture, width, height));
        dumpFeature(
            stem.string() + "_cnn_conv3.png",
            width,
            height,
            readFloatTexture(conv3Texture, width, height));
    }

    const std::vector<float> output = readFloatTexture(
        outputTexture,
        outputWidth,
        outputHeight);
    Image result;
    result.width = static_cast<std::uint32_t>(outputWidth);
    result.height = static_cast<std::uint32_t>(outputHeight);
    result.rgba.resize(output.size());
    for (std::size_t i = 0; i < output.size(); ++i) {
        result.rgba[i] = static_cast<std::uint8_t>(
            std::lround(std::clamp(output[i], 0.0f, 1.0f) * 255.0f));
    }

    glDeleteTextures(1, &outputTexture);
    glDeleteTextures(1, &conv3Texture);
    glDeleteTextures(1, &conv2Texture);
    glDeleteTextures(1, &conv1Texture);
    glDeleteTextures(1, &conv0Texture);
    glDeleteProgram(depthProgram);
    glDeleteProgram(conv3Program);
    glDeleteProgram(conv2Program);
    glDeleteProgram(conv1Program);
    glDeleteProgram(conv0Program);
    return result;
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        const auto requireValue = [&](const char* name) -> std::string {
            if (index + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + name);
            }
            return argv[++index];
        };
        if (argument == "--input" || argument == "-i") {
            options.input = requireValue("--input");
        } else if (argument == "--output" || argument == "-o") {
            options.output = requireValue("--output");
        } else if (argument == "--shader-dir") {
            options.shaderDir = requireValue("--shader-dir");
        } else if (argument == "--mode") {
            options.mode = requireValue("--mode");
        } else if (argument == "--scale") {
            options.scale = std::stoi(requireValue("--scale"));
        } else if (argument == "--refine-strength") {
            options.refineStrength = std::stof(requireValue("--refine-strength"));
        } else if (argument == "--dump-stages") {
            options.dumpStages = true;
        } else if (argument == "--help" || argument == "-h") {
            std::cout
                << "Usage: scrnn-upscale <input-texture> <output-texture> "
                   "[--mode original|cnn-s] [--scale 2|4] "
                   "[--refine-strength 0.5] [--dump-stages]\n"
                   "Input and output textures may be PNG or common DDS formats.\n"
                   "The -i/--input and -o/--output forms are also supported.\n";
            std::exit(0);
        } else if (!argument.empty() && argument[0] != '-') {
            if (options.input.empty()) {
                options.input = argument;
            } else if (options.output.empty()) {
                options.output = argument;
            } else {
                throw std::runtime_error("too many positional arguments");
            }
        } else {
            throw std::runtime_error("unknown argument: " + argument);
        }
    }
    if (options.input.empty() || options.output.empty()) {
        throw std::runtime_error("both --input and --output are required");
    }
    if (options.scale != 2 && options.scale != 4) {
        throw std::runtime_error("this first implementation supports 2x or two chained 2x passes (4x)");
    }
    if (options.mode != "original" && options.mode != "cnn-s") {
        throw std::runtime_error("--mode must be original or cnn-s");
    }
    if (options.refineStrength < 0.0f || options.refineStrength > 4.0f) {
        throw std::runtime_error("--refine-strength should be in [0, 4]");
    }
    return options;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const Options options = parseOptions(argc, argv);
        const Image input = loadTexture(options.input);
        if (input.width == 0 || input.height == 0) {
            throw std::runtime_error("input image is empty");
        }
        if (options.output.has_parent_path()) {
            fs::create_directories(options.output.parent_path());
        }

        const auto start = std::chrono::steady_clock::now();
        EglContext context;
        std::cout
            << "OpenGL: " << glGetString(GL_VERSION) << "\n"
            << "Renderer: " << glGetString(GL_RENDERER) << "\n"
            << "Mode: " << options.mode << "\n";

        GLuint inputTexture = uploadImage(input);
        const auto runPass = [&](const Image& image, GLuint texture, bool dumpStages) {
            if (options.mode == "cnn-s") {
                return runCnnPass(
                    image,
                    texture,
                    options.shaderDir,
                    dumpStages,
                    options.output);
            }
            return runAnime4KPass(
                image,
                texture,
                options.shaderDir,
                options.refineStrength,
                dumpStages,
                options.output);
        };
        Image result = runPass(input, inputTexture, options.dumpStages);

        if (options.scale == 4) {
            GLuint intermediateTexture = uploadImage(result);
            result = runPass(result, intermediateTexture, false);
            glDeleteTextures(1, &intermediateTexture);
        }
        glDeleteTextures(1, &inputTexture);
        writeTexture(options.output, result.width, result.height, result.rgba);

        const auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start);
        std::cout
            << "Output: " << options.output << "\n"
            << "Size: " << result.width << "x" << result.height << "\n"
            << "Elapsed: " << elapsed.count() << " s\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
}
