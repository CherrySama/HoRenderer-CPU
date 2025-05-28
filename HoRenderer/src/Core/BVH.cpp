/*
    Created by Yinghao He on 2025-05-25
*/
#include "BVH.hpp"
#include "Scene.hpp"

// Constructing BVH from Scene
BVHnode::BVHnode(const Scene& scene) {
    auto objects = scene.GetObjects(); 
    *this = BVHnode(objects, 0, objects.size());
}

BVHnode::BVHnode(std::vector<std::shared_ptr<Hittable>>& objects, size_t start, size_t end) {
    // Build the bounding box of the current node
    bbox = objects[start]->getBoundingBox();
    for (size_t i = start + 1; i < end; i++) 
        bbox = calculateSurroundingBox(bbox, objects[i]->getBoundingBox());
    size_t object_span = end - start;

    if (object_span == 1) {
        left = right = objects[start];
        return;
    } else if (object_span == 2) {
        left = objects[start];
        right = objects[start + 1];
        return;
    }

    // Select the longest axis to split
    Vector3f extent = bbox.max() - bbox.min();
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    SplitStrategy strategy;
    if (object_span <= 4)
        strategy = SplitStrategy::SIMPLE_MIDDLE;
    else
        strategy = SplitStrategy::ACCUMULATED;

    // sorting
    auto comparator = (axis == 0) ? box_x_compare : (axis == 1) ? box_y_compare :
                                                                  box_z_compare;
    std::sort(objects.begin() + start, objects.begin() + end, comparator);

    size_t split_point;
    switch (strategy) {
        case SplitStrategy::SIMPLE_MIDDLE: {
            split_point = start + object_span / 2;
            break;
        }
        case SplitStrategy::ACCUMULATED: {
            auto result = FindOptimalSplitAccumulated(objects, start, end, bbox, axis);
            split_point = result.split_index;
            break;
        }
    }

    #pragma omp task shared(left) 
    left = std::make_shared<BVHnode>(objects, start, split_point);
    
    #pragma omp task shared(right)  
    right = std::make_shared<BVHnode>(objects, split_point, end);
    
    #pragma omp taskwait
}

bool BVHnode::isHit(const Ray& r, Vector2f t_interval, Hit_Payload& rec) const {
    // First detect the bounding box
    Vector2f temp_interval = t_interval;
    if (!bbox.isHit(r, temp_interval))
        return false;

    // Check left and right subtrees
    bool hit_left = left->isHit(r, t_interval, rec);
    Vector2f right_interval = Vector2f(t_interval.x, hit_left ? rec.t : t_interval.y);
    bool hit_right = right->isHit(r, right_interval, rec);

    return hit_left || hit_right;
}

float BVHnode::calculateSurfaceArea(const AABB &bbox)
{
    Vector3f extent = bbox.max() - bbox.min();
    return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
}

float BVHnode::computeSAHCost(size_t N_left, const AABB &box_left, size_t N_right, const AABB &box_right, const AABB &box_parent)
{
    const float C_t = 1.0f;  // Traversal Cost
    const float C_i = 1.0f;  // Intersection test cost
    float SA_parent = calculateSurfaceArea(box_parent);
    float SA_left = calculateSurfaceArea(box_left);
    float SA_right = calculateSurfaceArea(box_right);
    float cost = C_t + (SA_left / SA_parent) * N_left * C_i + (SA_right / SA_parent) * N_right * C_i;
    return cost;
}

bool BVHnode::box_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b, int axis_index) {
    AABB box_a = a->getBoundingBox();
    AABB box_b = b->getBoundingBox();
    
    if (axis_index == 0) return box_a.min().x < box_b.min().x;
    if (axis_index == 1) return box_a.min().y < box_b.min().y;
    return box_a.min().z < box_b.min().z;
}

bool BVHnode::box_x_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b) {
    return box_compare(a, b, 0);
}

bool BVHnode::box_y_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b) {
    return box_compare(a, b, 1);
}

bool BVHnode::box_z_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b) {
    return box_compare(a, b, 2);
}

SplitResult BVHnode::FindOptimalSplitAccumulated(std::vector<std::shared_ptr<Hittable>> &objects, size_t start, size_t end, const AABB &parent_bbox, int axis)
{
    size_t object_span = end - start;
    
    // Precompute bounding boxes for all objects
    std::vector<AABB> primitive_boxes(object_span);
    for (size_t i = 0; i < object_span; ++i) 
        primitive_boxes[i] = objects[start + i]->getBoundingBox();
    
    // Cumulative bounding boxes from left to right
    std::vector<AABB> left_boxes(object_span - 1);
    left_boxes[0] = primitive_boxes[0];
    for (size_t i = 1; i < object_span - 1; ++i) 
        left_boxes[i] = calculateSurroundingBox(left_boxes[i-1], primitive_boxes[i]);
    
    // Accumulate bounding boxes from right to left
    std::vector<AABB> right_boxes(object_span - 1);
    right_boxes[object_span - 2] = primitive_boxes[object_span - 1];
    for (int i = object_span - 3; i >= 0; --i) 
        right_boxes[i] = calculateSurroundingBox(primitive_boxes[i + 1], right_boxes[i + 1]);
    
    // Finding the optimal split point
    float min_cost = std::numeric_limits<float>::max();
    size_t best_split = start + 1;
    AABB best_left_bbox, best_right_bbox;
    
    for (size_t i = 0; i < object_span - 1; ++i) {
        size_t N_left = i + 1;
        size_t N_right = object_span - N_left;
        
        const AABB& left_bbox = left_boxes[i];      
        const AABB& right_bbox = right_boxes[i];    
        
        float cost = computeSAHCost(N_left, left_bbox, N_right, right_bbox, parent_bbox);
        
        if (cost < min_cost) {
            min_cost = cost;
            best_split = start + N_left;
            best_left_bbox = left_bbox;
            best_right_bbox = right_bbox;
        }
    }

    if (best_split <= start || best_split >= end) 
        best_split = start + object_span / 2;  
    
    return {best_split, min_cost, best_left_bbox, best_right_bbox};
}