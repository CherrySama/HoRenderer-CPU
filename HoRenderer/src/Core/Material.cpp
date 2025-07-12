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
    return ToWorld(mapped_normal, surface_normal);
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
    pdf = Dv * std::abs(1.0f / (4.0f * VdotH));
    float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, roughness_u, roughness_v);
    float G = G1_V * G1_L;

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
        scatter_direction = sampler.CosineSampleHemisphere(N);
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
    float NdotL = glm::max(glm::dot(N, scatter_direction), 0.0f);
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

    brdf *= (1.0f - Fi) * (1.0f - Fo) / PI;

    brdf += specular * F * D * G / (4.0f * NdotL * NdotV);

    float Dv = G1_V * VdotH * D / NdotV;

    float cosine_pdf = NdotL / PI;
    
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

Vector3f FrostedGlass::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &scatter_direction, float &pdf, Sampler &sampler) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());

    float NdotV = glm::dot(N, V);
    if (NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    
    float eta_ratio = rec.front_face ? eta : (1.0f / eta);
    float F_approx = BSDF::FresnelDielectric(V, N, eta_ratio);
    float avg_roughness = (roughness_u + roughness_v) * 0.5f;
    float F_avg = BSDF::AverageFresnelDielectric(eta);
    Vector3f ms_compensation = BSDF::MultipleScatteringCompensation(albedo, avg_roughness, F_avg);

    if (sampler.random_float() < F_approx) {
        Vector3f H = sampler.GGXNVDSample(N, V, alpha_u, alpha_v);
        scatter_direction = glm::reflect(-V, H);

        float NdotL = glm::dot(N, scatter_direction);
        if (NdotL <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        float VdotH = glm::dot(V, H);
        float F = BSDF::FresnelDielectric(V, H, eta_ratio);
        float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
        float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
        float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);

        float Dv = G1_V * VdotH * D / NdotV;
        pdf = F_approx * Dv * std::abs(1.0f / (4.0f * VdotH));

        Vector3f brdf = albedo * F * D * G1_V / (4.0f * NdotV * NdotL);
        brdf += ms_compensation * (1.0f - G1_V) * F;
        return brdf;
    } else {
        Vector3f H = sampler.GGXDistributionSample(N, alpha_u, alpha_v);
        Vector3f w_i = -V;  
        float c_i = glm::dot(w_i, H);

        if (c_i < 0.0f) {
            H = -H;  
            c_i = -c_i;
        }
        float eta_inv = 1.0f / eta_ratio;
        float sin_t_squared = eta_inv * eta_inv * (1.0f - c_i * c_i);

        if (sin_t_squared >= 1.0f) {
            scatter_direction = glm::reflect(-V, H);

            float NdotL = glm::dot(N, scatter_direction);
            if (NdotL <= 0.0f) {
                pdf = 0.0f;
                return Vector3f(0.0f);
            }

            float VdotH = glm::dot(V, H);
            float F = BSDF::FresnelDielectric(V, H, eta_ratio);
            float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
            float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
            float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
            float G = G1_V * G1_L;
            float HdotN = glm::dot(H, N);
            pdf = (1.0f - F_approx) * D * HdotN * std::abs(1.0f / (4.0f * VdotH));

            Vector3f brdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
            return brdf;
        }
        float c_o = std::sqrt(1.0f - sin_t_squared); 
        Vector3f w_o = -eta_inv * w_i + (eta_inv * c_i - c_o) * H;
        scatter_direction = glm::normalize(w_o);

        float NdotL = glm::dot(N, scatter_direction);
        if ((rec.front_face && NdotL >= 0.0f) || (!rec.front_face && NdotL <= 0.0f)) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        float VdotH = glm::dot(V, H);
        float LdotH = glm::dot(scatter_direction, H);
        float F = BSDF::FresnelDielectric(V, H, eta_ratio);
        float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
        float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
        float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
        float G = G1_V * G1_L;

        float denom = VdotH + eta_ratio * LdotH;
        if (std::abs(denom) < Epsilon) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        float jacobian_factor = std::abs(LdotH) / std::abs(denom);
        jacobian_factor *= jacobian_factor;

        float HdotN = glm::dot(H, N);
        pdf = (1.0f - F_approx) * D * HdotN * jacobian_factor;

        float eta_factor = rec.front_face ? (eta_ratio * eta_ratio) : 1.0f;
        Vector3f btdf = albedo * (1.0f - F) * D * G * std::abs(VdotH * LdotH) * eta_factor / (std::abs(NdotV * NdotL) * denom * denom);
        btdf += ms_compensation * (1.0f - G) * (1.0f - F);
        return btdf;
    }
}

