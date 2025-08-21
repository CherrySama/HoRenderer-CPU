/*
    Created by Yinghao He on 2025-08-21
*/
#include "Denoiser.hpp"

Denoiser::Denoiser() 
    : m_debug_mode(false), m_last_denoise_time(0.0f), 
      m_buffer_width(0), m_buffer_height(0) {
}

Denoiser::Denoiser(const SpatialDenoiserParams& params) 
    : m_params(params), m_debug_mode(false), m_last_denoise_time(0.0f),
      m_buffer_width(0), m_buffer_height(0) {
}

void Denoiser::Apply(float* pixels, int width, int height) {
    if (!pixels || width <= 0 || height <= 0) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();

    EnsureBufferSize(width, height);
    
    if (m_debug_mode) {
        std::cout << "Denoiser: Starting bilateral filtering " << width << "x" << height << std::endl;
    }

    std::memcpy(m_temp_buffer.get(), pixels, width * height * 4 * sizeof(float));
    
    const int radius = m_params.radius;
    const float sigma_space = m_params.sigma_space;
    const float sigma_color = m_params.sigma_color;

    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Vector3f center_color = GetPixel(m_temp_buffer.get(), x, y, width, height);
            Vector3f filtered_color(0.0f);
            float weight_sum = 0.0f;

            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;

                    nx = glm::clamp(nx, 0, width - 1);
                    ny = glm::clamp(ny, 0, height - 1);
                    
                    Vector3f neighbor_color = GetPixel(m_temp_buffer.get(), nx, ny, width, height);

                    float spatial_dist = std::sqrt(float(dx * dx + dy * dy));
                    float spatial_weight = GaussianWeight(spatial_dist, sigma_space);
                    float color_weight = ColorWeight(center_color, neighbor_color, sigma_color);
                    
                    float total_weight = spatial_weight * color_weight;
                    
                    filtered_color += neighbor_color * total_weight;
                    weight_sum += total_weight;
                }
            }
            
            if (weight_sum > Epsilon) {
                filtered_color /= weight_sum;
                SetPixel(pixels, x, y, width, filtered_color);
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    m_last_denoise_time = std::chrono::duration<float, std::milli>(end_time - start_time).count();
    
    if (m_debug_mode) {
        std::cout << "Denoiser: Bilateral filtering completed in " << m_last_denoise_time << "ms" << std::endl;
    }
}

Vector3f Denoiser::GetPixel(const float* pixels, int x, int y, int width, int height) const {
    x = glm::clamp(x, 0, width - 1);
    y = glm::clamp(y, 0, height - 1);
    
    int index = (y * width + x) * 4;
    return Vector3f(pixels[index], pixels[index + 1], pixels[index + 2]);
}

void Denoiser::SetPixel(float* pixels, int x, int y, int width, const Vector3f& color) const {
    int index = (y * width + x) * 4;
    pixels[index] = color.r;
    pixels[index + 1] = color.g;
    pixels[index + 2] = color.b;
}

float Denoiser::GaussianWeight(float distance, float sigma) const {
    return std::exp(-(distance * distance) / (2.0f * sigma * sigma));
}

float Denoiser::ColorWeight(const Vector3f& color1, const Vector3f& color2, float sigma) const {
    float color_diff = glm::length(color1 - color2);
    return std::exp(-(color_diff * color_diff) / (2.0f * sigma * sigma));
}

void Denoiser::EnsureBufferSize(int width, int height) {
    if (m_buffer_width != width || m_buffer_height != height) {
        m_temp_buffer = std::make_unique<float[]>(width * height * 4);
        m_buffer_width = width;
        m_buffer_height = height;
        
        if (m_debug_mode) {
            std::cout << "SpatialDenoiser: Allocated buffer " << width << "x" << height << std::endl;
        }
    }
}