/*
    Created by Yinghao He on 2025-06-23
*/
#pragma once

#include "Util.hpp"

namespace BSDF {
	float DistributionGGX(const Vector3f &H, const Vector3f &N, float alpha_u, float alpha_v);

	float GeometrySchlickGGX(const Vector3f &V, const Vector3f &N, float alpha_u, float alpha_v);

	float GeometrySmithGGX(const Vector3f &V, const Vector3f &L, const Vector3f &N, float alpha_u, float alpha_v);

    Vector3f FresnelConductor(const Vector3f &V, const Vector3f &H, const Vector3f &eta, const Vector3f &k);

    float FresnelDielectric(const Vector3f &V, const Vector3f &H, float eta_inv);
}