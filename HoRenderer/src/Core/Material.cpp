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
        Vector3f mapped_normal = glm::normalize(tangent_normal * 2.0f - 1.0f);
        surface_normal = ToWorld(mapped_normal, rec.normal);
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
    Vector3f brdf_result = (albedo / PI) * oren_nayar_factor;
    
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
    roughness_u *= roughness_u;
    roughness_v *= roughness_v;

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

Vector3f Plastic::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    Vector3f kd = albedo_texture->GetColor(rec.uv.x, rec.uv.y);      
    Vector3f ks = specular_texture->GetColor(rec.uv.x, rec.uv.y);   
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;
    float d_sum = kd.x + kd.y + kd.z;
    float s_sum = ks.x + ks.y + ks.z;

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());

    float etai_over_etat = rec.front_face ? (1.0f / eta) : eta;
    float F_avg = BSDF::AverageFresnelDielectric(eta);
    float Fo = BSDF::FresnelDielectric(V, N, 1.0f / eta);
    float Fi = Fo;

    float specular_sampling_weight = s_sum / (s_sum + d_sum);
    float pdf_specular = Fi * specular_sampling_weight;
    float pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);
    pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

    Vector3f H;
    float NdotV = glm::dot(N, V);
    float NdotL;
    if (sampler.random_float() < pdf_specular) {
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
        Fi = BSDF::FresnelDielectric(scatter_direction, N, 1.0f / eta);

        NdotL = glm::max(glm::dot(N, scatter_direction), 0.0f);
        if (NdotL <= 0.0f || NdotV <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }
    }
    
    Vector3f F = Vector3f(BSDF::FresnelDielectric(scatter_direction, H, 1.0f / eta));
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

    brdf *= (1.0f - Fi) * (1.0f - Fo) / PI;
    brdf += specular * F * D * G / (4.0f * NdotL * NdotV);

    float Dv = G1_V * VdotH * D / NdotV;
    float pdf_NdotL = NdotL > 0.0f ? NdotL * INV_PI : 0.0f;
    pdf = pdf_specular * Dv * std::abs(1.0f / (4.0f * VdotH)) + (1.0f - pdf_specular) * pdf_NdotL;
    
    return brdf;
}

Vector3f Plastic::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &scatter_direction, float &pdf) const
{
    Vector3f kd = albedo_texture->GetColor(rec.uv.x, rec.uv.y);      
    Vector3f ks = specular_texture->GetColor(rec.uv.x, rec.uv.y);   
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];

    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;
    float d_sum = kd.x + kd.y + kd.z;
    float s_sum = ks.x + ks.y + ks.z;

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f H = glm::normalize(V + scatter_direction);

    float etai_over_etat = rec.front_face ? (1.0f / eta) : eta;
    float F_avg = BSDF::AverageFresnelDielectric(eta);
    float Fo = BSDF::FresnelDielectric(V, N, 1.0f / eta);
    float Fi = BSDF::FresnelDielectric(scatter_direction, N, 1.0f / eta);

    float specular_sampling_weight = s_sum / (s_sum + d_sum);
    float pdf_specular = Fi * specular_sampling_weight;
    float pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);
    pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

    float NdotV = glm::dot(N, V);
    float NdotL = glm::dot(N, scatter_direction);
    if (NdotL <= 0.0f || NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f F = Vector3f(BSDF::FresnelDielectric(scatter_direction, H, 1.0f / eta));
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

    brdf *= (1.0f - Fi) * (1.0f - Fo) * INV_PI;
    brdf += specular * F * D * G / (4.0f * NdotL * NdotV);

    float Dv = G1_V * VdotH * D / NdotV;
    float cosine_pdf = NdotL * INV_PI;
    pdf = pdf_specular * Dv * std::abs(1.0f / (4.0f * VdotH)) + (1.0f - pdf_specular) * cosine_pdf;
    
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
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    
    Vector3f H = sampler.GGXNVDSample(N, V, alpha_u, alpha_v);
    float eta_ratio = rec.front_face ? (1.0f / eta) : eta;
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
        float Dv = G1_V * VdotH * D / NdotV;

        pdf = F * Dv * std::abs(1.0f / (4.0f * VdotH));

        Vector3f bsdf = albedo * F * D * G1_V / (4.0f * NdotV * NdotL);
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
        bsdf *= glm::pow(1.0f / eta_ratio, 2.0f);

        return bsdf;
    }
}

Vector3f Dielectric::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    float eta_ratio = rec.front_face ? (1.0f / eta) : eta;
    

    Vector3f H;
    bool isReflect = glm::dot(N, scatter_direction) * glm::dot(N, V) >= 0.0f;
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
		bsdf *= glm::pow(1.0f / eta_ratio, 2.0f);
    }

    return bsdf;
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

    scatter_direction = sampler.SampleCosineHemisphere(N);
    float NdotL = glm::dot(N, scatter_direction);
    if (NdotL <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f H = glm::normalize(V + scatter_direction);
    float NdotH = glm::max(glm::dot(N, H), 0.0f);
    
    Vector3f diffuse = albedo * INV_PI;
    
    if (NdotH > 0.0f) {
        float charlie_d = BSDF::DistributionCharlie(roughness, NdotH);
        Vector3f sheen_color = glm::mix(Vector3f(1.0f), albedo, tint);
        Vector3f sheen = intensity * sheen_color * charlie_d;
        diffuse += sheen;
    }

    pdf = NdotL * INV_PI;
    return diffuse;
}

Vector3f Fabric::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());
    Vector3f L = glm::normalize(scatter_direction);
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness = glm::clamp(roughness_texture->GetColor(rec.uv.x, rec.uv.y)[0], 0.01f, 1.0f);

    float NdotV = glm::dot(N, V);
    float NdotL = glm::dot(N, L);
    
    if (NdotV <= 0.0f || NdotL <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f H = glm::normalize(V + L);
    float NdotH = glm::max(glm::dot(N, H), 0.0f);
    
    Vector3f diffuse = albedo * INV_PI;
    
    if (NdotH > 0.0f) {
        float charlie_d = BSDF::DistributionCharlie(roughness, NdotH);
        Vector3f sheen_color = glm::mix(Vector3f(1.0f), albedo, tint);
        Vector3f sheen = intensity * sheen_color * charlie_d;
        diffuse += sheen;
    }

    pdf = NdotL * INV_PI;
    return diffuse;
}