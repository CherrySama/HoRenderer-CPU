/*
    Created by Yinghao He on 2025-06-23
*/
#include "BSDF.hpp"

namespace BSDF {
	float DistributionGGX(const Vector3f &H, const Vector3f &N, float alpha_u, float alpha_v)
    {
        float NdotH = glm::max(glm::dot(N, H), 0.0f);
        if (NdotH <= 0.0f) return 0.0f;
        
        if (alpha_u == alpha_v) {
            float alpha2 = alpha_u * alpha_u;
            float NdotH2 = NdotH * NdotH;
            float denom = (NdotH2 * (alpha2 - 1.0f) + 1.0f);
            return alpha2 / (PI * denom * denom);
        } else {
            Vector3f local_H = ToLocal(H, N);
            float term = (local_H.x * local_H.x) / (alpha_u * alpha_u) + (local_H.y * local_H.y) / (alpha_v * alpha_v) + (local_H.z * local_H.z);
            return 1.0f / (PI * alpha_u * alpha_v * term * term);
        }
    }

    float GeometrySmithG1(const Vector3f &V, const Vector3f &H, const Vector3f &N, float alpha_u, float alpha_v)
    {
        float cos_v_n = glm::dot(V, N);

        if (cos_v_n * glm::dot(V, H) <= 0.0f) {
            return 0.0f;
        }

        if (std::abs(cos_v_n - 1.0f) < Epsilon) {
            return 1.0f;
        }

        if (alpha_u == alpha_v) {
            float cos_v_n_2 = cos_v_n * cos_v_n;
            float tan_v_n_2 = (1.0f - cos_v_n_2) / cos_v_n_2;
            float alpha_2 = alpha_u * alpha_u;

            return 2.0f / (1.0f + std::sqrt(1.0f + alpha_2 * tan_v_n_2));
        } else {
            Vector3f dir = ToLocal(V, N);
            float xy_alpha_2 = (alpha_u * dir.x) * (alpha_u * dir.x) + (alpha_v * dir.y) * (alpha_v * dir.y);
            float tan_v_n_alpha_2 = xy_alpha_2 / (dir.z * dir.z);

            return 2.0f / (1.0f + std::sqrt(1.0f + tan_v_n_alpha_2));
        }
    }

    Vector3f FresnelConductor(const Vector3f &V, const Vector3f &H, const Vector3f &eta, const Vector3f &k)
	{
        float VdotH = glm::max(glm::dot(V, H), 0.0f);
        Vector3f result;

        for (int i = 0; i < 3; i++)
		{
            float eta_r = eta[i];
            float eta_i = k[i];
            
            float cos_theta_2 = VdotH * VdotH;
            float sin_theta_2 = 1.0f - cos_theta_2;
            float eta_r2 = eta_r * eta_r;
            float eta_i2 = eta_i * eta_i;
            
            float temp1 = eta_r2 - eta_i2 - sin_theta_2;
            float a2_plus_b2 = std::sqrt(temp1 * temp1 + 4.0f * eta_r2 * eta_i2);
            float a = std::sqrt(0.5f * (a2_plus_b2 + temp1));
            
            float term1 = a2_plus_b2 + cos_theta_2;
            float term2 = 2.0f * VdotH * a;
            float term3 = a2_plus_b2 * cos_theta_2 + sin_theta_2 * sin_theta_2;
            float term4 = term2 * sin_theta_2;
            
            float Rs = (term1 - term2) / (term1 + term2);
            float Rp = Rs * (term3 - term4) / (term3 + term4);
            
            result[i] = 0.5f * (Rs + Rp);
        }
        
        return result;
    }

    float FresnelDielectric(const Vector3f &V, const Vector3f &H, float eta_inv)
    {
        float cos_theta_i = glm::abs(glm::dot(V, H));
        float cos_theta_t_2 = 1.0f - eta_inv * eta_inv * (1.0f - cos_theta_i * cos_theta_i);

        if (cos_theta_t_2 <= 0.0f)
            return 1.0f;

        float cos_theta_t = std::sqrt(cos_theta_t_2);
        float Rs = (eta_inv * cos_theta_i - cos_theta_t) / (eta_inv * cos_theta_i + cos_theta_t);
        float Rp = (cos_theta_i - eta_inv * cos_theta_t) / (cos_theta_i + eta_inv * cos_theta_t);

        return (Rs * Rs + Rp * Rp) * 0.5f;
    }

    float AverageFresnelDielectric(float eta)
    {
        if (eta < 1.0f) {
            return -1.4399f * (eta * eta) + 0.7099f * eta + 0.6681f + 0.0636f / eta;
        }
        else {
            float inv_eta = 1.0f / eta;
            float inv_eta_2 = inv_eta * inv_eta;
            float inv_eta_3 = inv_eta_2 * inv_eta;
            float inv_eta_4 = inv_eta_3 * inv_eta;
            float inv_eta_5 = inv_eta_4 * inv_eta;

            return 0.919317f - 3.4793f * inv_eta + 6.75335f * inv_eta_2 - 7.80989f * inv_eta_3 + 4.98554f * inv_eta_4 - 1.36881f * inv_eta_5;
        }
    }
}