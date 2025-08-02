/*
    Created by Yinghao He on 2025-06-06
*/
#pragma once

#include "Util.hpp"
#include "PhaseFunction.hpp"


class Medium {
public:
    virtual ~Medium() = default;
    
    virtual Vector3f GetSigmaA(const Vector3f& p) const = 0;
    virtual Vector3f GetSigmaS(const Vector3f& p) const = 0;
    virtual Vector3f GetSigmaT(const Vector3f& p) const = 0;
    virtual std::shared_ptr<PhaseFunction> GetPhaseFunction() const = 0;
    
    // volume rendering
    virtual bool IsHomogeneous() const = 0;
    // Transmission calculation
    virtual Vector3f Transmittance(const Vector3f& p1, const Vector3f& p2) const = 0;
    // Distance sampling 
    virtual float SampleDistance(const Ray& ray, float max_t, Sampler& sampler, int& sampled_channel, Vector3f& channel_pdfs) const = 0;
};

class HomogeneousMedium : public Medium {
public:
    HomogeneousMedium(const Vector3f &sigma_s, const Vector3f &sigma_a, std::shared_ptr<PhaseFunction> phase)
        : sigma_s(sigma_s), sigma_a(sigma_a), phase_function(phase) {
        sigma_t = sigma_s + sigma_a;
    }

    virtual Vector3f GetSigmaA(const Vector3f& p) const override;
    virtual Vector3f GetSigmaS(const Vector3f& p) const override;
    virtual Vector3f GetSigmaT(const Vector3f& p) const override;
    virtual std::shared_ptr<PhaseFunction> GetPhaseFunction() const override;
    
    virtual bool IsHomogeneous() const override;
    virtual Vector3f Transmittance(const Vector3f& p1, const Vector3f& p2) const override;
    virtual float SampleDistance(const Ray &ray, float max_t, Sampler &sampler, int& sampled_channel, Vector3f& channel_pdfs) const override;

private:
    Vector3f sigma_s; // Scattering coefficient
    Vector3f sigma_a; // Absorption coefficient
    Vector3f sigma_t; // Extinction coefficient -> sigma_s + sigma_a
    std::shared_ptr<PhaseFunction> phase_function;
};