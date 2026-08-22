/*
	Created by Yinghao He on 2025-05-21
*/
#include "Material.hpp"
#include "Hittable.hpp"
#include "Sampler.hpp"
#include "BSDF.hpp"

namespace {

constexpr float DeltaRoughnessThreshold = 1e-6f;
constexpr float MinMicrofacetAlpha = 1e-4f;

struct PlasticSamplingWeights {
    float specular_probability;
    bool has_scattering;
};

PlasticSamplingWeights ComputePlasticSamplingWeights(const Vector3f& diffuse,
                                                      const Vector3f& specular,
                                                      float view_fresnel)
{
    const float diffuse_sum = glm::compAdd(glm::max(diffuse, Vector3f(0.0f)));
    const float specular_sum = glm::compAdd(glm::max(specular, Vector3f(0.0f)));
    const float component_sum = diffuse_sum + specular_sum;
    if (component_sum <= Epsilon) {
        return {0.0f, false};
    }

    const float base_specular_weight = specular_sum / component_sum;
    const float specular_weight = view_fresnel * base_specular_weight;
    const float diffuse_weight = (1.0f - view_fresnel) * (1.0f - base_specular_weight);
    const float weight_sum = specular_weight + diffuse_weight;
    if (weight_sum <= Epsilon) {
        return {0.0f, false};
    }

    return {specular_weight / weight_sum, true};
}

Vector3f EvaluateFabricBrdf(const Vector3f& albedo,
                            float roughness,
                            float sheen_weight,
                            float sheen_tint,
                            float NdotV,
                            float NdotL,
                            float NdotH,
                            float VdotH)
{
    const Vector3f base_color = glm::clamp(albedo, Vector3f(0.0f), Vector3f(1.0f));
    Vector3f brdf = (1.0f - sheen_weight) * base_color * INV_PI;

    if (NdotH <= 0.0f || NdotV <= 0.0f || NdotL <= 0.0f || sheen_weight <= 0.0f) {
        return brdf;
    }

    const float D = BSDF::DistributionCharlie(roughness, NdotH);
    const float G = glm::min(1.0f,
                             2.0f * NdotH * glm::min(NdotV, NdotL) /
                                 glm::max(VdotH, Epsilon));
    const Vector3f sheen_color = glm::mix(Vector3f(1.0f), base_color, sheen_tint);
    brdf += sheen_weight * sheen_color * D * G / (4.0f * NdotV * NdotL);
    return brdf;
}

}

Vector3f Material::Emit(const Ray& r_in, const Hit_Payload& rec, float u, float v) const
{
	return Vector3f(0);
}

bool Material::IsScatteringDirectionValid(const Hit_Payload& rec,
                                          const Vector3f& view_direction,
                                          const Vector3f& scatter_direction) const
{
    Vector3f geometric_normal = rec.geometric_normal;
    if (glm::length2(geometric_normal) <= Epsilon * Epsilon) {
        geometric_normal = rec.normal;
    }

    const float side_product = glm::dot(view_direction, geometric_normal) *
                               glm::dot(scatter_direction, geometric_normal);
    return side_product > 0.0f || (side_product < 0.0f && SupportsTransmission());
}

Vector3f Material::GetSurfaceNormal(const Hit_Payload& rec) const {
    Vector3f surface_normal = glm::normalize(rec.normal);

    if (normal_texture != nullptr) {
        Vector3f tangent_normal = normal_texture->GetColor(rec.uv.x, rec.uv.y);
        Vector3f mapped_normal = glm::normalize(tangent_normal * 2.0f - 1.0f);
        if (glm::length2(rec.tangent) > Epsilon * Epsilon &&
            glm::length2(rec.bitangent) > Epsilon * Epsilon) {
            Vector3f tangent = glm::normalize(rec.tangent -
                                              surface_normal * glm::dot(surface_normal, rec.tangent));
            Vector3f bitangent = glm::normalize(rec.bitangent -
                                                surface_normal * glm::dot(surface_normal, rec.bitangent));
            surface_normal = glm::normalize(mapped_normal.x * tangent +
                                            mapped_normal.y * bitangent +
                                            mapped_normal.z * surface_normal);
        } else {
            surface_normal = ToWorld(mapped_normal, surface_normal);
        }

        if (glm::length2(rec.geometric_normal) > Epsilon * Epsilon &&
            glm::dot(surface_normal, rec.geometric_normal) < 0.0f) {
            surface_normal = -surface_normal;
        }
    }
    
    return surface_normal;
}

