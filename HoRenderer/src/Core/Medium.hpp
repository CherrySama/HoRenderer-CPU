/*
    Created by Yinghao He on 2025-06-06
*/
#pragma once

#include "Util.hpp"
#include "DensityField.hpp"
#include "PhaseFunction.hpp"


struct MediumSample {
    bool scattered = false;
    float t = Infinity;
    Vector3f position = Vector3f(0.0f);
    Vector3f weight = Vector3f(1.0f);
};

class Medium {
public:
    virtual ~Medium() = default;
    
    virtual Vector3f GetSigmaA(const Vector3f& p) const = 0;
    virtual Vector3f GetSigmaS(const Vector3f& p) const = 0;
    virtual Vector3f GetSigmaT(const Vector3f& p) const = 0;
    virtual std::shared_ptr<PhaseFunction> GetPhaseFunction() const = 0;
    
    // volume rendering
    virtual bool IsHomogeneous() const = 0;
    // Samples either a scattering event before max_t or passage through the segment.
    // weight already includes transmittance and the sampling-probability correction.
    virtual MediumSample Sample(const Ray& ray, float max_t, Sampler& sampler) const = 0;
    // Unbiased segment transmittance estimator. Homogeneous media evaluate it exactly.
    virtual Vector3f Transmittance(const Ray& ray, float max_t, Sampler& sampler) const = 0;
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
    virtual MediumSample Sample(const Ray& ray, float max_t, Sampler& sampler) const override;
    virtual Vector3f Transmittance(const Ray& ray, float max_t, Sampler& sampler) const override;

private:
    Vector3f sigma_s; // Scattering coefficient
    Vector3f sigma_a; // Absorption coefficient
    Vector3f sigma_t; // Extinction coefficient -> sigma_s + sigma_a
    std::shared_ptr<PhaseFunction> phase_function;
};

class HeterogeneousMedium : public Medium {
public:
    HeterogeneousMedium(std::shared_ptr<DensityField> density_field,
                        const Vector3f& sigma_s_base,
                        const Vector3f& sigma_a_base,
                        std::shared_ptr<PhaseFunction> phase);

    Vector3f GetSigmaA(const Vector3f& p) const override;
    Vector3f GetSigmaS(const Vector3f& p) const override;
    Vector3f GetSigmaT(const Vector3f& p) const override;
    std::shared_ptr<PhaseFunction> GetPhaseFunction() const override;

    bool IsHomogeneous() const override { return false; }
    MediumSample Sample(const Ray& ray, float max_t, Sampler& sampler) const override;
    Vector3f Transmittance(const Ray& ray, float max_t, Sampler& sampler) const override;

private:
    float LookupDensity(const Vector3f& p) const;
    bool IntersectDensityBounds(const Ray& ray, float max_t, Vector2f& interval) const;

private:
    std::shared_ptr<DensityField> density_field;
    Vector3f sigma_s_base;
    Vector3f sigma_a_base;
    Vector3f sigma_t_base;
    std::shared_ptr<PhaseFunction> phase_function;
    float majorant;
};
