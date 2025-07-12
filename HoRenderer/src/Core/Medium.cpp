/*
    Created by Yinghao He on 2025-06-06
*/
#include "Medium.hpp"

bool HomogeneousMedium::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Hit_Payload rec1, rec2;
    Vector2f infinite_interval = Vector2f(-Infinity, Infinity);
    if (!boundary->isHit(r, infinite_interval, rec1))
        return false;

    Vector2f second_interval = Vector2f(rec1.t + 0.0001f, Infinity);
    if (!boundary->isHit(r, second_interval, rec2))
        return false;

    if (rec1.t < t_interval.x) rec1.t = t_interval.x;
    if (rec2.t > t_interval.y) rec2.t = t_interval.y;

    if (rec1.t >= rec2.t)
        return false;

    if (rec1.t < 0)
        rec1.t = 0;

    float distance_inside_boundary = rec2.t - rec1.t;

    // luminance weight
    float avg_sigma_t = 0.299f * sigma_t.x + 0.587f * sigma_t.y + 0.114f * sigma_t.z;
    uint32_t seed = hash_ray(r.origin(), r.direction());
    seed = seed * 1664525u + 1013904223u;
    float random_sample = (seed & 0xFFFFFFFF) * (1.0f / 4294967296.0f);
    float hit_distance = -std::log(std::max(random_sample, 1e-8f)) / avg_sigma_t;

    if (hit_distance > distance_inside_boundary)
        return false; 

    rec.t = rec1.t + hit_distance;
    rec.p = r.at(rec.t);

    rec.normal = Vector3f(1, 0, 0); 
    rec.front_face = true;
    rec.mat = phase_function; 
    rec.uv = Vector2f(0, 0); 
    
    return true;
}

