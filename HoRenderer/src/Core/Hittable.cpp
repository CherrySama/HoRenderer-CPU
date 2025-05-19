/*
    Created by Yinghao He on 2025-05-18
*/
#include "Hittable.hpp"

void Hit_Payload::set_face_normal(const Ray &r, const Vector3f &outward_normal)
{
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.
        // make sure outward_normal is unit vector
        // Vector3f normalized_normal = glm::normalize(outward_normal);
        front_face = dot(r.direction(), outward_normal) < 0.0f;
        normal = front_face ? outward_normal : -outward_normal;
}
