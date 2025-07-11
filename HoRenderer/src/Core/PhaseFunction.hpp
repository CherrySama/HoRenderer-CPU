/*
    Created by Yinghao He on 2025-06-12
*/
#pragma once

#include "Util.hpp"
#include "Texture.hpp"
#include "Material.hpp"


class IsotropicPhase : public Material {
public:
    IsotropicPhase(const Vector3f& albedo) : albedo_texture(std::make_shared<SolidTexture>(albedo)) {}
    IsotropicPhase(std::shared_ptr<Texture> albedo) : albedo_texture(albedo) {}

    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const override;
    virtual Vector3f Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const override;
    virtual bool IsVolumetric() const override { return true; }
    
private:
    std::shared_ptr<Texture> albedo_texture;
};

class HenyeyGreensteinPhase : public Material {
public:
    HenyeyGreensteinPhase(const Vector3f &albedo, float asymmetry_param) :
        albedo_texture(std::make_shared<SolidTexture>(albedo)), g(asymmetry_param) {}
    HenyeyGreensteinPhase(const Vector3f &albedo, const Vector3f &asymmetry_param) :
        albedo_texture(std::make_shared<SolidTexture>(albedo)), g(0.299f * asymmetry_param.x + 0.587f * asymmetry_param.y + 0.114f * asymmetry_param.z) {}

    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const override;
    virtual bool IsVolumetric() const override { return true; }
    
private:
    std::shared_ptr<Texture> albedo_texture;
    float g;  
};
