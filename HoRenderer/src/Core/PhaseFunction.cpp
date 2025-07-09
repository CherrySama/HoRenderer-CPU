/*
    Created by Yinghao He on 2025-06-12
*/
#include "PhaseFunction.hpp"
#include "Sampler.hpp"
#include "Ray.hpp"
#include "Hittable.hpp"


Vector3f IsotropicPhase::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    scatter_direction = sampler.random_unit_vector();
    pdf = INV_4PI;
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    return albedo;
}

Vector3f IsotropicPhase::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const
{
    pdf = INV_4PI;
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    return albedo;
}

Vector3f HenyeyGreensteinPhase::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    Vector3f V = -glm::normalize(r_in.direction()); 
    
    int channel = std::min(static_cast<int>(sampler.random_float() * 3.0f), 2);
    float gc = g[channel];
    
    float cos_theta = 0.0f;

    if (std::abs(gc) < Epsilon) {
        cos_theta = 1.0f - 2.0f * sampler.random_float();
    } else {
        float xi = sampler.random_float();
        float sqr_term = (1.0f - gc * gc) / (1.0f - gc + 2.0f * gc * xi);
        cos_theta = (1.0f + gc * gc - sqr_term * sqr_term) / (2.0f * gc);
    }

    cos_theta = glm::clamp(cos_theta, -1.0f, 1.0f);

    float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
    float phi = 2.0f * PI * sampler.random_float();

    Vector3f local_direction = Vector3f(sin_theta * std::cos(phi),
                                        sin_theta * std::sin(phi),
                                        cos_theta);

    scatter_direction = ToWorld(local_direction, -V);

    Vector3f attenuation(0.0f);
    pdf = 0.0f;
    
    for (int dim = 0; dim < 3; ++dim) {
        float temp = 1.0f + g[dim] * g[dim] + 2.0f * g[dim] * cos_theta;
        if (temp > Epsilon) {
            float phase_value = INV_4PI * (1.0f - g[dim] * g[dim]) / (temp * std::sqrt(temp));
            attenuation[dim] = phase_value;
            pdf += phase_value;
        }
    }
    
    pdf *= (1.0f / 3.0f);  
    
    return albedo * attenuation;
}

Vector3f HenyeyGreensteinPhase::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    Vector3f V = -glm::normalize(r_in.direction());  
    Vector3f L = glm::normalize(scatter_direction);  
    float cos_theta = glm::dot(L, -V);
    
    Vector3f attenuation(0.0f);
    pdf = 0.0f;

    for (int dim = 0; dim < 3; ++dim) {
        float temp = 1.0f + g[dim] * g[dim] + 2.0f * g[dim] * cos_theta;
        if (temp > Epsilon) {
            float phase_value = INV_4PI * (1.0f - g[dim] * g[dim]) / (temp * std::sqrt(temp));
            attenuation[dim] = phase_value;
            pdf += phase_value;
        }
    }
    
    pdf *= (1.0f / 3.0f);  
    
    return albedo * attenuation;
}