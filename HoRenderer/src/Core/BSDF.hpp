/*
    Created by Yinghao He on 2025-06-23
*/
#pragma once

#include "Util.hpp"

namespace BSDF {
	float DistributionGGX(const Vector3f &H, const Vector3f &N, float alpha_u, float alpha_v);

    float DistributionGTR1(const Vector3f &H, const Vector3f &N, float alpha);

    float GeometrySmithG1(const Vector3f &V, const Vector3f &H, const Vector3f &N, float alpha_u, float alpha_v);

    Vector3f SchlickFresnel(const Vector3f &F0, float cosTheta);

    Vector3f ComputeF0(const Vector3f &base_color, float metallic, float specular = 0.5f);

    Vector3f FresnelConductor(const Vector3f &V, const Vector3f &H, const Vector3f &eta, const Vector3f &k);

    float FresnelDielectric(const Vector3f &V, const Vector3f &H, float eta_inv);

    float AverageFresnelDielectric(float eta);

    Vector3f MultipleScatteringCompensation(const Vector3f& albedo, float roughness, float F_avg);
}