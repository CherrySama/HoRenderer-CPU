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
    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const = 0;
};

// Lambertian
class Lambertian : public Material {
public:
    Lambertian(const Vector3f& a) : albedo(a) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    Vector3f albedo; // Diffuse Color
};

class DiffuseBRDF : public Material {
public:
    DiffuseBRDF(const Vector3f& a, float rough = 0.0f) : albedo(a), roughness(rough) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    Vector3f albedo; // Diffuse Color
    float roughness;     // Surface roughness (0 = perfect Lambertian, 1 = very rough)
};

// Not physically correct
class Metal : public Material {
public:
    Metal(const Vector3f& a, float fu = 0.0f) : albedo(a), fuzz(fu < 1.0f ? fu : 1.0f) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    Vector3f albedo; // Diffuse Color
    float fuzz;
};

class Dielectric : public Material {
public:
    Dielectric(float refract) : refractive_index(refract) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;
private:
    float refractive_index;

    // Use Schlick's approximation for reflectance.
    static float Reflectance(float cosine, float refraction_index);
};