/*
    Created by Yinghao He on 2025-06-12
*/
#pragma once

#include "Util.hpp"

// Focus on phase functions
class PhaseFunction {
public:
    virtual ~PhaseFunction() = default;
    
    virtual Vector3f Sample(const Vector3f& wi, const Vector2f& sample, Vector3f& wo, float& pdf) const = 0;
    virtual float Evaluate(const Vector3f& wi, const Vector3f& wo) const = 0;
    virtual float Pdf(const Vector3f& wi, const Vector3f& wo) const = 0;
};

class IsotropicPhase : public PhaseFunction {
public:
    IsotropicPhase() = default;

    virtual Vector3f Sample(const Vector3f& wi, const Vector2f& sample, Vector3f& wo, float& pdf) const override;
    virtual float Evaluate(const Vector3f& wi, const Vector3f& wo) const override;
    virtual float Pdf(const Vector3f& wi, const Vector3f& wo) const override;
};

class HenyeyGreensteinPhase : public PhaseFunction {
public:
    HenyeyGreensteinPhase(float g) : g(g) {}

    virtual Vector3f Sample(const Vector3f& wi, const Vector2f& sample, Vector3f& wo, float& pdf) const override;
    virtual float Evaluate(const Vector3f& wi, const Vector3f& wo) const override;
    virtual float Pdf(const Vector3f &wi, const Vector3f &wo) const override;
    
private:
    float g;
};