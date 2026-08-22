/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Ray.hpp"
#include "AABB.hpp"

class Hittable;

class Hit_Payload {
public:    
    Vector3f p;
    Vector3f normal;
    Vector3f geometric_normal = Vector3f(0.0f);
    Vector3f tangent = Vector3f(0.0f);
    Vector3f bitangent = Vector3f(0.0f);
    float t;
    bool front_face;
    std::shared_ptr<Material> mat;
    Vector2f uv;
    const Hittable* hit_object = nullptr;
    int interior_medium_id = -1;  // Internal medium ID, -1 means vacuum
    int exterior_medium_id = -1;  // External medium ID, -1 means vacuum

public:
    void set_face_normal(const Ray &r, const Vector3f &outward_normal);
    void set_face_normals(const Ray &r,
                          const Vector3f &outward_geometric_normal,
                          const Vector3f &outward_shading_normal);
};

class Hittable{
public:
    virtual ~Hittable() = default;
    virtual bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const = 0;
    virtual AABB getBoundingBox() const = 0;
};
