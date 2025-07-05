/*
    Created by Yinghao He on 2025-05-18
*/
#include "Scene.hpp"
#include "BVH.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "Sampler.hpp"


AliasTable1D::AliasTable1D(const std::vector<float>& distrib) {
	std::queue<Element> greater, lesser;

	sumDistrib = 0.0f;
	for (auto i : distrib) {
		sumDistrib += i;
	}

	for (int i = 0; i < distrib.size(); i++) {
		float scaledPdf = distrib[i] * distrib.size();
		(scaledPdf >= sumDistrib ? greater : lesser).push(Element(i, scaledPdf));
	}

	table.resize(distrib.size(), Element(-1, 0.0f));

	while (!greater.empty() && !lesser.empty()) {
		auto [l, pl] = lesser.front();
		lesser.pop();
		auto [g, pg] = greater.front();
		greater.pop();

		table[l] = Element(g, pl);

		pg += pl - sumDistrib;
		(pg < sumDistrib ? lesser : greater).push(Element(g, pg));
	}

	while (!greater.empty()) {
		auto [g, pg] = greater.front();
		greater.pop();
		table[g] = Element(g, pg);
	}

	while (!lesser.empty()) {
		auto [l, pl] = lesser.front();
		lesser.pop();
		table[l] = Element(l, pl);
	}
}

int AliasTable1D::Sample(const Vector2f& sample) const {
	int rx = sample.x * table.size();
	if (rx == table.size()) {
		rx--;
	}
	float ry = sample.y;

	return (ry <= table[rx].second / sumDistrib) ? rx : table[rx].first;
}

void Scene::Clean()
{
    hit_objects.clear();
    bvh_tree.reset();
    lights.clear();
}

void Scene::Add(std::shared_ptr<Hittable> object)
{
    hit_objects.push_back(object);
}

void Scene::Add(std::shared_ptr<Light> light)
{
    lights.push_back(light);
    auto shape = light->GetShape();
    if (shape) 
        hit_objects.push_back(shape);
}

const std::vector<std::shared_ptr<Hittable>> Scene::GetObjects() const
{
    return hit_objects;        
}

const std::vector<std::shared_ptr<Light>>& Scene::GetLights() const {
    return lights;
}

void Scene::BuildBVH()
{
    if (!hit_objects.empty()) 
    {
        // auto start_time = std::chrono::high_resolution_clock::now();
        bvh_tree = std::make_shared<BVHnode>(hit_objects, 0, hit_objects.size());
        // auto end_time = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        // std::cout << "BVH build: " << hit_objects.size() << " objects, Time: " << duration.count() << "ms" << std::endl;
    }
}

void Scene::BuildLightTable()
{
    if (lights.empty()) return;
    
    std::vector<float> power(lights.size());
    for (int i = 0; i < lights.size(); i++) {
        auto light = lights[i];
        power[i] = light->GetPower();
    }
    lightTable = AliasTable1D(power);
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

Vector3f Scene::SampleLightEnvironment(const Ray& r_in, const Hit_Payload& rec, Vector3f& light_direction, float& pdf, Sampler& sampler) const
{
    if (lights.empty()) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    int index = lightTable.Sample(sampler.get_2d_sample());
    auto light = lights[index];

    float light_pdf = 0.0f;
    Vector3f radiance = light->Sample(r_in, rec, light_direction, light_pdf, sampler);

    pdf = light_pdf * (light->GetPower() / lightTable.Sum());

    Ray shadow_ray = Ray::SpawnRay(rec.p, light_direction, rec.normal);
    Hit_Payload shadow_rec;
    if (isHit(shadow_ray, Vector2f(Epsilon, Infinity), shadow_rec)) {
        if (!shadow_rec.mat || !shadow_rec.mat->IsEmit()) {
            return Vector3f(0.0f);
        }
    }
    
    return radiance;
}

Vector3f Scene::EvaluateLight(const Ray &light_ray, const Hit_Payload &light_rec, float &pdf) const
{
    if (lights.empty()) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f radiance(0.0f);
    pdf = 0.0f;

    for (int i = 0; i < lights.size(); i++) {
        auto light = lights[i];
        float light_pdf = 0.0f;

        Vector3f light_radiance = light->Evaluate(light_ray, light_rec, light_pdf);
        
        if (light_pdf > 0.0f) {
            light_pdf *= (light->GetPower() / lightTable.Sum());
            radiance = light_radiance;
            pdf = light_pdf;
            break;
        }
    }
    
    return radiance;
}