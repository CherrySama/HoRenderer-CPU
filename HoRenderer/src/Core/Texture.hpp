/*
    Created by Yinghao He on 2025-05-30
*/
#pragma once

#include "Util.hpp"

enum class TextureType {
    SOLIDCOLOR,
    IMAGE,
    HDR
};

struct TextureParams {
	TextureType type;
	Vector3f color;
	std::string filepath;
};

class Texture {
public:
    Texture(TextureType type) : tex_type(type) {}
    virtual ~Texture() = default;
    virtual Vector3f GetColor(float u, float v) const = 0;
    static std::shared_ptr<Texture> Create(const TextureParams &params);

private:
    TextureType tex_type;
};

class SolidTexture : public Texture {
public:
    SolidTexture(const Vector3f& c) : Texture(TextureType::SOLIDCOLOR), color(c) {}
    Vector3f GetColor(float u, float v) const override;
    
private:
    Vector3f color;
};

class ImageTexture : public Texture {
public:
    ImageTexture(const std::string &filepath);
    Vector3f GetColor(float u, float v) const override;

private:
    int width, height, channels;
    std::unique_ptr<unsigned char[]> image_data;
    bool load_success;

    bool LoadImage(const std::string &filepath);
};

class HDRTexture : public Texture {
public:
    HDRTexture(const std::string &filepath);
    Vector3f GetColor(float u, float v) const override;

private:
    int width, height, channels;
    std::unique_ptr<float[]> hdr_data;
    bool load_success;

    bool LoadHDR(const std::string &filepath);
};