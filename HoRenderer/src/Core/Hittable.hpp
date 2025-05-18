/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Ray.hpp"

class Hit_Record {
public:    
    Vector3f p;
    Vector3f normal;
    float t;
    bool front_face;

public:
    void set_face_normal(const Ray &r, const Vector3f &outward_normal);
};

class Hittable{
public:
    virtual ~Hittable() = default;
    virtual bool isHit(const Ray &r, float t_min, float t_max, Hit_Record &rec) const = 0;
};