/*
	Created by Yinghao He on 2025-05-21
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "Sampler.hpp"
#include "Texture.hpp"

enum class MaterialType {
    LAMBERTIAN,
    DIFFUSE_BRDF,
    METAL,
    DIELECTRIC
};

struct MaterialParams {
    MaterialType type;
    Vector3f albedo;
    std::shared_ptr<Texture> albedo_texture;
    float roughness;
    float fuzz;
    float refractive_index;
};

class Material {
public:
    Material() = default;
    virtual ~Material() = default;

    // Core Scattering Function
    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const = 0;
    static std::shared_ptr<Material> Create(const MaterialParams& params);
};

// Lambertian
class Lambertian : public Material {
public:
    Lambertian(const Vector3f& a) : albedo_texture(std::make_shared<SolidTexture>(a)) {}
    Lambertian(std::shared_ptr<Texture> tex) : albedo_texture(tex) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    std::shared_ptr<Texture> albedo_texture;
};

class DiffuseBRDF : public Material {
public:
    DiffuseBRDF(const Vector3f &a, float rough = 0.0f) : albedo_texture(std::make_shared<SolidTexture>(a)), roughness(rough) {}
    DiffuseBRDF(std::shared_ptr<Texture> tex, float rough = 0.0f) : albedo_texture(tex), roughness(rough) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    std::shared_ptr<Texture> albedo_texture;
    float roughness;
};

// Not physically correct
class Metal : public Material {
public:
    Metal(const Vector3f& a, float fu = 0.0f) : albedo(a), fuzz(fu < 1.0f ? fu : 1.0f) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    Vector3f albedo; 
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