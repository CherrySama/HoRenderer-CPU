/*
    Created by Yinghao He on 2025-06-12
*/
#include "PhaseFunction.hpp"


Vector3f IsotropicPhase::Sample(const Vector3f &wi, const Vector2f &sample, Vector3f &wo, float &pdf) const {
    // Isotropy: uniform sampling on the spherical surface
    // ρ = 1/(4π)
    float z = sample.x * 2.0f - 1.0f;
    float phi = sample.y * 2.0f * PI;
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    wo = Vector3f(r * std::cos(phi), r * std::sin(phi), z);

    pdf = INV_4PI;            // 1/(4π)
    return Vector3f(INV_4PI); // Return the phase function value
}

float IsotropicPhase::Evaluate(const Vector3f &wi, const Vector3f &wo) const {
    return INV_4PI; // Isotropic phase function constant value
}

float IsotropicPhase::Pdf(const Vector3f &wi, const Vector3f &wo) const {
    return INV_4PI;
}

Vector3f HenyeyGreensteinPhase::Sample(const Vector3f &wi, const Vector2f &sample, Vector3f &wo, float &pdf) const {
    float cos_theta;
    if (std::abs(g) < Epsilon) {
        // When g≈0 degenerates into isotropy
        cos_theta = 1.0f - 2.0f * sample.x;
    } else {
        //  cos(θ) = (1 + g² - ((1-g²)/(1-g+2g*ξ))²) / (2g)
        float xi = sample.x;
        float sqr_term = (1.0f - g * g) / (1.0f - g + 2.0f * g * xi);
        cos_theta = (1.0f + g * g - sqr_term * sqr_term) / (2.0f * g);
    }
    cos_theta = glm::clamp(cos_theta, -1.0f, 1.0f);

    float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
    float phi = 2.0f * PI * sample.y;

    // Building a local coordinate system
    Vector3f local_direction = Vector3f(sin_theta * std::cos(phi),
                                        sin_theta * std::sin(phi),
                                        cos_theta);
    wo = ToWorld(local_direction, -wi); 

    float phase_value = Evaluate(wi, wo);
    pdf = phase_value;

    return Vector3f(phase_value);
}

float HenyeyGreensteinPhase::Evaluate(const Vector3f &wi, const Vector3f &wo) const {
    // ρ(cos θ) = (1-g²) / (4π(1+g²+2g cos θ)^(3/2))
    float cos_theta = glm::dot(-wi, wo); 
    float temp = 1.0f + g * g + 2.0f * g * cos_theta;
    if (temp > Epsilon) 
        return INV_4PI * (1.0f - g * g) / (temp * std::sqrt(temp));
    
    return INV_4PI;
}

float HenyeyGreensteinPhase::Pdf(const Vector3f &wi, const Vector3f &wo) const {
    return Evaluate(wi, wo);
}