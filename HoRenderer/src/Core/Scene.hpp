/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"

//  A class that stores a list of class Hittable
class Scene : public Hittable {
public:
    Scene() {}
    Scene(std::shared_ptr<Hittable> object) { Add(object); }

    void Clean();
    void Add(std::shared_ptr<Hittable> object);

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;

private:
    std::vector<std::shared_ptr<Hittable>> hit_objects;
};