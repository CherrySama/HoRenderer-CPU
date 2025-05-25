/*
    Created by Yinghao He on 2025-05-25
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"


class BVHnode : public Hittable {
public:
    BVHnode() {}
    BVHnode(const Scene &scene);
    BVHnode(std::vector<std::shared_ptr<Hittable>> &objects, size_t start, size_t end);

    bool isHit(const Ray& r, Vector2f t_interval, Hit_Payload& rec) const override;
    AABB getBoundingBox() const override { return bbox; }
    
private:
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;
    AABB bbox;

    // compare function
    static bool box_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b, int axis_index);
    static bool box_x_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b);
    static bool box_y_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b);
    static bool box_z_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b);
};