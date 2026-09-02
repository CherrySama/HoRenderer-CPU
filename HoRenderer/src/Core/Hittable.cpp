/*
    Created by Yinghao He on 2025-05-18
*/
#include "Hittable.hpp"

namespace {

bool IsFiniteNonZeroNormal(const Vector3f& normal)
{
    return std::isfinite(normal.x) &&
           std::isfinite(normal.y) &&
           std::isfinite(normal.z) &&
           glm::length2(normal) > Epsilon * Epsilon;
}

Vector3f NormalizeOrFallback(const Vector3f& normal, const Vector3f& fallback)
{
    return IsFiniteNonZeroNormal(normal) ? glm::normalize(normal) : fallback;
}

} // namespace

void Hit_Payload::set_face_normal(const Ray &r, const Vector3f &outward_normal)
{
    set_face_normals(r, outward_normal, outward_normal);
}

void Hit_Payload::set_face_normals(const Ray &r,
                                   const Vector3f &outward_geometric_normal,
                                   const Vector3f &outward_shading_normal)
{
    const Vector3f fallback_normal(0.0f, 0.0f, 1.0f);
    const Vector3f geometric_outward =
        NormalizeOrFallback(outward_geometric_normal, fallback_normal);
    Vector3f shading_normal =
        NormalizeOrFallback(outward_shading_normal, geometric_outward);
    if (glm::dot(shading_normal, geometric_outward) < 0.0f) {
        shading_normal = -shading_normal;
    }

    front_face = glm::dot(r.direction(), geometric_outward) < 0.0f;
    geometric_normal = front_face ? geometric_outward : -geometric_outward;
    normal = front_face ? shading_normal : -shading_normal;
}
