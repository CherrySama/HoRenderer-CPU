/*
    Created by Yinghao He on 2025-05-18
*/
#include "HittableList.hpp"

void HittableList::Clean()
{
    hit_objects.clear();
}

void HittableList::Add(std::shared_ptr<Hittable> object)
{
    hit_objects.push_back(object);
}

bool HittableList::isHit(const Ray &r, float t_min, float t_max, Hit_Record &rec) const
{
    Hit_Record temp_rec;
    bool isHit = false;
    auto closest_t = t_max;

    for (const auto &object:hit_objects) {
        if (object->isHit(r, t_min, closest_t, temp_rec))
        {
            isHit = true;
            closest_t = temp_rec.t;
            rec = temp_rec;
        }
    }

    return isHit;
}