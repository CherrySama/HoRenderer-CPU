/*
	Created by Yinghao He on 2025-05-21
*/
#include "Material.hpp"
#include "Hittable.hpp"
#include "Sampler.hpp"
#include "BSDF.hpp"

Vector3f Material::Emit(const Ray& r_in, const Hit_Payload& rec, float u, float v) const
{
	return Vector3f(0);
}

Vector3f Material::GetSurfaceNormal(const Hit_Payload& rec) const {
    Vector3f surface_normal = rec.normal;

    if (normal_texture != nullptr) {
        Vector3f tangent_normal = normal_texture->GetColor(rec.uv.x, rec.uv.y);
        
        surface_normal = NormalFromTangentToWorld(rec.normal, tangent_normal);
    }
    
    return surface_normal;
}

void Material::SetNormal(std::shared_ptr<Texture> &normal) {
    normal_texture = normal;
}

Vector3f Material::NormalFromTangentToWorld(const Vector3f &surface_normal, const Vector3f &tangent_normal) const {
    Vector3f mapped_normal = glm::normalize(tangent_normal * 2.0f - 1.0f);

    Vector3f up_vector = std::abs(surface_normal.z) < 0.9f ? Vector3f(0.0f, 0.0f, 1.0f) : Vector3f(1.0f, 0.0f, 0.0f);

    Vector3f tangent_x = glm::normalize(glm::cross(up_vector, surface_normal));
    Vector3f tangent_y = glm::normalize(glm::cross(tangent_x, surface_normal));

    return glm::normalize(tangent_x * mapped_normal.x + tangent_y * mapped_normal.y + surface_normal * mapped_normal.z);
}

Vector3f Diffuse::Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);

    scatter_direction = sampler.CosineSampleHemisphere(N);

    float NdotL = glm::dot(N, scatter_direction);
    float NdotV = glm::dot(N, V);
    float LdotV = glm::dot(scatter_direction, V);

    if (NdotL <= Epsilon || NdotV <= Epsilon) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    pdf = NdotL / PI; // PDF = cos(θ)/π
    
    float s = LdotV - NdotL * NdotV;
    float t = (s <= 0.0f) ? 1.0f : (1.0f / std::max(NdotL, NdotV));
    float sigma_prime = glm::clamp(roughness, 0.0f, 1.0f);
    
    float denominator = PI + (PI * 0.5f - 2.0f / 3.0f) * sigma_prime;
    float A = 1.0f / denominator;
    float B = sigma_prime / denominator;
    
    float oren_nayar_factor = A + B * s * t;
    Vector3f brdf_result = (albedo / PI) * oren_nayar_factor;
    
    return brdf_result;
}

Vector3f Conductor::Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const
{
	Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);

    scatter_direction = sampler.GGXSampleHemisphere(N, V, roughness_u, roughness_v);
	float NdotV = glm::max(glm::dot(N, V), 0.0f);
    float NdotL = glm::max(glm::dot(N, scatter_direction), 0.0f);
	if (NdotL <= Epsilon || NdotV <= Epsilon) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

	Vector3f H = glm::normalize(V + scatter_direction);
    Vector3f F = BSDF::FresnelConductor(V, H, eta, k);
    float D = BSDF::DistributionGGX(H, N, roughness_u, roughness_v);
    float G = BSDF::GeometrySmithGGX(V, scatter_direction, N, roughness_u, roughness_v);

	float VdotH = glm::max(glm::dot(V, H), 0.0f);
    float G1 = BSDF::GeometrySchlickGGX(V, N, roughness_u, roughness_v);
    pdf = (D * G1) / (4.0f * NdotV); 

	Vector3f brdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
    
    return brdf;
}

// bool Dielectric::Scatter(const Ray& r_in, const Hit_Payload& rec, Vector3f& attenuation, Ray& scattered, Sampler& sampler) const
// {
// 	Vector3f surface_normal = GetSurfaceNormal(rec);
// 	attenuation = Vector3f(1.0f, 1.0f, 1.0f);
// 	float ri = rec.front_face ? (1.0f / refractive_index) : refractive_index;

// 	Vector3f unit_direction = glm::normalize(r_in.direction());
// 	float cos_theta = std::fmin(dot(-unit_direction, surface_normal), 1.0);
// 	float sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

// 	bool cannot_refract = ri * sin_theta > 1.0;
// 	Vector3f direction;

// 	if (cannot_refract || Reflectance(cos_theta, ri) > sampler.random_float())
// 		direction = glm::reflect(unit_direction, surface_normal);
// 	else
// 		direction = glm::refract(unit_direction, surface_normal, ri);

// 	scattered = Ray::SpawnRay(rec.p, direction, surface_normal);
// 	return true;
// }

// float Dielectric::Reflectance(float cosine, float refraction_index)
// {
// 	auto r0 = (1.0f - refraction_index) / (1.0f + refraction_index);
// 	r0 = r0 * r0;
// 	return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5.0f);
// }

// bool DiffuseLight::Scatter(const Ray &r_in, const Hit_Payload &rec, Vector3f &attenuation, Ray &scattered, Sampler &sampler) const
// {
// 	return false;
// }

// Vector3f DiffuseLight::Emit(const Ray& r_in, const Hit_Payload& rec, float u, float v) const
// {
//     if (!rec.front_face)
//         return Vector3f(0);

//     return albedo_texture->GetColor(u, v);
// }