void Material::SetNormal(std::shared_ptr<Texture> &normal) {
    normal_texture = normal;
}

Vector3f Diffuse::Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness = roughness_texture->GetColor(rec.uv.x, rec.uv.y)[0];

    scatter_direction = sampler.SampleCosineHemisphere(N);

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
    Vector3f brdf_result = albedo * oren_nayar_factor;
    
    return brdf_result;
}

Vector3f Diffuse::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness = roughness_texture->GetColor(rec.uv.x, rec.uv.y)[0];

    float NdotL = glm::dot(N, scatter_direction);
    float NdotV = glm::dot(N, V);
    float LdotV = glm::dot(scatter_direction, V);

    if (NdotL <= 0.0f || NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    pdf = NdotL / PI;

    float s = LdotV - NdotL * NdotV;
    float t = (s <= 0.0f) ? 1.0f : (1.0f / std::max(NdotL, NdotV));
    float sigma_prime = glm::clamp(roughness, 0.0f, 1.0f);
    
    float denominator = PI + (PI * 0.5f - 2.0f / 3.0f) * sigma_prime;
    float A = 1.0f / denominator;
    float B = sigma_prime / denominator;
    
    float oren_nayar_factor = A + B * s * t;
    Vector3f brdf_result = albedo * oren_nayar_factor;
    
    return brdf_result;
}


Vector3f Conductor::Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];

    if (IsDelta(rec) && glm::length2(rec.geometric_normal) > Epsilon * Epsilon) {
        N = glm::normalize(rec.geometric_normal);
    }

    float NdotV = glm::dot(N, V);
    if (NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    if (IsDelta(rec)) {
        scatter_direction = glm::reflect(-V, N);
        float NdotL = glm::dot(N, scatter_direction);
        if (NdotL <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        Vector3f F = BSDF::FresnelConductor(V, N, eta, k);
        pdf = 1.0f;
        return albedo * F / NdotL;
    }

    roughness_u = std::max(roughness_u * roughness_u, MinMicrofacetAlpha);
    roughness_v = std::max(roughness_v * roughness_v, MinMicrofacetAlpha);

    Vector3f H = sampler.GGXNVDSample(N, V, roughness_u, roughness_v);
    scatter_direction = glm::reflect(-V, H);

    float NdotL = glm::dot(N, scatter_direction);
    if (NdotV <= 0.0f || NdotL <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    float VdotH = glm::dot(V, H);

    Vector3f F = BSDF::FresnelConductor(V, H, eta, k);
    float D = BSDF::DistributionGGX(H, N, roughness_u, roughness_v);
    float G1_V = BSDF::GeometrySmithG1(V, H, N, roughness_u, roughness_v);
    float Dv = G1_V * VdotH * D / NdotV;
    float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, roughness_u, roughness_v);
    float G = G1_V * G1_L;

    pdf = Dv * std::abs(1.0f / (4.0f * VdotH));
    Vector3f brdf = albedo * F * D * G / (4.0f * NdotV * NdotL);

    return brdf;
}

Vector3f Conductor::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];

    if (IsDelta(rec)) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    roughness_u = std::max(roughness_u * roughness_u, MinMicrofacetAlpha);
    roughness_v = std::max(roughness_v * roughness_v, MinMicrofacetAlpha);

    Vector3f H = glm::normalize(V + scatter_direction);

    float NdotV = glm::dot(N, V);
    float NdotL = glm::dot(N, scatter_direction);
    if (NdotV <= 0.0f || NdotL <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    float VdotH = glm::dot(V, H);

    Vector3f F = BSDF::FresnelConductor(V, H, eta, k);
    float D = BSDF::DistributionGGX(H, N, roughness_u, roughness_v);
    float G1_V = BSDF::GeometrySmithG1(V, H, N, roughness_u, roughness_v);
    float Dv = G1_V * VdotH * D / NdotV;
    pdf = Dv * std::abs(1.0f / (4.0f * VdotH));
    float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, roughness_u, roughness_v);
    float G = G1_V * G1_L;

    Vector3f brdf = albedo * F * D * G / (4.0f * NdotV * NdotL);

    return brdf;
}

bool Conductor::IsDelta(const Hit_Payload& rec) const
{
    const float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    const float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    return std::abs(roughness_u) <= DeltaRoughnessThreshold &&
           std::abs(roughness_v) <= DeltaRoughnessThreshold;
}

