/*
	Created by Yinghao He on 2025-05-21
*/
#include "Material.hpp"

bool Lambertian::scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const
{
	// Generate random scattering directions (Lambertian distribution)
	Vector3f scatter_direction = rec.normal + sampler.random_unit_vector();

	// Preventing numerical problems caused by generating zero vectors
    if (glm::length2(scatter_direction) < Epsilon) 
        scatter_direction = rec.normal;
    
	// SpawnRay could avoid self-intersection problem
	scattered = Ray::SpawnRay(rec.p, scatter_direction, rec.normal);

	attenuation = albedo; // Simplified Lambertian BRDF
    
    return true;
}
