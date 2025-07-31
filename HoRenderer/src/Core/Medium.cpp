/*
    Created by Yinghao He on 2025-06-06
*/
#include "Medium.hpp"
#include "Sampler.hpp"


Vector3f HomogeneousMedium::GetSigmaA(const Vector3f &p) const {
    return sigma_a;
}

Vector3f HomogeneousMedium::GetSigmaS(const Vector3f &p) const {
    return sigma_s;
}

Vector3f HomogeneousMedium::GetSigmaT(const Vector3f &p) const {
    return sigma_t;
}

std::shared_ptr<PhaseFunction> HomogeneousMedium::GetPhaseFunction() const {
    return phase_function;
}

bool HomogeneousMedium::IsHomogeneous() const {
    return true;
}

// T = exp(-σₜ * ||p₂-p₁||)
Vector3f HomogeneousMedium::Transmittance(const Vector3f &p1, const Vector3f &p2) const {
    float distance = glm::length(p2 - p1);
    // Spectroscopy to calculate transmittance
    return Vector3f(std::exp(-sigma_t.x * distance),
                    std::exp(-sigma_t.y * distance),
                    std::exp(-sigma_t.z * distance));
}

// t = -log(1-u) / σₜ
float HomogeneousMedium::SampleDistance(const Ray &ray, float max_t, Sampler &sampler) const {
    float avg_sigma_t = (sigma_t.x + sigma_t.y + sigma_t.z) / 3.0f;
    if (avg_sigma_t <= Epsilon) 
        return Infinity; // no scattering

    float u = sampler.random_float();
    float sampled_t = -std::log(1.0f - u) / avg_sigma_t;

    return sampled_t;
}