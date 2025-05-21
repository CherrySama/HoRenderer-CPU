/*
	Created by Yinghao He on 2025-05-21
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "Hittable.hpp"
#include "Sampler.hpp"

class Material {
public:
    virtual ~Material() = default;

    // Core Scattering Function
    virtual bool scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const = 0;
};

// Lambertian
class Lambertian : public Material {
public:
    Lambertian(const Vector3f& a) : albedo(a) {}

    virtual bool scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    Vector3f albedo; // Diffuse Color
};