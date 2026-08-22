/*
    Created by Yinghao He on 2025-05-18
*/
#include "Hittable.hpp"

void Hit_Payload::set_face_normal(const Ray &r, const Vector3f &outward_normal)
{
    set_face_normals(r, outward_normal, outward_normal);
}

void Hit_Payload::set_face_normals(const Ray &r,
                                   const Vector3f &outward_geometric_normal,
                                   const Vector3f &outward_shading_normal)
{
    Vector3f shading_normal = outward_shading_normal;
    if (glm::dot(shading_normal, outward_geometric_normal) < 0.0f) {
        shading_normal = -shading_normal;
    }

    front_face = glm::dot(r.direction(), outward_geometric_normal) < 0.0f;
    geometric_normal = front_face ? outward_geometric_normal : -outward_geometric_normal;
    normal = front_face ? shading_normal : -shading_normal;
}
