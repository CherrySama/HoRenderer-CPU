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
    float roughness = roughness_texture->GetColor(rec.uv.x, rec.uv.y)[0];

    scatter_direction = sampler.CosineSampleHemisphere(N);

    float NdotL = glm::dot(N, scatter_direction);
    float NdotV = glm::dot(N, V);
    float LdotV = glm::dot(scatter_direction, V);

    if (NdotL <= 0.0f || NdotV <= 0.0f) {
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
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    roughness_u *= roughness_u;
    roughness_v *= roughness_v;

    Vector3f H = sampler.GGXNVDSample(N, V, roughness_u, roughness_v);
    scatter_direction = glm::reflect(-V, H);

	float NdotV = glm::max(glm::dot(N, V), 0.0f);
    float NdotL = glm::max(glm::dot(N, scatter_direction), 0.0f);
    float VdotH = glm::dot(V, H);

    Vector3f F = BSDF::FresnelConductor(V, H, eta, k);
    float D = BSDF::DistributionGGX(H, N, roughness_u, roughness_v);
    float G1_V = BSDF::GeometrySchlickGGX(V, N, roughness_u, roughness_v);
    float Dv = G1_V * VdotH * D / NdotV;
    pdf = Dv * std::abs(1.0f / (4.0f * VdotH));

    float G1_L = BSDF::GeometrySchlickGGX(scatter_direction, N, roughness_u, roughness_v);
    float G = G1_V * G1_L;

	Vector3f brdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
    
    return brdf;
}

Vector3f Dielectric::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float alpha_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float alpha_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    alpha_u *= alpha_u;
    alpha_v *= alpha_v;
    float etai_over_etat = rec.front_face ? (1.0f / eta) : eta;

    Vector3f H = sampler.GGXNVDSample(N, V, alpha_u, alpha_v);
    float F = BSDF::FresnelDielectric(V, H, etai_over_etat);

    float NdotV = glm::max(glm::dot(N, V), 0.0f);
    float VdotH = glm::max(glm::dot(V, H), 0.0f);
    float G1_V = BSDF::GeometrySchlickGGX(V, N, alpha_u, alpha_v);
    float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
    float Dv = G1_V * VdotH * D / NdotV;

    if (sampler.random_float() < F) {
        scatter_direction = glm::reflect(-V, H);
        float NdotL = glm::dot(N, scatter_direction);

        if (NdotL <= 0.0f || NdotV <= 0.0f) { 
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        pdf = F * Dv / (4.0f * VdotH);
        float G1_L = BSDF::GeometrySchlickGGX(scatter_direction, N, alpha_u, alpha_v);  
        float G = G1_V * G1_L; 

        Vector3f brdf = albedo * F * D * G / (4.0f * NdotV * glm::abs(NdotL));
        return brdf;
    } else {
        scatter_direction = glm::refract(-V, H, etai_over_etat);

        if (glm::length(scatter_direction) < Epsilon) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        float NdotL = glm::dot(N, scatter_direction);
        if (NdotL * NdotV >= 0.0f) { 
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        float HdotV = glm::dot(H, V);
        float HdotL = glm::dot(H, scatter_direction);
        float sqrtDenom = etai_over_etat * HdotV + HdotL;
        
        float dwh_dwi = glm::abs(HdotL) / (sqrtDenom * sqrtDenom);
        pdf = (1.0f - F) * Dv * dwh_dwi;

        float factor = glm::abs(HdotL * HdotV / (NdotV * glm::abs(NdotL)));
        float G = BSDF::GeometrySmithGGX(V, scatter_direction, N, alpha_u, alpha_v);
        
        Vector3f btdf = albedo * (1.0f - F) * D * G * factor / (sqrtDenom * sqrtDenom);
        btdf *= (1.0f / (etai_over_etat * etai_over_etat));

        return btdf;
    }
}

Vector3f Emission::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    pdf = 0.0f;
    scatter_direction = Vector3f(0.0f);
    return Vector3f(0.0f);
}

Vector3f Emission::Emit(const Ray &r_in, const Hit_Payload &rec, float u, float v) const
{
    if (!rec.front_face)
        return Vector3f(0.0f);
        
    return intensity * albedo_texture->GetColor(u, v);
}