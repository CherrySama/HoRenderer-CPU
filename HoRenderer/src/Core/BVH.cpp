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
    for (size_t object_index = start + 1; object_index < end; object_index++) {
        bbox = calculateSurroundingBox(bbox, objects[object_index]->getBoundingBox());
    }

    size_t object_span = end - start;

    if (object_span == 1) {
        // Leaf node: only one object
        left = right = objects[start];
    } else if (object_span == 2) {
        // Two objects: placed in the left and right subtrees respectively
        left = objects[start];
        right = objects[start + 1];
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

        // Recursively construct left and right subtrees
        auto mid = start + object_span / 2;
        left = std::make_shared<BVHnode>(objects, start, mid);
        right = std::make_shared<BVHnode>(objects, mid, end);
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