Vector3f Plastic::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    Vector3f kd = albedo_texture->GetColor(rec.uv.x, rec.uv.y);      
    Vector3f ks = specular_texture->GetColor(rec.uv.x, rec.uv.y);   
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    float alpha_u = std::max(roughness_u * roughness_u, MinMicrofacetAlpha);
    float alpha_v = std::max(roughness_v * roughness_v, MinMicrofacetAlpha);

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    float NdotV = glm::dot(N, V);
    if (NdotV <= 0.0f) {
        pdf = 0.0f;
        scatter_direction = Vector3f(0.0f);
        return Vector3f(0.0f);
    }

    float F_avg = BSDF::AverageFresnelDielectric(eta);
    float F_view = BSDF::FresnelDielectric(V, N, 1.0f / eta);
    PlasticSamplingWeights sampling_weights = ComputePlasticSamplingWeights(kd, ks, F_view);
    if (!sampling_weights.has_scattering) {
        pdf = 0.0f;
        scatter_direction = Vector3f(0.0f);
        return Vector3f(0.0f);
    }

    Vector3f H;
    float NdotL;
    if (sampler.random_float() < sampling_weights.specular_probability) {
        H = sampler.GGXNVDSample(N, V, alpha_u, alpha_v);
        scatter_direction = glm::reflect(-V, H);

        NdotL = glm::max(glm::dot(N, scatter_direction), 0.0f);
        if (NdotL <= 0.0f || NdotV <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }
    } else {
        scatter_direction = sampler.SampleCosineHemisphere(N);
        H = glm::normalize(V + scatter_direction);

        NdotL = glm::max(glm::dot(N, scatter_direction), 0.0f);
        if (NdotL <= 0.0f || NdotV <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }
    }

    float F_light = BSDF::FresnelDielectric(scatter_direction, N, 1.0f / eta);
    Vector3f F = Vector3f(BSDF::FresnelDielectric(V, H, 1.0f / eta));
    float VdotH = glm::dot(V, H);
    float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
    float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
    float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
    float G = G1_V * G1_L;
    
    Vector3f diffuse = kd;
    Vector3f specular = ks;

    Vector3f brdf;
    if (nonlinear) {
        brdf = diffuse / (Vector3f(1.0f) - diffuse * F_avg);
    } else {
        brdf = diffuse / (Vector3f(1.0f) - F_avg);
    }

    brdf *= (1.0f - F_light) * (1.0f - F_view) / PI;
    brdf += specular * F * D * G / (4.0f * NdotL * NdotV);

    float Dv = G1_V * VdotH * D / NdotV;
    float pdf_NdotL = NdotL > 0.0f ? NdotL * INV_PI : 0.0f;
    pdf = sampling_weights.specular_probability * Dv * std::abs(1.0f / (4.0f * VdotH)) +
          (1.0f - sampling_weights.specular_probability) * pdf_NdotL;
    
    return brdf;
}

Vector3f Plastic::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const
{
    Vector3f kd = albedo_texture->GetColor(rec.uv.x, rec.uv.y);      
    Vector3f ks = specular_texture->GetColor(rec.uv.x, rec.uv.y);   
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];

    float alpha_u = std::max(roughness_u * roughness_u, MinMicrofacetAlpha);
    float alpha_v = std::max(roughness_v * roughness_v, MinMicrofacetAlpha);

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());

    float NdotV = glm::dot(N, V);
    float NdotL = glm::dot(N, scatter_direction);
    if (NdotL <= 0.0f || NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    float F_avg = BSDF::AverageFresnelDielectric(eta);
    float F_view = BSDF::FresnelDielectric(V, N, 1.0f / eta);
    PlasticSamplingWeights sampling_weights = ComputePlasticSamplingWeights(kd, ks, F_view);
    if (!sampling_weights.has_scattering) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f H = glm::normalize(V + scatter_direction);
    float F_light = BSDF::FresnelDielectric(scatter_direction, N, 1.0f / eta);
    Vector3f F = Vector3f(BSDF::FresnelDielectric(V, H, 1.0f / eta));
    float VdotH = glm::dot(V, H);
    float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
    float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
    float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
    float G = G1_V * G1_L;

    Vector3f diffuse = kd;
    Vector3f specular = ks;

    Vector3f brdf;
    if (nonlinear) {
        brdf = diffuse / (Vector3f(1.0f) - diffuse * F_avg);
    } else {
        brdf = diffuse / (Vector3f(1.0f) - F_avg);
    }

    brdf *= (1.0f - F_light) * (1.0f - F_view) * INV_PI;
    brdf += specular * F * D * G / (4.0f * NdotL * NdotV);

    float Dv = G1_V * VdotH * D / NdotV;
    float cosine_pdf = NdotL * INV_PI;
    pdf = sampling_weights.specular_probability * Dv * std::abs(1.0f / (4.0f * VdotH)) +
          (1.0f - sampling_weights.specular_probability) * cosine_pdf;
    
    return brdf;
}

