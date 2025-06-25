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

    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const = 0;
    // virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const = 0;
    virtual Vector3f Emit(const Ray& r_in, const Hit_Payload& rec, float u, float v) const;
    
protected:
    Vector3f GetSurfaceNormal(const Hit_Payload &rec) const;
    void SetNormal(std::shared_ptr<Texture> &normal);
    Vector3f NormalFromTangentToWorld(const Vector3f &surface_normal, const Vector3f &tangent_normal) const;

protected:
    std::shared_ptr<Texture> normal_texture = nullptr;
};

class Diffuse : public Material {
public:
    Diffuse(const Vector3f &a, float rough = 0.0f) : albedo_texture(std::make_shared<SolidTexture>(a)), roughness(rough) {}
    Diffuse(std::shared_ptr<Texture> tex, float rough = 0.0f) : albedo_texture(tex), roughness(rough) {}

    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const override;

private:
    std::shared_ptr<Texture> albedo_texture;
    float roughness;
};

class Conductor : public Material {
public:
    Conductor(const Vector3f &albedo, float roughness_u, float roughness_v, const Vector3f &eta, const Vector3f &k) :
        albedo_texture(std::make_shared<SolidTexture>(albedo)), roughness_u(roughness_u), roughness_v(roughness_v), eta(eta), k(k) {}

    Conductor(std::shared_ptr<Texture> albedo_tex, float roughness_u, float roughness_v, const Vector3f &eta, const Vector3f &k) :
        albedo_texture(albedo_tex), roughness_u(roughness_u), roughness_v(roughness_v), eta(eta), k(k) {}
        
    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const override;

private:
    std::shared_ptr<Texture> albedo_texture; 
    float roughness_u, roughness_v;
    Vector3f eta, k; 
};

// class Dielectric : public Material {
// public:
//     Dielectric(float refract) : refractive_index(refract) {}

//     virtual bool Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const override;

// private:
//     float refractive_index;
//     // Use Schlick's approximation for reflectance.
//     static float Reflectance(float cosine, float refraction_index);
// };

// class DiffuseLight : public Material {
// public:
//     DiffuseLight(std::shared_ptr<Texture> tex) : albedo_texture(tex) {}
//     DiffuseLight(const Vector3f& emit) : albedo_texture(std::make_shared<SolidTexture>(emit)) {}
//     virtual bool Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const override;
//     virtual Vector3f Emit(const Ray &r_in, const Hit_Payload &rec, float u, float v) const override;
    
// private:
//     std::shared_ptr<Texture> albedo_texture;
// };