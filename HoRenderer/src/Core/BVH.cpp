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
    // Construct the bounding box of the current node
    bbox = objects[start]->getBoundingBox();
    for (size_t object_index = start + 1; object_index < end; object_index++) 
        bbox = calculateSurroundingBox(bbox, objects[object_index]->getBoundingBox());
    

    size_t object_span = end - start;

    if (object_span == 1) {
        // Leaf node: only one object
        left = right = objects[start];
    } else if (object_span == 2) {
        // Two objects: placed in the left and right subtrees respectively
        left = objects[start];
        right = objects[start + 1];
    } else if (object_span <= 16) {
        int axis = 0;
        Vector3f extent = bbox.max() - bbox.min();
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent[axis]) axis = 2;

        // Choose comparison function based on axis
        auto comparator = (axis == 0) ? box_x_compare : (axis == 1) ? box_y_compare :
                                                                      box_z_compare;

        std::sort(objects.begin() + start, objects.begin() + end, comparator);

        // 简单中点分割
        auto mid = start + object_span / 2;

        #pragma omp task shared(left)
            left = std::make_shared<BVHnode>(objects, start, mid);

        #pragma omp task shared(right)
            right = std::make_shared<BVHnode>(objects, mid, end);

        #pragma omp taskwait
    } else {
        // Multiple objects: segmentation required
        
        // Select the longest axis to split
        int axis = 0;
        Vector3f extent = bbox.max() - bbox.min();
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent[axis]) axis = 2;

        // Choose comparison function based on axis
        auto comparator = (axis == 0) ? box_x_compare
                        : (axis == 1) ? box_y_compare
                                      : box_z_compare;

        // Sorting
        std::sort(objects.begin() + start, objects.begin() + end, comparator);

        // SAH Segmentation
        float min_cost = Infinity;
        size_t best_split = start + 1;

        // Calculate the bounding box of all objects
        std::vector<AABB> boxes(object_span);
        for (size_t i = start; i < end; ++i) {
            boxes[i - start] = objects[i]->getBoundingBox();
        }

        // To find the best split point
        AABB left_box = boxes[0];
        for (size_t i = 1; i < object_span - 1; ++i) {
            left_box = calculateSurroundingBox(left_box, boxes[i]);
            size_t N_left = i + 1;
            size_t N_right = object_span - N_left;

            // 计算右子树包围盒
            AABB right_box = boxes[i + 1];
            for (size_t j = i + 2; j < object_span; ++j) {
                right_box = calculateSurroundingBox(right_box, boxes[j]);
            }

            float cost = computeSAHCost(N_left, left_box, N_right, right_box, bbox);
            if (cost < min_cost) {
                min_cost = cost;
                best_split = start + i + 1;
            }
        }

        #pragma omp task shared(left)
        left = std::make_shared<BVHnode>(objects, start, best_split);

        #pragma omp task shared(right)
        right = std::make_shared<BVHnode>(objects, best_split, end);

        #pragma omp taskwait
    }
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
    const float C_t = 1.0f;  // 遍历成本
    const float C_i = 1.0f;  // 相交测试成本
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