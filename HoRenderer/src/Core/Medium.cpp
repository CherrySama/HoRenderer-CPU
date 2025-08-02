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
float HomogeneousMedium::SampleDistance(const Ray &ray, float max_t, Sampler &sampler, int& sampled_channel, Vector3f& channel_pdfs) const {
    float max_sigma_t = std::max({sigma_t.x, sigma_t.y, sigma_t.z});
    if (max_sigma_t <= Epsilon) {
        sampled_channel = 0;
        channel_pdfs = Vector3f(0.0f);
        return Infinity; // no scattering
    }

    float channel_u = sampler.random_float();
    sampled_channel = std::min(2, static_cast<int>(channel_u * 3.0f));
    float sigma_t_channel = sigma_t[sampled_channel];
    if (sigma_t_channel <= Epsilon) {
        // If selected channel has no extinction, try other channels
        sampled_channel = -1;
        for (int i = 0; i < 3; i++) {
            if (sigma_t[i] > Epsilon) {
                sampled_channel = i;
                sigma_t_channel = sigma_t[i];
                break;
            }
        }
        if (sampled_channel == -1) {
            channel_pdfs = Vector3f(0.0f);
            return Infinity; // no extinction in any channel
        }
    }

    float u = sampler.random_float();
    float sampled_t = -std::log(1.0f - u) / sigma_t_channel;
    for (int i = 0; i < 3; i++) {
        if (sigma_t[i] > Epsilon) {
            channel_pdfs[i] = sigma_t[i] * std::exp(-sigma_t[i] * sampled_t);
        } else {
            channel_pdfs[i] = 0.0f;
        }
    }
    
    return sampled_t;
}