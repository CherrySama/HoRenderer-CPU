/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"

//  A class that stores a list of class Hittable
class HittableList : public Hittable {
public:
    HittableList() {}
    HittableList(std::shared_ptr<Hittable> object) { Add(object); }

    void Clean();
    void Add(std::shared_ptr<Hittable> object);

    bool isHit(const Ray &r, float t_min, float t_max, Hit_Record &rec) const override;

private:
    std::vector<std::shared_ptr<Hittable>> hit_objects;
};