/*
    Created by Yinghao He on 2025-05-30
*/
#include "Texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../Common/stb_image.h"

Vector3f SolidTexture::GetColor(float u, float v) const
{
    return color;
}

ImageTexture::ImageTexture(const std::string &filepath) : Texture(TextureType::IMAGE), width(0), height(0), channels(0), load_success(false)
{
    load_success = LoadImage(filepath);
    if (!load_success) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
    }
}

bool ImageTexture::LoadImage(const std::string &filepath)
{
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "STB failed to load image: " << filepath << std::endl;
        std::cerr << "STB error: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    if (channels < 3) {
        std::cerr << "Image must have at least 3 channels (RGB), found: " << channels << std::endl;
        stbi_image_free(data);
        return false;
    }
    
    int total_size = width * height * channels;
    image_data = std::make_unique<unsigned char[]>(total_size);
    std::memcpy(image_data.get(), data, total_size);

    stbi_image_free(data);
    
    std::cout << "Successfully loaded image: " << filepath 
              << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
    
    return true;
}

Vector3f ImageTexture::GetColor(float u, float v) const
{
    if (!load_success || !image_data) {
        return Vector3f(1.0f, 0.0f, 1.0f); 
    }

    u = u - std::floor(u);
    v = 1.0f - (v - std::floor(v)); 
    
    int i = static_cast<int>(u * width);
    int j = static_cast<int>(v * height);
    
    i = std::clamp(i, 0, width - 1);
    j = std::clamp(j, 0, height - 1);

    int pixel_index = j * width * channels + i * channels;
    
    float r = image_data[pixel_index] / 255.0f;
    float g = image_data[pixel_index + 1] / 255.0f;
    float b = image_data[pixel_index + 2] / 255.0f;

    Vector3f srgb_color(r, g, b);
    return SRGBToLinear(srgb_color);
}

HDRTexture::HDRTexture(const std::string &filepath) : Texture(TextureType::HDR), width(0), height(0), channels(0), load_success(false)
{
    load_success = LoadHDR(filepath);
    if (!load_success) {
        std::cerr << "Failed to load HDR image: " << filepath << std::endl;
    }
}

bool HDRTexture::LoadHDR(const std::string &filepath)
{
    float* data = stbi_loadf(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "STB failed to load HDR image: " << filepath << std::endl;
        std::cerr << "STB error: " << stbi_failure_reason() << std::endl;
        return false;
    }

    if (channels < 3) {
        std::cerr << "HDR image must have at least 3 channels (RGB), found: " << channels << std::endl;
        stbi_image_free(data);
        return false;
    }

    int total_size = width * height * channels;
    hdr_data = std::make_unique<float[]>(total_size);
    std::memcpy(hdr_data.get(), data, total_size * sizeof(float));

    stbi_image_free(data);
    
    std::cout << "Successfully loaded HDR image: " << filepath 
              << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
    
    return true;
}

Vector3f HDRTexture::GetColor(float u, float v) const
{
    if (!load_success || !hdr_data) {
        return Vector3f(2.0f, 0.0f, 2.0f); 
    }

    u = u - std::floor(u);
    v = 1.0f - (v - std::floor(v)); 

    int i = static_cast<int>(u * width);
    int j = static_cast<int>(v * height);

    i = std::clamp(i, 0, width - 1);
    j = std::clamp(j, 0, height - 1);

    int pixel_index = j * width * channels + i * channels;

    float r = hdr_data[pixel_index];
    float g = hdr_data[pixel_index + 1];
    float b = hdr_data[pixel_index + 2];
    
    return Vector3f(r, g, b);
}