Vector3f Emission::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    pdf = 0.0f;
    scatter_direction = Vector3f(0.0f);
    return Vector3f(0.0f);
}

Vector3f Emission::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    pdf = 0.0f;
    return Vector3f(0.0f);
}

Vector3f Emission::Emit(const Ray &r_in, const Hit_Payload &rec, float u, float v) const
{
    if (!rec.front_face)
        return Vector3f(0.0f);
        
    return intensity * albedo_texture->GetColor(u, v);
}

Vector3f Emission::Emit(const Vector2f &uv) const
{
    return intensity * albedo_texture->GetColor(uv.x, uv.y);
}

Vector3f Dielectric::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    float NdotV = glm::dot(N, V);
    if (NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    float eta_ratio = rec.front_face ? (1.0f / eta) : eta;

    if (IsDelta(rec)) {
        Vector3f geometric_normal = rec.geometric_normal;
        if (glm::length2(geometric_normal) <= Epsilon * Epsilon) {
            geometric_normal = N;
        }

        float F = BSDF::FresnelDielectric(V, geometric_normal, eta_ratio);
        if (sampler.random_float() < F) {
            scatter_direction = glm::reflect(-V, geometric_normal);
            float NdotL = glm::dot(geometric_normal, scatter_direction);
            if (NdotL <= 0.0f) {
                pdf = 0.0f;
                return Vector3f(0.0f);
            }

            pdf = F;
            return albedo * F / NdotL;
        }

        scatter_direction = glm::refract(-V, geometric_normal, eta_ratio);
        float NdotL = std::abs(glm::dot(geometric_normal, scatter_direction));
        float transmission_probability = 1.0f - F;
        if (NdotL <= Epsilon || transmission_probability <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        pdf = transmission_probability;
        return albedo * transmission_probability * eta_ratio * eta_ratio / NdotL;
    }

    float alpha_u = std::max(roughness_u * roughness_u, MinMicrofacetAlpha);
    float alpha_v = std::max(roughness_v * roughness_v, MinMicrofacetAlpha);
    Vector3f H = sampler.GGXNVDSample(N, V, alpha_u, alpha_v);
    float F = BSDF::FresnelDielectric(V, H, eta_ratio);

    if (sampler.random_float() < F) {
        scatter_direction = glm::reflect(-V, H);

        float NdotV = glm::dot(N, V);
        float NdotL = glm::dot(N, scatter_direction);
        if (NdotL <= 0.0f || NdotV <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }
        NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

        float VdotH = glm::dot(V, H);
        float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
        float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
        float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
        float G = G1_V * G1_L;
        float Dv = G1_V * VdotH * D / NdotV;

        pdf = F * Dv * std::abs(1.0f / (4.0f * VdotH));

        Vector3f bsdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
        return bsdf;

    } else {
        scatter_direction = glm::refract(-V, H, eta_ratio);

        float NdotL = glm::dot(N, scatter_direction);
        float NdotV = glm::dot(N, V);
        if (NdotL * NdotV >= 0.0f) {
			pdf = 0.0f;
			return Vector3f(0.0f);
		}
        NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

        float VdotH = glm::dot(V, H);
        float LdotH = glm::dot(scatter_direction, H);
        float F = BSDF::FresnelDielectric(V, H, eta_ratio);
        float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
        float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
        float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
        float G = G1_V * G1_L;
        float Dv = G1_V * VdotH * D / NdotV;

        float HdotV = glm::dot(H, V);
		float HdotL = glm::dot(H, scatter_direction);
		float sqrtDenom = eta_ratio * HdotV + HdotL;
        float factor = std::abs(HdotL * HdotV / (NdotL * NdotV));

        float dwh_dwi = std::abs(HdotL) / glm::pow(sqrtDenom, 2.0f);
        pdf = (1.0f - F) * Dv * dwh_dwi;

        Vector3f bsdf = albedo * (1.0f - F) * D * G * factor / glm::pow(sqrtDenom, 2.0f);
        bsdf *= eta_ratio * eta_ratio;

        return bsdf;
    }
}

Vector3f Dielectric::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];

    if (IsDelta(rec)) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    float alpha_u = std::max(roughness_u * roughness_u, MinMicrofacetAlpha);
    float alpha_v = std::max(roughness_v * roughness_v, MinMicrofacetAlpha);

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    float eta_ratio = rec.front_face ? (1.0f / eta) : eta;
    

    Vector3f H;
    Vector3f geometric_normal = rec.geometric_normal;
    if (glm::length2(geometric_normal) <= Epsilon * Epsilon) {
        geometric_normal = N;
    }
    bool isReflect = glm::dot(geometric_normal, scatter_direction) *
                     glm::dot(geometric_normal, V) > 0.0f;
    if (isReflect) {
		H = glm::normalize(V + scatter_direction);
	}
    else {
		H = -glm::normalize(eta_ratio * V + scatter_direction);
		if (glm::dot(N, H) < 0.0f) 
			H = -H;
    }

    float NdotV = glm::dot(N, V);
    float NdotL = glm::dot(N, scatter_direction);
    float VdotH = glm::dot(V, H);
    float F = BSDF::FresnelDielectric(V, H, eta_ratio);
    float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
    float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
    float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
    float G = G1_V * G1_L;
    float Dv = G1_V * VdotH * D / NdotV;

    pdf = Dv * std::abs(1.0f / (4.0f * glm::dot(V, H)));

    Vector3f bsdf;
    if (isReflect) {
		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;
			return Vector3f(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		float dwh_dwi = std::abs(1.0f / (4.0f * glm::dot(V, H)));
		pdf = F * Dv * dwh_dwi;

		bsdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
	}
    else {
		if (NdotL * NdotV >= 0.0f) {
			pdf = 0.0f;
			return Vector3f(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		float HdotV = glm::dot(H, V);
		float HdotL = glm::dot(H, scatter_direction);
		float sqrtDenom = eta_ratio * HdotV + HdotL;
		float factor = std::abs(HdotL * HdotV / (NdotL * NdotV));

		float dwh_dwi = std::abs(HdotL) / glm::pow(sqrtDenom, 2.0f);
		pdf = (1.0f - F) * Dv * dwh_dwi;

		bsdf = albedo * (1.0f - F) * D * G * factor / glm::pow(sqrtDenom, 2.0f);
		bsdf *= eta_ratio * eta_ratio;
    }

    return bsdf;
}

bool Dielectric::IsDelta(const Hit_Payload& rec) const
{
    const float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    const float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    return std::abs(roughness_u) <= DeltaRoughnessThreshold &&
           std::abs(roughness_v) <= DeltaRoughnessThreshold;
}

Vector3f Fabric::Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness = glm::clamp(roughness_texture->GetColor(rec.uv.x, rec.uv.y)[0], 0.01f, 1.0f);

    float NdotV = glm::dot(N, V);
    if (NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    scatter_direction = sampler.SampleUniformHemisphere(N);
    float NdotL = glm::dot(N, scatter_direction);
    if (NdotL <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f H = glm::normalize(V + scatter_direction);
    float NdotH = glm::max(glm::dot(N, H), 0.0f);
    float VdotH = glm::max(glm::dot(V, H), 0.0f);
    
    Vector3f brdf = EvaluateFabricBrdf(albedo, roughness, sheen_weight, sheen_tint,
                                       NdotV, NdotL, NdotH, VdotH);

    pdf = INV_2PI;
    
    return brdf;
}

Vector3f Fabric::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness = glm::clamp(roughness_texture->GetColor(rec.uv.x, rec.uv.y)[0], 0.01f, 1.0f);

    float NdotV = glm::dot(N, V);
    float NdotL = glm::dot(N, scatter_direction);
    
    if (NdotV <= 0.0f || NdotL <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f H = glm::normalize(V + scatter_direction);
    float NdotH = glm::max(glm::dot(N, H), 0.0f);
    float VdotH = glm::max(glm::dot(V, H), 0.0f);
    
    Vector3f brdf = EvaluateFabricBrdf(albedo, roughness, sheen_weight, sheen_tint,
                                       NdotV, NdotL, NdotH, VdotH);

    pdf = INV_2PI;  // 1/(2π)
    
    return brdf;
}
