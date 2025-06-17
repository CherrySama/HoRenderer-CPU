/*
    Created by Yinghao He on 2025-06-12
*/
#include "PhaseFunction.hpp"
#include "Sampler.hpp"
#include "Ray.hpp"
#include "Hittable.hpp"


bool IsotropicPhase::Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const {
    Vector3f scatter_direction = sampler.random_unit_vector();
    scattered = Ray::SpawnRay(rec.p, scatter_direction, rec.normal);
    attenuation = tex->GetColor(rec.uv.x, rec.uv.y);
    return true;
}