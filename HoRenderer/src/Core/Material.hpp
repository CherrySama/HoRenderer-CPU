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
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const = 0;
    virtual Vector3f Emit(const Ray& r_in, const Hit_Payload& rec, float u, float v) const;
    virtual bool IsDelta() const { return false; }
    virtual bool IsVolumetric() const { return false; }
    virtual Vector3f GetEmission(const Vector2f& uv) const { return Vector3f(0); }
    
protected:
    Vector3f GetSurfaceNormal(const Hit_Payload &rec) const;
    void SetNormal(std::shared_ptr<Texture> &normal);
    Vector3f NormalFromTangentToWorld(const Vector3f &surface_normal, const Vector3f &tangent_normal) const;

protected:
    std::shared_ptr<Texture> normal_texture = nullptr;
};

class Diffuse : public Material {
public:
    Diffuse(const Vector3f &a, float rough = 0.0f) : albedo_texture(std::make_shared<SolidTexture>(a)), roughness_texture(std::make_shared<SolidTexture>(Vector3f(rough))) {}
    Diffuse(std::shared_ptr<Texture> tex, std::shared_ptr<Texture> rough) : albedo_texture(tex), roughness_texture(rough) {}

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const override;

private:
    std::shared_ptr<Texture> albedo_texture;
    std::shared_ptr<Texture> roughness_texture;
};

class Conductor : public Material {
public:
    Conductor(const Vector3f &albedo, float roughness_u, float roughness_v, const Vector3f &eta, const Vector3f &k) :
        albedo_texture(std::make_shared<SolidTexture>(albedo)), roughness_texture_u(std::make_shared<SolidTexture>(Vector3f(roughness_u))), roughness_texture_v(std::make_shared<SolidTexture>(Vector3f(roughness_v))), eta(eta), k(k) {}

    Conductor(std::shared_ptr<Texture> albedo_tex, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, const Vector3f &eta, const Vector3f &k) :
        albedo_texture(albedo_tex), roughness_texture_u(roughness_u), roughness_texture_v(roughness_v), eta(eta), k(k) {}

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const override;

private:
    std::shared_ptr<Texture> albedo_texture; 
    std::shared_ptr<Texture> roughness_texture_u;  
    std::shared_ptr<Texture> roughness_texture_v;
    Vector3f eta, k; 
};

class Plastic : public Material {
public:
    Plastic(const Vector3f &albedo, const Vector3f &specular, float roughness_u, float roughness_v, float int_ior, float ext_ior, bool nonlinear = true) :
        albedo_texture(std::make_shared<SolidTexture>(albedo)),
        specular_texture(std::make_shared<SolidTexture>(specular)),
        roughness_texture_u(std::make_shared<SolidTexture>(Vector3f(roughness_u))),
        roughness_texture_v(std::make_shared<SolidTexture>(Vector3f(roughness_v))),
        eta(int_ior / ext_ior),
        nonlinear(nonlinear) {}

    Plastic(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> specular, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, float int_ior, float ext_ior, bool nonlinear = true) :
        albedo_texture(albedo),
        specular_texture(specular),
        roughness_texture_u(roughness_u),
        roughness_texture_v(roughness_v),
        eta(int_ior / ext_ior),
        nonlinear(nonlinear) {}

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const override;

private:
    std::shared_ptr<Texture> albedo_texture;      
    std::shared_ptr<Texture> specular_texture;    
    std::shared_ptr<Texture> roughness_texture_u; 
    std::shared_ptr<Texture> roughness_texture_v; 
    float eta;                                    
    bool nonlinear;
};

class Emission : public Material {
public:
    Emission(std::shared_ptr<Texture> tex, float intens = 1.0f) : albedo_texture(tex), intensity(intens) {}
    Emission(const Vector3f& emit, float intens = 1.0f) : albedo_texture(std::make_shared<SolidTexture>(emit)), intensity(intens) {}

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const override;
    virtual Vector3f Emit(const Ray &r_in, const Hit_Payload &rec, float u, float v) const override;
    virtual Vector3f GetEmission(const Vector2f &uv) const override;

private:
    std::shared_ptr<Texture> albedo_texture;
    float intensity;
};

class FrostedGlass : public Material {
public:
    FrostedGlass(const Vector3f &albedo, float roughness_u, float roughness_v, float int_ior, float ext_ior) :
        albedo_texture(std::make_shared<SolidTexture>(albedo)),
        roughness_texture_u(std::make_shared<SolidTexture>(Vector3f(roughness_u))),
        roughness_texture_v(std::make_shared<SolidTexture>(Vector3f(roughness_v))),
        eta(int_ior / ext_ior) {}

    FrostedGlass(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, float int_ior, float ext_ior) :
        albedo_texture(albedo),
        roughness_texture_u(roughness_u),
        roughness_texture_v(roughness_v),
        eta(int_ior / ext_ior) {}

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const override;

private:
    std::shared_ptr<Texture> albedo_texture;
    std::shared_ptr<Texture> roughness_texture_u;
    std::shared_ptr<Texture> roughness_texture_v;
    float eta;
};

class Glass : public Material {
public:
    Glass(float refraction_index) : refraction_index(refraction_index) {}
    
    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const override;
    virtual bool IsDelta() const override { return true; }
    
private:
    float refraction_index;
};