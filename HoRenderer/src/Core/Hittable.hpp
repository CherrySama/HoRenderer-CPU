/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Ray.hpp"

class Hit_Payload {
public:    
    Vector3f p;
    Vector3f normal;
    float t;
    bool front_face;
    std::shared_ptr<Material> mat;

public:
    void set_face_normal(const Ray &r, const Vector3f &outward_normal);
};

class Hittable{
public:
    virtual ~Hittable() = default;
    virtual bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const = 0;
};