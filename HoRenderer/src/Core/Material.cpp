/*
	Created by Yinghao He on 2025-05-21
*/
#include "Material.hpp"
#include "Hittable.hpp"

bool Lambertian::Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const
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

bool DiffuseBRDF::Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const
{
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
	Vector3f H = glm::normalize(V + L);

	// Calculate dot products
	float NdotV = glm::clamp(glm::dot(N, V), 0.0f, 1.0f);
	float NdotL = glm::clamp(glm::dot(N, L), 0.0f, 1.0f);
	float VdotH = glm::clamp(glm::dot(V, H), 0.0f, 1.0f);

	// Ensure correct hemisphere
	if (NdotV <= Epsilon || NdotL <= Epsilon) {
		attenuation = albedo * 0.1f;
		return true;
	}

	// Oren Nayar Params
    float theta_i = std::acos(glm::clamp(NdotV, 0.0f, 1.0f));
    float theta_r = std::acos(glm::clamp(NdotL, 0.0f, 1.0f));
    float alpha = std::max(theta_i, theta_r);
    float beta = std::min(theta_i, theta_r);

	// Calculate the azimuth difference cos(φᵢ - φᵣ)
    // Vector3f V_perp = V - NdotV * N;
    // Vector3f L_perp = L - NdotL * N;
    // float cos_phi_diff = 0.0f;

	// float V_perp_len = glm::length(V_perp);
    // float L_perp_len = glm::length(L_perp);
    // if (V_perp_len > 1e-6f && L_perp_len > 1e-6f) {
    //     cos_phi_diff = glm::dot(V_perp, L_perp) / (V_perp_len * L_perp_len);
    // }

	// Simplified calculation of cos_phi_diff using half-vector
	float cos_phi_diff = 2.0f * VdotH * VdotH - 1.0f;

    float sigma2 = roughness * roughness;

	// Calculation coefficient
	float C1 = 1.0f - 0.5f * sigma2 / (sigma2 + 0.33f);
    
    float C2 = 0.45f * sigma2 / (sigma2 + 0.09f) * std::sin(alpha);
    // if (cos_phi_diff >= 0.0f) {
    //     C2 = 0.45f * sigma2 / (sigma2 + 0.09f) * std::sin(alpha);
    // } else {
    //     C2 = 0.45f * sigma2 / (sigma2 + 0.09f) * (std::sin(alpha) - std::pow(2.0f * beta / PI, 3.0f));
    // }
    
    // float C3 = 0.125f * sigma2 / (sigma2 + 0.09f) * std::pow(4.0f * alpha * beta / (PI * PI), 2.0f);
	// float C3 = 0.0f;

	// L1 item (primary reflex)
    // Vector3f L1 = (albedo / PI) * (C1 + C2 * cos_phi_diff * std::tan(beta) + C3 * (1.0f - std::abs(cos_phi_diff)) * std::tan((alpha + beta) / 2.0f));
	Vector3f L1 = (albedo / PI) * (C1 + C2 * cos_phi_diff * std::tan(beta));

    // L2 term (multiple scattering)
    // Vector3f L2 = 0.17f *  (albedo * albedo / PI) * sigma2 / (sigma2 + 0.13f) * (1.0f - cos_phi_diff * std::pow(2.0f * beta / PI, 2.0f));

    Vector3f oren_nayar = L1;
    attenuation = oren_nayar * PI;

	return true;
}

bool Metal::Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const
{
	Vector3f reflected = sampler.Reflect(r_in.direction(), rec.normal);
	reflected = glm::normalize(reflected) + (fuzz * sampler.random_unit_vector());
	scattered = Ray::SpawnRay(rec.p, reflected, rec.normal);
	attenuation = albedo;

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
		direction = sampler.Reflect(unit_direction, rec.normal);
	else
		direction = sampler.Refract(unit_direction, rec.normal, ri);

	scattered = Ray::SpawnRay(rec.p, direction, rec.normal);
	return true;
}

float Dielectric::Reflectance(float cosine, float refraction_index)
{
	auto r0 = (1.0f - refraction_index) / (1.0f + refraction_index);
	r0 = r0 * r0;
	return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5.0f);
}
