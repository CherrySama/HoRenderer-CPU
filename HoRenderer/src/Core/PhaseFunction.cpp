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
    
    float cos_theta;
    if (std::abs(g) < Epsilon) {
        // cos(θ) = 1 - 2ξ
        cos_theta = 1.0f - 2.0f * sampler.random_float();
    } else {
        // cos(θ) = (1 + g² - ((1-g²)/(1-g+2g*ξ))²) / (2g)
        float xi = sampler.random_float();
        float sqr_term = (1.0f - g * g) / (1.0f - g + 2.0f * g * xi);
        cos_theta = (1.0f + g * g - sqr_term * sqr_term) / (2.0f * g);
    }
    cos_theta = glm::clamp(cos_theta, -1.0f, 1.0f);

    float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
    float phi = 2.0f * PI * sampler.random_float();

    Vector3f local_direction = Vector3f(sin_theta * std::cos(phi),
                                        sin_theta * std::sin(phi),
                                        cos_theta);
    scatter_direction = ToWorld(local_direction, -V);

    // pdf
    float temp = 1.0f + g * g + 2.0f * g * cos_theta;
    if (temp > Epsilon) {
        pdf = INV_4PI * (1.0f - g * g) / (temp * std::sqrt(temp));
    } else {
        pdf = INV_4PI;
    }

    return albedo;
}

Vector3f HenyeyGreensteinPhase::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    Vector3f V = -glm::normalize(r_in.direction());  
    Vector3f L = glm::normalize(scatter_direction);  
    float cos_theta = glm::dot(L, -V);
    
    float temp = 1.0f + g * g + 2.0f * g * cos_theta;
    if (temp > Epsilon) {
        pdf = INV_4PI * (1.0f - g * g) / (temp * std::sqrt(temp));
    } else {
        pdf = INV_4PI;  
    }
    
    return albedo;
}