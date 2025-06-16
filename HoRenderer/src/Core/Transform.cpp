/*
    Created by Yinghao He on 2025-06-06
*/
#include "Transform.hpp"

bool Translate::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Ray offset_r(r.origin() - offset, r.direction());

    if (!object->isHit(offset_r, t_interval, rec))
        return false;

    rec.p += offset;

    return true;
}

bool RotateY::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Vector3f origin = Vector3f(cos_theta * r.origin().x + sin_theta * r.origin().z,
                               r.origin().y,
                               -sin_theta * r.origin().x + cos_theta * r.origin().z);

    Vector3f direction = Vector3f(cos_theta * r.direction().x + sin_theta * r.direction().z,
                                  r.direction().y,
                                  -sin_theta * r.direction().x + cos_theta * r.direction().z);

    Ray rotated_ray(origin, direction);

    if (!object->isHit(rotated_ray, t_interval, rec))
        return false;

    Vector3f rotated_point = Vector3f(cos_theta * rec.p.x - sin_theta * rec.p.z,
                                      rec.p.y,
                                      sin_theta * rec.p.x + cos_theta * rec.p.z);

    Vector3f rotated_normal = Vector3f(cos_theta * rec.normal.x - sin_theta * rec.normal.z,
                                       rec.normal.y,
                                       sin_theta * rec.normal.x + cos_theta * rec.normal.z);

    rec.p = rotated_point;
    rec.normal = rotated_normal;

    return true;
}

AABB RotateY::computeRotatedBoundingBox() {
    AABB original_bbox = object->getBoundingBox();
    Vector3f min_point = original_bbox.min();
    Vector3f max_point = original_bbox.max();

    Vector3f min_rotated(Infinity, Infinity, Infinity);
    Vector3f max_rotated(-Infinity, -Infinity, -Infinity);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                float x = (i == 0) ? min_point.x : max_point.x;
                float y = (j == 0) ? min_point.y : max_point.y;
                float z = (k == 0) ? min_point.z : max_point.z;

                float rotated_x = cos_theta * x - sin_theta * z;
                float rotated_y = y;
                float rotated_z = sin_theta * x + cos_theta * z;

                min_rotated.x = std::min(min_rotated.x, rotated_x);
                min_rotated.y = std::min(min_rotated.y, rotated_y);
                min_rotated.z = std::min(min_rotated.z, rotated_z);

                max_rotated.x = std::max(max_rotated.x, rotated_x);
                max_rotated.y = std::max(max_rotated.y, rotated_y);
                max_rotated.z = std::max(max_rotated.z, rotated_z);
            }
        }
    }

    return AABB(min_rotated, max_rotated);
}

bool Scale::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Vector3f scaled_origin = Vector3f(r.origin().x * inv_scale.x,
                                      r.origin().y * inv_scale.y,
                                      r.origin().z * inv_scale.z);

    Vector3f scaled_direction = Vector3f(r.direction().x * inv_scale.x,
                                         r.direction().y * inv_scale.y,
                                         r.direction().z * inv_scale.z);

    Ray scaled_ray(scaled_origin, scaled_direction);

    if (!object->isHit(scaled_ray, t_interval, rec))
        return false;

    Vector3f world_point = Vector3f(
        rec.p.x * scale.x,
        rec.p.y * scale.y,
        rec.p.z * scale.z);

    Vector3f world_normal = Vector3f(
        rec.normal.x * inv_scale.x,
        rec.normal.y * inv_scale.y,
        rec.normal.z * inv_scale.z);

    world_normal = glm::normalize(world_normal);

    float direction_scale = glm::length(scaled_direction) / glm::length(r.direction());
    rec.t = rec.t / direction_scale;

    rec.p = world_point;
    rec.normal = world_normal;

    return true;
}

AABB Scale::computeScaledBoundingBox() {
    AABB original_bbox = object->getBoundingBox();
    Vector3f min_point = original_bbox.min();
    Vector3f max_point = original_bbox.max();

    Vector3f scaled_min = Vector3f(min_point.x * scale.x,
                                   min_point.y * scale.y,
                                   min_point.z * scale.z);

    Vector3f scaled_max = Vector3f(max_point.x * scale.x,
                                   max_point.y * scale.y,
                                   max_point.z * scale.z);

    Vector3f final_min = Vector3f(std::min(scaled_min.x, scaled_max.x),
                                  std::min(scaled_min.y, scaled_max.y),
                                  std::min(scaled_min.z, scaled_max.z));

    Vector3f final_max = Vector3f(std::max(scaled_min.x, scaled_max.x),
                                  std::max(scaled_min.y, scaled_max.y),
                                  std::max(scaled_min.z, scaled_max.z));

    return AABB(final_min, final_max);
}