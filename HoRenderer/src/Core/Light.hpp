/*
    Created by Yinghao He on 2025-06-22
*/
#pragma once

#include "Util.hpp"
#include "Shape.hpp"
#include "Scene.hpp"
// 1. Sample function: based on shading point information,
// sample light source direction and calculate expected radiance
// 2. Evaluate function: based on light source intersection information,
// calculate actual radiance (avoid repeated intersection detection)
class Light {
public:
    virtual ~Light() = default;

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const = 0;
    virtual Vector3f Evaluate(const Ray &r_in, const Hit_Payload &rec, float &pdf) const = 0;
    virtual float GetPower() const = 0;
    virtual std::shared_ptr<Hittable> GetShape() const = 0;
};

class QuadAreaLight : public Light {
public:
    QuadAreaLight(std::shared_ptr<Quad> quad) :
        quad(quad) {
        area = glm::length(glm::cross(quad->get_u(), quad->get_v()));
    }

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray &r_in, const Hit_Payload &rec, float &pdf) const override;
    virtual float GetPower() const override;
    virtual std::shared_ptr<Hittable> GetShape() const override;

private:
    std::shared_ptr<Quad> quad;
    float area;
};

class SphereAreaLight : public Light {
public:
    SphereAreaLight(std::shared_ptr<Sphere> sphere, const Vector3f &color, float intensity = 1.0f)
        : sphere(sphere) {
        center = sphere->getCenter();
        radius = sphere->getRadius();
        area = 4.0f * PI * radius * radius;
    }

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray &r_in, const Hit_Payload &rec, float &pdf) const override;
    virtual float GetPower() const override;
    virtual std::shared_ptr<Hittable> GetShape() const override;

private:
    Vector3f SampleSphereSurface(Sampler &sampler) const;

private:
    std::shared_ptr<Sphere> sphere;
    Vector3f center;
    float radius;
    float area;
};

class InfiniteAreaLight : public Light {
public:
    InfiniteAreaLight(std::shared_ptr<HDRTexture> hdr, float scale = 1.0f);

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, float& pdf) const override;
    virtual float GetPower() const override;
    virtual std::shared_ptr<Hittable> GetShape() const override;

private:
    inline float Luminance(const Vector3f &color) const {
        return 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
    }

private:
    std::shared_ptr<HDRTexture> hdr_texture;
    AliasTable2D table;
    float scale;
};