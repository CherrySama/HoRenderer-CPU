/*
    Created by Yinghao He on 2025-05-20
*/
#include "Sampler.hpp"
#include "SobolMatrices1024x52.hpp"

float Sampler::random_float() const
{
    thread_local static int thread_dimension = 0;
    thread_local static int last_sample = -1;
    thread_local static int last_pixel_x = -1;
    thread_local static int last_pixel_y = -1;
    
    if (last_sample != current_sample || last_pixel_x != pixel_x || last_pixel_y != pixel_y) {
        thread_dimension = 0;
        last_sample = current_sample;
        last_pixel_x = pixel_x;
        last_pixel_y = pixel_y;
    }
    
    uint64_t sample_index = static_cast<uint64_t>(current_sample);

    // 73856093 is the prime number for pixel hash function
    uint32_t pixel_hash = static_cast<uint32_t>(pixel_x * 73856093) ^ static_cast<uint32_t>(pixel_y * 19349663);
    int pixel_dimension_offset = (pixel_hash % 100) * 10; 
    int final_dimension = pixel_dimension_offset + thread_dimension++;
    final_dimension = final_dimension % SobolMatricesDim;

    uint32_t sobol_value = SobolSample(sample_index, final_dimension);
    
    float result = static_cast<float>(sobol_value) * 0x1p-32f;
    return std::min(result, 0.99999994f);      
}

Vector3f Sampler::random_unit_vector() const
{
    float z = random_float(-1.0f, 1.0f);
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2.0f * PI * random_float();
    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}

Vector3f Sampler::random_unit_2Dvector() const
{
    float theta = random_float(0.0f, 2.0f * PI);
    float r = std::sqrt(random_float());
    return Vector3f(r * std::cos(theta), r * std::sin(theta), 0.0f);
}

Vector3f Sampler::sample_square() const
{
    Vector2f sample = Vector2f(random_float(), random_float());
    sample = filter->SampleOffset(sample);
    return Vector3f(sample.x, sample.y, 0.0f);
}

void Sampler::SetCurrentSample(int sample_index)
{
    current_sample = sample_index;
}

void Sampler::SetPixel(int x, int y)
{
    pixel_x = x;
    pixel_y = y;
}