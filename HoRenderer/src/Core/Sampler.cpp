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
        use_second_sample = false;
        cached_sample = 0.0f;
        last_sample = current_sample;
        last_pixel_x = pixel_x;
        last_pixel_y = pixel_y;
    }

    uint64_t sample_index = static_cast<uint64_t>(current_sample);
    uint32_t pixel_hash = hash_pixel(pixel_x, pixel_y);
    int pixel_dimension_offset = (pixel_hash % 512) * 2;

    int dim1 = (pixel_dimension_offset + dimension_pair_index * 2) % SobolMatricesDim;
    int dim2 = (pixel_dimension_offset + dimension_pair_index * 2 + 1) % SobolMatricesDim;

    uint64_t effective_sample_index = sample_index + (pixel_hash == 0 ? 7 : pixel_hash % 8);
    uint32_t sobol_value1 = SobolSample(effective_sample_index, dim1);
    uint32_t sobol_value2 = SobolSample(effective_sample_index, dim2);

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

Vector3f Sampler::CosineSampleHemisphere(const Vector3f& normal) const
{
    Vector2f sample = get_2d_sample();
    float cos_theta = std::sqrt(1.0f - sample.x);  // cos_theta = sqrt(1-u1)
    float sin_theta = std::sqrt(sample.x);         // sin_theta = sqrt(u1)
    float phi = 2.0f * PI * sample.y;                 // phi = 2π * u2

    Vector3f local_direction(sin_theta * std::cos(phi),
                             sin_theta * std::sin(phi),
                             cos_theta);

    return ToWorld(local_direction, normal);
}

Vector3f Sampler::GGXNVDSample(const Vector3f &normal, const Vector3f &view, float alpha_u, float alpha_v) const
{
    Vector2f sample = get_2d_sample();

    Vector3f V = ToLocal(view, normal);

    // Visible Normal Distribution Sampling - Eric Heitz algorithm
    Vector3f Vh = glm::normalize(Vector3f(alpha_u * V.x, alpha_v * V.y, V.z));
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    Vector3f T1 = (lensq > 0.0f) ? Vector3f(-Vh.y, Vh.x, 0.0f) * (1.0f / std::sqrt(lensq)) : Vector3f(1.0f, 0.0f, 0.0f);
    Vector3f T2 = glm::cross(Vh, T1);
    
    float r = std::sqrt(sample.x);
    float phi = 2.0f * PI * sample.y;
    float t1 = r * std::cos(phi);
    float t2 = r * std::sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - s) * std::sqrt(1.0f - t1 * t1) + s * t2;
    
    Vector3f Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;

    Vector3f local_H = glm::normalize(Vector3f(alpha_u * Nh.x, alpha_v * Nh.y, std::max(0.0f, Nh.z)));

    Vector3f H = ToWorld(local_H, normal);

    return H;
}

Vector3f Sampler::GGXDistributionSample(const Vector3f& normal, float alpha_u, float alpha_v) const
{
    Vector2f sample = get_2d_sample();

    Vector3f up_vector = (std::abs(normal.z) < 0.9f) ? Vector3f(0.0f, 0.0f, 1.0f) : Vector3f(1.0f, 0.0f, 0.0f);
    Vector3f tangent = glm::normalize(glm::cross(up_vector, normal));
    Vector3f bitangent = glm::normalize(glm::cross(normal, tangent));

    float u1 = sample.x;
    float u2 = sample.y;
    
    if (std::abs(alpha_u - alpha_v) < 1e-6f) {
        float alpha2 = alpha_u * alpha_u;
        float cos_theta_2 = (1.0f - u1) / (u1 * (alpha2 - 1.0f) + 1.0f);
        float cos_theta = std::sqrt(std::max(0.0f, cos_theta_2));
        float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta_2));
        float phi = 2.0f * PI * u2;
        
        Vector3f local_H(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta);
        return glm::normalize(tangent * local_H.x + bitangent * local_H.y + normal * local_H.z);
    } else {
        float phi;
        if (u2 < 0.25f) {
            float u2_remapped = 4.0f * u2;
            phi = std::atan(alpha_v / alpha_u * std::tan(PI * 0.5f * u2_remapped));
        } else if (u2 < 0.5f) {
            float u2_remapped = 4.0f * (u2 - 0.25f);
            phi = PI - std::atan(alpha_v / alpha_u * std::tan(PI * 0.5f * u2_remapped));
        } else if (u2 < 0.75f) {
            float u2_remapped = 4.0f * (u2 - 0.5f);
            phi = PI + std::atan(alpha_v / alpha_u * std::tan(PI * 0.5f * u2_remapped));
        } else {
            float u2_remapped = 4.0f * (u2 - 0.75f);
            phi = 2.0f * PI - std::atan(alpha_v / alpha_u * std::tan(PI * 0.5f * u2_remapped));
        }
        
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);
        float alpha_eff_2 = alpha_u * alpha_u * cos_phi * cos_phi + alpha_v * alpha_v * sin_phi * sin_phi;
        
        float cos_theta_2 = (1.0f - u1) / (u1 * (alpha_eff_2 - 1.0f) + 1.0f);
        float cos_theta = std::sqrt(std::max(0.0f, cos_theta_2));
        float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta_2));
        
        Vector3f local_H(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
        return glm::normalize(tangent * local_H.x + bitangent * local_H.y + normal * local_H.z);
    }
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