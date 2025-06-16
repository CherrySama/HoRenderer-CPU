/*
    Created by Yinghao He on 2025-06-06
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"

inline AABB operator+(const AABB& bbox, const Vector3f& offset) {
    return AABB(bbox.min() + offset, bbox.max() + offset);
}

inline AABB operator+(const Vector3f& offset, const AABB& bbox) {
    return bbox + offset;
}

class Translate : public Hittable {
public:
    Translate(std::shared_ptr<Hittable> object, const Vector3f& offset) : object(object), offset(offset) {
        bbox = object->getBoundingBox() + offset;
    }
    virtual bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override { return bbox; }
private:
    std::shared_ptr<Hittable> object;
    Vector3f offset;
    AABB bbox;
};

class RotateY : public Hittable {
public:
    RotateY(std::shared_ptr<Hittable> object, float angle_degrees) : object(object) {
        float radians = degrees_to_radians(angle_degrees);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = computeRotatedBoundingBox();
    }

    virtual bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override { return bbox; }
    
private:
    std::shared_ptr<Hittable> object;
    float sin_theta, cos_theta;
    AABB bbox;

    AABB computeRotatedBoundingBox();
};

class Scale : public Hittable {
public:
    Scale(std::shared_ptr<Hittable> object, float scale_factor) : object(object), scale(Vector3f(scale_factor, scale_factor, scale_factor)) {
        inv_scale = Vector3f(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
        bbox = computeScaledBoundingBox();
    }
    Scale(std::shared_ptr<Hittable> object, const Vector3f &scale_factors) : object(object), scale(scale_factors) {
        inv_scale = Vector3f((std::abs(scale.x) > Epsilon) ? (1.0f / scale.x) : 1.0f,
                             (std::abs(scale.y) > Epsilon) ? (1.0f / scale.y) : 1.0f,
                             (std::abs(scale.z) > Epsilon) ? (1.0f / scale.z) : 1.0f);
        bbox = computeScaledBoundingBox();
    }

    virtual bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override { return bbox; }
    
private:
    std::shared_ptr<Hittable> object;
    Vector3f scale;
    Vector3f inv_scale;
    AABB bbox;

    AABB computeScaledBoundingBox();
};