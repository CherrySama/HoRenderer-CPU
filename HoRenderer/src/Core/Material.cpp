/*
	Created by Yinghao He on 2025-05-21
*/
#include "Material.hpp"
#include "Hittable.hpp"
#include "Sampler.hpp"

Vector3f Material::Emit(float u, float v) const
{
	return Vector3f(0);
}

std::shared_ptr<Material> Material::Create(const MaterialParams& params)
{
    switch (params.type) {
    case MaterialType::LAMBERTIAN:
        return std::make_shared<Lambertian>(params.albedo_texture);
    case MaterialType::DIFFUSE_BRDF:
        return std::make_shared<DiffuseBRDF>(params.albedo_texture, params.roughness);
    case MaterialType::METAL:
        return std::make_shared<Metal>(params.albedo_texture, params.fuzz);
    case MaterialType::DIELECTRIC:
        return std::make_shared<Dielectric>(params.refractive_index);
    case MaterialType::DIFFUSELIGHT:
        return std::make_shared<DiffuseLight>(params.albedo_texture);
    }
    return NULL;
}

bool Lambertian::Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const
{
	// Generate random scattering directions (Lambertian distribution)
	Vector3f scatter_direction = rec.normal + sampler.random_unit_vector();

	// Preventing numerical problems caused by generating zero vectors
    if (glm::length2(scatter_direction) < Epsilon) 
        scatter_direction = rec.normal;
    
	// SpawnRay could avoid self-intersection problem
	scattered = Ray::SpawnRay(rec.p, scatter_direction, rec.normal);

	attenuation = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    
    return true;
}

bool DiffuseBRDF::Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    // Generate random scattering directions (Lambertian distribution)
	Vector3f scatter_direction = rec.normal + sampler.random_unit_vector();

	// Preventing numerical problems caused by generating zero vectors
    if (glm::length2(scatter_direction) < Epsilon) 
        scatter_direction = rec.normal;
    
	// SpawnRay could avoid self-intersection problem
	scattered = Ray::SpawnRay(rec.p, scatter_direction, rec.normal);

	// Calculate incident and outgoing directions
	Vector3f V = -glm::normalize(r_in.direction());  // Incident direction (pointing toward surface)
	Vector3f L = glm::normalize(scatter_direction);   // Outgoing direction
	Vector3f N = rec.normal;                          // Surface normal

	// Calculate dot products
	float NdotV = glm::clamp(glm::dot(N, V), 0.0f, 1.0f);
	float NdotL = glm::clamp(glm::dot(N, L), 0.0f, 1.0f);
	float LdotV = glm::dot(L, V);

	// Ensure correct hemisphere
	if (NdotV <= Epsilon || NdotL <= Epsilon) {
		attenuation = albedo * 0.1f;
		return true;
	}

	// A tiny improvement of Oren-Nayar reflectance model: https://mimosa-pudica.net/improved-oren-nayar.html
    float s = LdotV - NdotL * NdotV;
    float t = (s <= 0.0f) ? 1.0f : (1.0f / std::max(NdotL, NdotV));
    float sigma_prime = glm::clamp(roughness, 0.0f, 1.0f);

	// A = 1 / (π + (π/2 - 2/3)σ')
    // B = σ' / (π + (π/2 - 2/3)σ')
    float denominator = PI + (PI * 0.5f - 2.0f / 3.0f) * sigma_prime;
    float A = 1.0f / denominator;
    float B = sigma_prime / denominator;

	// L_ION(N,L,V) = ρ(N·L)(A + B·s/t)
    float oren_nayar_factor = A + B * s * t;
    Vector3f brdf_result = (albedo / PI) * NdotL * oren_nayar_factor;    
    attenuation = brdf_result;

    return true;
}

bool Metal::Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const
{
	Vector3f reflected = glm::reflect(r_in.direction(), rec.normal);
	reflected = glm::normalize(reflected) + (fuzz * sampler.random_unit_vector());
	scattered = Ray::SpawnRay(rec.p, reflected, rec.normal);
	attenuation = albedo_texture->GetColor(rec.uv.x, rec.uv.y);

	return (glm::dot(scattered.direction(), rec.normal) > 0.0f);
}

bool Dielectric::Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const
{
	attenuation = Vector3f(1.0f, 1.0f, 1.0f);
	float ri = rec.front_face ? (1.0f / refractive_index) : refractive_index;

	Vector3f unit_direction = glm::normalize(r_in.direction());
	float cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
	float sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

	bool cannot_refract = ri * sin_theta > 1.0;
	Vector3f direction;

	if (cannot_refract || Reflectance(cos_theta, ri) > sampler.random_float())
		direction = glm::reflect(unit_direction, rec.normal);
	else
		direction = glm::refract(unit_direction, rec.normal, ri);

	scattered = Ray::SpawnRay(rec.p, direction, rec.normal);
	return true;
}

float Dielectric::Reflectance(float cosine, float refraction_index)
{
	auto r0 = (1.0f - refraction_index) / (1.0f + refraction_index);
	r0 = r0 * r0;
	return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5.0f);
}

bool DiffuseLight::Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const
{
	return false;
}

Vector3f DiffuseLight::Emit(float u, float v) const
{
	return albedo_texture->GetColor(u, v);
}