Vector3f FrostedGlass::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    Vector3f albedo = albedo_texture->GetColor(rec.uv.x, rec.uv.y);
    float roughness_u = roughness_texture_u->GetColor(rec.uv.x, rec.uv.y)[0];
    float roughness_v = roughness_texture_v->GetColor(rec.uv.x, rec.uv.y)[0];
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());

    float NdotV = glm::dot(N, V);
    if (NdotV <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    float eta_ratio = rec.front_face ? (1.0f / eta) : eta;
    float NdotL = glm::dot(N, scatter_direction);

    Vector3f H;
    if ((NdotL > 0.0f && rec.front_face) || (NdotL < 0.0f && !rec.front_face)) {
        if (NdotL <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        H = glm::normalize(V + scatter_direction);
        float F = BSDF::FresnelDielectric(V, H, eta_ratio);
        float VdotH = glm::dot(V, H);
        float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
        float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
        float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
        float G = G1_V * G1_L;

        float Dv = G1_V * VdotH * D / NdotV;
        pdf = F * Dv * std::abs(1.0f / (4.0f * VdotH));
        
        Vector3f brdf = albedo * F * D * G1_V / (4.0f * NdotV * NdotL);
        return brdf;
    } else {
        if ((rec.front_face && NdotL >= 0.0f) || (!rec.front_face && NdotL <= 0.0f)) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        H = rec.front_face ? -glm::normalize(V + scatter_direction * eta) : H = -glm::normalize(V * eta + scatter_direction);
        if (glm::dot(H, N) < 0.0f) 
            H = -H;

        float F = BSDF::FresnelDielectric(V, H, eta_ratio);
        float VdotH = glm::dot(V, H);
        float LdotH = glm::dot(scatter_direction, H);
        float D = BSDF::DistributionGGX(H, N, alpha_u, alpha_v);
        float G1_V = BSDF::GeometrySmithG1(V, H, N, alpha_u, alpha_v);
        float G1_L = BSDF::GeometrySmithG1(scatter_direction, H, N, alpha_u, alpha_v);
        float G = G1_V * G1_L;

        float denom = VdotH + eta_ratio * LdotH;
        if (std::abs(denom) < Epsilon) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }

        float jacobian_factor = std::abs(LdotH) / std::abs(denom);
        jacobian_factor *= jacobian_factor;

        float HdotN = glm::dot(H, N);
        pdf = (1.0f - F) * D * HdotN * jacobian_factor;

        float eta_factor = rec.front_face ? (eta_ratio * eta_ratio) : 1.0f;
        Vector3f btdf = albedo * (1.0f - F) * D * G * std::abs(VdotH * LdotH) * eta_factor / (std::abs(NdotV * NdotL) * denom * denom);
        
        return btdf;
    }
}

Vector3f Glass::Sample(const Ray& r_in, const Hit_Payload& rec, Vector3f& scatter_direction, float& pdf, Sampler& sampler) const
{
    Vector3f N = GetSurfaceNormal(rec);
    Vector3f V = -glm::normalize(r_in.direction());

    float eta_ratio = rec.front_face ? (1.0f / refraction_index) : refraction_index;
    
    float cos_theta = glm::clamp(glm::dot(V, N), 0.0f, 1.0f);
    float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

    if (eta_ratio * sin_theta > 1.0f) {
        scatter_direction = glm::reflect(-V, N);
        pdf = 1.0f;
        return Vector3f(1.0f, 1.0f, 1.0f);
    } else {
        float fresnel_reflectance = BSDF::FresnelDielectric(V, N, eta_ratio);
        
        if (sampler.random_float() < fresnel_reflectance) {
            scatter_direction = glm::reflect(-V, N);
            pdf = 1.0f;
            return Vector3f(1.0f, 1.0f, 1.0f);
        } else {
            scatter_direction = glm::refract(-V, N, eta_ratio);
            
            if (glm::length(scatter_direction) < Epsilon) {
                scatter_direction = glm::reflect(-V, N);
                pdf = 1.0f;
                return Vector3f(1.0f, 1.0f, 1.0f);
            }
            
            pdf = 1.0f;

            float eta_factor = rec.front_face ? (eta_ratio * eta_ratio) : 1.0f;
            return Vector3f(eta_factor, eta_factor, eta_factor);
        }
    }
}

Vector3f Glass::Evaluate(const Ray& r_in, const Hit_Payload& rec, const Vector3f& scatter_direction, float& pdf) const
{
    pdf = Epsilon;
    return Vector3f(Epsilon, Epsilon, Epsilon);
}