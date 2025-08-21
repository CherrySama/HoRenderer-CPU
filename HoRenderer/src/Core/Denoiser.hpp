/*
    Created by Yinghao He on 2025-08-21
*/
#pragma once

#include "Util.hpp"

struct SpatialDenoiserParams {
    float sigma_space = 2.0f;      // Spatial domain standard deviation
    float sigma_color = 0.15f;     // Color gamut standard deviation  
    int radius = 3;                // Filter radius
};

class Denoiser {
public:
    Denoiser();
    Denoiser(const SpatialDenoiserParams& params);
    ~Denoiser() = default;

    void Apply(float* pixels, int width, int height);

    void SetParams(const SpatialDenoiserParams& params) { m_params = params; }
    const SpatialDenoiserParams& GetParams() const { return m_params; }

    void EnableDebugMode(bool enable) { m_debug_mode = enable; }
    float GetLastDenoiseTime() const { return m_last_denoise_time; }

private:
    SpatialDenoiserParams m_params;
    bool m_debug_mode;
    float m_last_denoise_time;

    std::unique_ptr<float[]> m_temp_buffer;
    int m_buffer_width, m_buffer_height;

    Vector3f GetPixel(const float* pixels, int x, int y, int width, int height) const;
    void SetPixel(float* pixels, int x, int y, int width, const Vector3f& color) const;

    float GaussianWeight(float distance, float sigma) const;
    float ColorWeight(const Vector3f& color1, const Vector3f& color2, float sigma) const;

    void EnsureBufferSize(int width, int height);
};