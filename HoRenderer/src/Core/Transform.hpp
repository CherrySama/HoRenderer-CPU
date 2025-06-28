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

enum class RotationAxis {
    X, Y, Z
};

class Rotate : public Hittable {
public:
    Rotate(std::shared_ptr<Hittable> object, RotationAxis axis, float angle_degrees) : object(object), axis(axis) {
        float radians = degrees_to_radians(angle_degrees);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = computeRotatedBoundingBox();
    }
    virtual bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override { return bbox; }

private:
    std::shared_ptr<Hittable> object;
    RotationAxis axis;
    float sin_theta, cos_theta;
    AABB bbox;

private:
    Vector3f rotateX_inverse(const Vector3f &v) const;
    Vector3f rotateX_forward(const Vector3f& v) const;
    Vector3f rotateY_inverse(const Vector3f &v) const;
    Vector3f rotateY_forward(const Vector3f& v) const;
    Vector3f rotateZ_inverse(const Vector3f &v) const;
    Vector3f rotateZ_forward(const Vector3f& v) const;
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

namespace Transform {
    inline std::shared_ptr<Translate> translate(std::shared_ptr<Hittable> object, const Vector3f& offset) {
        return std::make_shared<Translate>(object, offset);
    }
    
    inline std::shared_ptr<Rotate> rotate(std::shared_ptr<Hittable> object, RotationAxis axis, float angle_degrees) {
        return std::make_shared<Rotate>(object, axis, angle_degrees);
    }
    
    inline std::shared_ptr<Scale> scale(std::shared_ptr<Hittable> object, float scale_factor) {
        return std::make_shared<Scale>(object, scale_factor);
    }
    
    inline std::shared_ptr<Scale> scale(std::shared_ptr<Hittable> object, const Vector3f& scale_factors) {
        return std::make_shared<Scale>(object, scale_factors);
    }
}