/*
    Created by Yinghao He on 2025-06-06
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"


class HomogeneousMedium : public Hittable {
public:
    HomogeneousMedium(std::shared_ptr<Hittable> boundary, const Vector3f &sigma_s, const Vector3f &sigma_a, std::shared_ptr<Material> phase) :
        boundary(boundary), sigma_s(sigma_s), sigma_a(sigma_a), phase_function(phase), sigma_t(sigma_s + sigma_a) {}

    virtual bool isHit(const Ray& r, Vector2f t_interval, Hit_Payload& rec) const override;
    virtual AABB getBoundingBox() const override { return boundary->getBoundingBox(); }
    
private:
    std::shared_ptr<Hittable> boundary;
    Vector3f sigma_s;  // Scattering coefficient
    Vector3f sigma_a;  // Absorption coefficient
    Vector3f sigma_t;  // Extinction coefficient -> sigma_s + sigma_a
    std::shared_ptr<Material> phase_function;  
};