/*
    Created by Yinghao He on 2025-06-06
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"
#include "PhaseFunction.hpp"

class HomogeneousMedium : public Hittable {
public:
    HomogeneousMedium(std::shared_ptr<Hittable> boundary, float density, std::shared_ptr<Texture> tex) :
        boundary(boundary), neg_inv_density(-1 / density),
        phase_function(std::make_shared<IsotropicPhase>(tex)) {}

    HomogeneousMedium(std::shared_ptr<Hittable> boundary, float density, const Vector3f &albedo) :
        boundary(boundary), neg_inv_density(-1 / density),
        phase_function(std::make_shared<IsotropicPhase>(albedo)) {
    }

    virtual bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override { return boundary->getBoundingBox(); }
private:
    std::shared_ptr<Hittable> boundary;
    float neg_inv_density;
    std::shared_ptr<Material> phase_function;
};