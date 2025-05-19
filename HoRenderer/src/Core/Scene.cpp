/*
    Created by Yinghao He on 2025-05-18
*/
#include "Scene.hpp"

void Scene::Clean()
{
    hit_objects.clear();
}

void Scene::Add(std::shared_ptr<Hittable> object)
{
    hit_objects.push_back(object);
}

bool Scene::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const
{
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