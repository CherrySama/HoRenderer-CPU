/*
    Created by Yinghao He on 2025-06-06
*/
#include "Medium.hpp"

bool HomogeneousMedium::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Hit_Payload rec1, rec2;
    Vector2f infinite_interval = Vector2f(-std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    if (!boundary->isHit(r, infinite_interval, rec1))
        return false;

    Vector2f second_interval = Vector2f(rec1.t + 0.0001f, std::numeric_limits<float>::max());
    if (!boundary->isHit(r, second_interval, rec2))
        return false;

    if (rec1.t < t_interval.x) rec1.t = t_interval.x;
    if (rec2.t > t_interval.y) rec2.t = t_interval.y;

    if (rec1.t >= rec2.t)
        return false;

    if (rec1.t < 0)
        rec1.t = 0;

    float ray_length = glm::length(r.direction());
    float distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
    
    float random_value = static_cast<float>(rand()) / RAND_MAX; 
    float hit_distance = neg_inv_density * std::log(random_value);

    if (hit_distance > distance_inside_boundary)
        return false; 

    rec.t = rec1.t + hit_distance / ray_length;
    rec.p = r.at(rec.t);

    rec.normal = Vector3f(1, 0, 0); 
    rec.front_face = true;
    rec.mat = phase_function; 
    rec.uv = Vector2f(0, 0); 
    
    return true;
}