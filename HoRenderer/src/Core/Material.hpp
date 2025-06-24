/*
	Created by Yinghao He on 2025-05-21
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "Texture.hpp"


class Material {
public:
    Material() = default;
    virtual ~Material() = default;

    virtual Vector3f Emit(const Ray& r_in, const Hit_Payload& rec, float u, float v) const;
    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const = 0;
    // virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Sampler& sampler) const = 0;
    // virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction) const = 0;
    // virtual float PDF(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction) const = 0;
    
protected:
    Vector3f GetSurfaceNormal(const Hit_Payload &rec) const;
    void SetNormal(std::shared_ptr<Texture> &normal);
    inline Vector3f NormalFromTangentToWorld(const Vector3f &surface_normal, const Vector3f &tangent_normal) const {
        Vector3f mapped_normal = glm::normalize(tangent_normal * 2.0f - 1.0f);

        Vector3f up_vector = std::abs(surface_normal.z) < 0.9f ? Vector3f(0.0f, 0.0f, 1.0f) : Vector3f(1.0f, 0.0f, 0.0f);

        Vector3f tangent_x = glm::normalize(glm::cross(up_vector, surface_normal));
        Vector3f tangent_y = glm::normalize(glm::cross(surface_normal, tangent_x));

        return glm::normalize(tangent_x * mapped_normal.x + tangent_y * mapped_normal.y + surface_normal * mapped_normal.z);
    }

protected:
    std::shared_ptr<Texture> normal_texture = nullptr;
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
    Metal(const Vector3f& a, float fu = 0.0f) : albedo_texture(std::make_shared<SolidTexture>(a)), fuzz(fu < 1.0f ? fu : 1.0f) {}
    Metal(std::shared_ptr<Texture> tex, float fu = 0.0f) : albedo_texture(tex), fuzz(fu < 1.0f ? fu : 1.0f) {}

    virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

private:
    std::shared_ptr<Texture> albedo_texture; 
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

class DiffuseLight : public Material {
public:
    DiffuseLight(std::shared_ptr<Texture> tex) : albedo_texture(tex) {}
    DiffuseLight(const Vector3f& emit) : albedo_texture(std::make_shared<SolidTexture>(emit)) {}
    virtual bool Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const override;
    virtual Vector3f Emit(const Ray &r_in, const Hit_Payload &rec, float u, float v) const override;
    
private:
    std::shared_ptr<Texture> albedo_texture;
};