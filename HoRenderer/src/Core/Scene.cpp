/*
    Created by Yinghao He on 2025-05-18
*/
#include "Scene.hpp"
#include "BVH.hpp"


void Scene::Clean()
{
    hit_objects.clear();
    bvh_tree.reset();
}

void Scene::Add(std::shared_ptr<Hittable> object)
{
    hit_objects.push_back(object);
}

const std::vector<std::shared_ptr<Hittable>> Scene::GetObjects() const
{
    return hit_objects;        
}

void Scene::BuildBVH()
{
    if (!hit_objects.empty()) 
    {
        // std::cout << "Start to build BVH tree..." << std::endl;
        bvh_tree = std::make_shared<BVHnode>(hit_objects, 0, hit_objects.size());
        // auto end_time = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        // std::cout << "BVH build: " << hit_objects.size() << " objects, Time: " << duration.count() << "ms" << std::endl;
    }
}

bool Scene::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const
{
    // If we have a BVH tree, then use BVH acceleration
    if (bvh_tree) {
        return bvh_tree->isHit(r, t_interval, rec);
    }
    
    Hit_Payload temp_rec;
    bool isHit = false;
    auto closest_t = t_interval.y;

    for (const auto &object:hit_objects) {
        if (object->isHit(r, Vector2f(t_interval.x, closest_t), temp_rec))
        {
            isHit = true;
            closest_t = temp_rec.t;
            rec = temp_rec;
        }
    }

    return isHit;
}

AABB Scene::getBoundingBox() const
{
    if (hit_objects.empty()) return AABB();

    AABB output_box = hit_objects[0]->getBoundingBox();
    for (size_t i = 1; i < hit_objects.size(); i++) {
        output_box = calculateSurroundingBox(output_box, hit_objects[i]->getBoundingBox());
    }
    return output_box;
}