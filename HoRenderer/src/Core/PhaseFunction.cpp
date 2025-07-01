/*
    Created by Yinghao He on 2025-06-12
*/
#include "PhaseFunction.hpp"
#include "Sampler.hpp"
#include "Ray.hpp"
#include "Hittable.hpp"


Vector3f IsotropicPhase::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    scatter_direction = sampler.random_unit_vector();
    pdf = INV_4PI;
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    return albedo;
}

Vector3f IsotropicPhase::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const
{
    pdf = INV_4PI;
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    return albedo;
}