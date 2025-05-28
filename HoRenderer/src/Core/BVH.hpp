/*
    Created by Yinghao He on 2025-05-25
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"

struct SplitResult {
    size_t split_index;
    float cost;
    AABB left_bbox;
    AABB right_bbox;
};

enum class SplitStrategy { SIMPLE_MIDDLE,
                           ACCUMULATED };

class BVHnode : public Hittable {
public:
    BVHnode() {}
    BVHnode(const Scene &scene);
    BVHnode(std::vector<std::shared_ptr<Hittable>> &objects, size_t start, size_t end);

    bool isHit(const Ray& r, Vector2f t_interval, Hit_Payload& rec) const override;
    AABB getBoundingBox() const override { return bbox; }
    
private:
    // Calculate the surface area of ​​the bounding box
    float calculateSurfaceArea(const AABB &bbox);
    
    // Calculating SAH costs
    float computeSAHCost(size_t N_left, const AABB& box_left, size_t N_right, const AABB& box_right, const AABB& box_parent);
    
    // compare function
    static bool box_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b, int axis_index);
    static bool box_x_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b);
    static bool box_y_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b);
    static bool box_z_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b);

    SplitResult FindOptimalSplitAccumulated(std::vector<std::shared_ptr<Hittable>> &objects, size_t start, size_t end, const AABB &parent_bbox, int axis);

private:
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;
    AABB bbox;
};