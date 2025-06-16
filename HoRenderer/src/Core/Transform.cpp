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

bool Rotate::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Vector3f origin, direction;

    switch (axis) {
    case RotationAxis::X:
        origin = rotateX_inverse(r.origin());
        direction = rotateX_inverse(r.direction());
        break;
    case RotationAxis::Y:
        origin = rotateY_inverse(r.origin());
        direction = rotateY_inverse(r.direction());
        break;
    case RotationAxis::Z:
        origin = rotateZ_inverse(r.origin());
        direction = rotateZ_inverse(r.direction());
        break;
    }

    Ray rotated_ray(origin, direction);
    if (!object->isHit(rotated_ray, t_interval, rec))
        return false;

    switch (axis) {
    case RotationAxis::X:
        rec.p = rotateX_forward(rec.p);
        rec.normal = rotateX_forward(rec.normal);
        break;
    case RotationAxis::Y:
        rec.p = rotateY_forward(rec.p);
        rec.normal = rotateY_forward(rec.normal);
        break;
    case RotationAxis::Z:
        rec.p = rotateZ_forward(rec.p);
        rec.normal = rotateZ_forward(rec.normal);
        break;
    }

    return true;
}

Vector3f Rotate::rotateX_inverse(const Vector3f &v) const {
    return Vector3f(v.x,
                    cos_theta * v.y + sin_theta * v.z,
                    -sin_theta * v.y + cos_theta * v.z);
}

Vector3f Rotate::rotateX_forward(const Vector3f &v) const {
    return Vector3f(v.x,
                    cos_theta * v.y - sin_theta * v.z,
                    sin_theta * v.y + cos_theta * v.z);
}

Vector3f Rotate::rotateY_inverse(const Vector3f &v) const {
    return Vector3f(cos_theta * v.x + sin_theta * v.z,
                    v.y,
                    -sin_theta * v.x + cos_theta * v.z);
}

Vector3f Rotate::rotateY_forward(const Vector3f &v) const {
    return Vector3f(cos_theta * v.x - sin_theta * v.z,
                    v.y,
                    sin_theta * v.x + cos_theta * v.z);
}

Vector3f Rotate::rotateZ_inverse(const Vector3f &v) const {
    return Vector3f(cos_theta * v.x + sin_theta * v.y,
                    -sin_theta * v.x + cos_theta * v.y,
                    v.z);
}

Vector3f Rotate::rotateZ_forward(const Vector3f &v) const {
    return Vector3f(cos_theta * v.x - sin_theta * v.y,
                    sin_theta * v.x + cos_theta * v.y,
                    v.z);
}

AABB Rotate::computeRotatedBoundingBox() {
    AABB original_bbox = object->getBoundingBox();
    Vector3f min_point = original_bbox.min();
    Vector3f max_point = original_bbox.max();

    Vector3f min_rotated(Infinity, Infinity, Infinity);
    Vector3f max_rotated(-Infinity, -Infinity, -Infinity);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                Vector3f vertex((i == 0) ? min_point.x : max_point.x,
                                (j == 0) ? min_point.y : max_point.y,
                                (k == 0) ? min_point.z : max_point.z);

                Vector3f rotated_vertex;
                switch (axis) {
                case RotationAxis::X:
                    rotated_vertex = rotateX_forward(vertex);
                    break;
                case RotationAxis::Y:
                    rotated_vertex = rotateY_forward(vertex);
                    break;
                case RotationAxis::Z:
                    rotated_vertex = rotateZ_forward(vertex);
                    break;
                }

                min_rotated.x = std::min(min_rotated.x, rotated_vertex.x);
                min_rotated.y = std::min(min_rotated.y, rotated_vertex.y);
                min_rotated.z = std::min(min_rotated.z, rotated_vertex.z);

                max_rotated.x = std::max(max_rotated.x, rotated_vertex.x);
                max_rotated.y = std::max(max_rotated.y, rotated_vertex.y);
                max_rotated.z = std::max(max_rotated.z, rotated_vertex.z);
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

    Vector3f world_point = Vector3f(rec.p.x * scale.x,
                                    rec.p.y * scale.y,
                                    rec.p.z * scale.z);

    Vector3f world_normal = Vector3f(rec.normal.x * inv_scale.x,
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