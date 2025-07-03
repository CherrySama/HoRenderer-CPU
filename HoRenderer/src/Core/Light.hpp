/*
    Created by Yinghao He on 2025-06-22
*/
#pragma once

#include "Util.hpp"
#include "Shape.hpp"


class Light {
public:
    virtual ~Light() = default;

    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& light_direction, float& pdf, Sampler& sampler) const = 0;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& light_direction, float& pdf) const = 0;
    virtual bool IsHit(const Ray &ray, float max_distance, Vector3f &radiance) const = 0;

    virtual float GetPower() const { return power; }

protected:
    Vector3f intensity;     
    Vector3f color;         
    float power;
};

class QuadAreaLight : public Light {
public:
    QuadAreaLight(std::shared_ptr<Quad> quad, const Vector3f &color, float intensity = 1.0f) :
        quad(quad) {
        this->color = color;
        this->intensity = Vector3f(intensity);
        area = glm::length(glm::cross(quad->get_u(), quad->get_v()));
        power = glm::length(color) * intensity * area * PI;
    }

    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& light_direction, float& pdf, Sampler& sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& light_direction, float& pdf) const override;
    virtual bool IsHit(const Ray &ray, float max_distance, Vector3f &radiance) const override;

private:
    Vector3f SampleQuadSurface(Sampler &sampler) const;
    Vector3f GetQuadNormal() const;

private:
    std::shared_ptr<Quad> quad;
    float area;
};

class SphereAreaLight : public Light {
public:
    SphereAreaLight(std::shared_ptr<Sphere> sphere, const Vector3f &color, float intensity = 1.0f);

    virtual Vector3f Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const override;
    virtual Vector3f Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &light_direction, float &pdf) const override;
    virtual bool IsHit(const Ray &ray, float max_distance, Vector3f &radiance) const override;

private:
    Vector3f SampleSphereSurface(Sampler &sampler) const;
    bool SampleSphereDirection(const Vector3f &surface_pos, Sampler &sampler, Vector3f &direction, float &pdf) const;

private:
    std::shared_ptr<Sphere> sphere;
    Vector3f center;
    float radius;
    float area;
};

class InfiniteAreaLight : public Light {
public:
    InfiniteAreaLight(std::shared_ptr<HDRTexture> hdr, float scale = 1.0f);

    virtual Vector3f Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& light_direction, float& pdf, Sampler& sampler) const override;
    virtual Vector3f Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& light_direction, float& pdf) const override;
    virtual bool IsHit(const Ray& ray, float max_distance, Vector3f& radiance) const override;

private:
    Vector3f SphericalToCartesian(float theta, float phi) const;
    Vector2f CartesianToSpherical(const Vector3f &dir) const;

private:
    std::shared_ptr<HDRTexture> hdr_texture;
    float scale;
};