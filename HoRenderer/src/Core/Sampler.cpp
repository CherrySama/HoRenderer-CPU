/*
    Created by Yinghao He on 2025-05-20
*/
#include "Sampler.hpp"
#include "SobolMatrices1024x52.hpp"

float Sampler::random_float() const
{
    if (use_second_sample) {
        use_second_sample = false;
        return cached_sample;
    } else {
        Vector2f sample_2d = get_2d_sample();
        cached_sample = sample_2d.y;
        use_second_sample = true;
        return sample_2d.x;
    }    
}

Vector2f Sampler::get_2d_sample() const
{
    if (last_sample != current_sample || last_pixel_x != pixel_x || last_pixel_y != pixel_y) {
        dimension_pair_index = 0;
        last_sample = current_sample;
        last_pixel_x = pixel_x;
        last_pixel_y = pixel_y;
    }

    uint64_t sample_index = static_cast<uint64_t>(current_sample);
    uint32_t pixel_hash = hash_pixel(pixel_x, pixel_y);
    int pixel_dimension_offset = (pixel_hash % 512) * 2;

    int dim1 = (pixel_dimension_offset + dimension_pair_index * 2) % SobolMatricesDim;
    int dim2 = (pixel_dimension_offset + dimension_pair_index * 2 + 1) % SobolMatricesDim;

    uint32_t sobol_value1 = SobolSample(sample_index, dim1);
    uint32_t sobol_value2 = SobolSample(sample_index, dim2);

    float u = std::min(static_cast<float>(sobol_value1) * 0x1p-32f, 0.99999994f);
    float v = std::min(static_cast<float>(sobol_value2) * 0x1p-32f, 0.99999994f);

    dimension_pair_index++;

    return Vector2f(u, v);
}

Vector3f Sampler::random_unit_vector() const
{
    Vector2f sample = get_2d_sample();  
    float z = sample.x * 2.0f - 1.0f;          
    float phi = sample.y * 2.0f * PI;           
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}

Vector3f Sampler::random_unit_2Dvector() const
{
    Vector2f sample = get_2d_sample();  
    float theta = sample.x * 2.0f * PI;
    float r = std::sqrt(sample.y);
    return Vector3f(r * std::cos(theta), r * std::sin(theta), 0.0f);
}

Vector3f Sampler::sample_square() const
{
    Vector2f sample = get_2d_sample();
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