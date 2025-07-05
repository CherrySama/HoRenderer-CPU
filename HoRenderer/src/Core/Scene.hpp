/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"


class AliasTable1D {
public:
	AliasTable1D() = default;
	AliasTable1D(const std::vector<float>& distrib);

	int Sample(const Vector2f& sample) const;
	inline float Sum() const { return sumDistrib; }
	inline std::vector<std::pair<int, float>> GetTable() const { return table; }

private:
    typedef std::pair<int, float> Element;
    std::vector<Element> table;
	float sumDistrib;
};

//  A class that stores a list of class Hittable
class Scene : public Hittable {
public:
    Scene() {}
    Scene(std::shared_ptr<Hittable> object) { Add(std::move(object)); }

    void Clean();
    void Add(std::shared_ptr<Hittable> object);
    void Add(std::shared_ptr<Light> light);
    const std::vector<std::shared_ptr<Hittable>> GetObjects() const;
    const std::vector<std::shared_ptr<Light>>& GetLights() const;

    void BuildBVH();
    void BuildLightTable();
    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override;

    Vector3f SampleLightEnvironment(const Ray& r_in, const Hit_Payload& rec, Vector3f& light_direction, float& pdf, Sampler& sampler) const;
    Vector3f EvaluateLight(const Ray& light_ray, const Hit_Payload& light_rec, float& pdf) const;
    
private:
    std::vector<std::shared_ptr<Hittable>> hit_objects;
    std::shared_ptr<BVHnode> bvh_tree;
    std::vector<std::shared_ptr<Light>> lights;
    AliasTable1D lightTable;
};