/*
    Created by Yinghao He on 2025-05-18
*/
#include "Scene.hpp"
#include "BVH.hpp"
#include "Light.hpp"
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

AliasTable2D::AliasTable2D(const std::vector<float>& weights, int w, int h) : width(w), height(h) 
{    
    rows.resize(height);
    std::vector<float> row_sums(height);
    
    for (int y = 0; y < height; y++) {
        std::vector<float> row_weights(width);
        for (int x = 0; x < width; x++) {
            row_weights[x] = weights[y * width + x];
        }
        rows[y] = AliasTable1D(row_weights);
        row_sums[y] = rows[y].Sum();
    }
    
    marginal = AliasTable1D(row_sums);
    total_sum = marginal.Sum();
}

Vector2i AliasTable2D::Sample(const Vector2f &sample, Vector2f &marginal_sample) const
{
    int y = marginal.Sample(Vector2f(sample.y, marginal_sample.y));
    
    int x = rows[y].Sample(Vector2f(sample.x, marginal_sample.x));
    
    return Vector2i(x, y);
}

float AliasTable2D::Pdf(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return 0.0f;
    
    float row_pdf = marginal.Sum() > 0 ? (rows[y].Sum() / marginal.Sum()) : 0.0f;
    float col_pdf = rows[y].Sum() > 0 ? (1.0f / width) : 0.0f; 
    
    return row_pdf * col_pdf * width * height / total_sum;
}

void Scene::Clean()
{
    hit_objects.clear();
    bvh_tree.reset();
    lights.clear();
    media.clear();
}

void Scene::Add(std::shared_ptr<Hittable> object)
{
    hit_objects.push_back(object);
}

void Scene::AddLights(std::shared_ptr<Light> light)
{
    lights.push_back(light);
    auto shape = light->GetShape();
    if (shape) 
        hit_objects.push_back(shape);
}

void Scene::AddMedium(std::shared_ptr<Medium> medium)
{
    media.push_back(medium);
}

void Scene::AddEnvLight(std::shared_ptr<InfiniteAreaLight> env_light)
{
    environment_light = env_light;
}

const std::vector<std::shared_ptr<Hittable>> Scene::GetObjects() const
{
    return hit_objects;        
}

const std::vector<std::shared_ptr<Light>> &Scene::GetLights() const
{
    return lights;
}

const std::shared_ptr<Medium> Scene::GetMedium(int medium_id) const
{
    if (medium_id >= 0 && medium_id < media.size()) 
        return media[medium_id];
    
    return nullptr; // vacuum
}

void Scene::BuildBVH()
{
    if (!hit_objects.empty()) 
        bvh_tree = std::make_shared<BVHnode>(hit_objects, 0, hit_objects.size());
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

Vector3f Scene::SampleLights(const Ray& r_in, const Hit_Payload& rec, Vector3f& light_direction, float& pdf, Sampler& sampler) const
{
    float total_power = lightTable.Sum();
    float env_power = 0.0f;
    
    if (environment_light) {
        env_power = environment_light->GetPower();
        total_power += env_power;
    }

    if (total_power <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    float env_prob = env_power / total_power;
    if (sampler.random_float() < env_prob && environment_light) {
        Vector3f radiance = environment_light->Sample(r_in, rec, light_direction, pdf, sampler);
        pdf *= env_prob;
        return radiance;
    } else if (!lights.empty()) {
        int index = lightTable.Sample(sampler.get_2d_sample());
        auto light = lights[index];

        float light_pdf = 0.0f;
        Vector3f radiance = light->Sample(r_in, rec, light_direction, light_pdf, sampler);

        float light_selection_pdf = light->GetPower() / lightTable.Sum();
        pdf = light_pdf * light_selection_pdf * (1.0f - env_prob);

        return radiance;
    }

    pdf = 0.0f;
    return Vector3f(0.0f);
}

Vector3f Scene::EvaluateLights(const Ray &light_ray, const Hit_Payload &light_rec, float &pdf) const
{
    float total_power = lightTable.Sum();
    float env_power = 0.0f;
    if (environment_light) {
        env_power = environment_light->GetPower();
        total_power += env_power;
    }
    
    if (total_power <= 0.0f) {
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
            float light_prob = light->GetPower() / lightTable.Sum();
            float env_prob = env_power / total_power;
            
            light_pdf *= light_prob * (1.0f - env_prob);
            radiance = light_radiance;
            pdf = light_pdf;
            return radiance;  
        }
    }

    if (environment_light) {
        float env_pdf = 0.0f;
        Vector3f env_radiance = environment_light->Evaluate(light_ray, light_rec, env_pdf);
        float env_prob = env_power / total_power;
        pdf = env_pdf * env_prob;
        return env_radiance;
    }
    
    return radiance;
}

Vector3f Scene::SampleEnvLight(const Ray &ray) const
{
    if (environment_light) {
        Hit_Payload dummy_hit;
        float dummy_pdf;
        return environment_light->Evaluate(ray, dummy_hit, dummy_pdf);
    }
    return Vector3f(0.05f, 0.05f, 0.05f); // default env light color
}

Vector3f Scene::EvaluateEnvLight(const Ray &ray, float &pdf) const
{
    if (environment_light) {
        float env_power = environment_light->GetPower();
        float total_power = lightTable.Sum() + env_power;
        float env_prob = total_power > 0.0f ? env_power / total_power : 1.0f;
        Hit_Payload dummy_hit;
        Vector3f color = environment_light->Evaluate(ray, dummy_hit, pdf);
        pdf *= env_prob;
        return color;
    }
    pdf = 0.0f;
    return Vector3f(0.05f, 0.05f, 0.05f); // default env light color
}

// Get the medium ID where the ray is currently located
int Scene::GetCurrentMediumId(const Ray &ray, const Hit_Payload *last_hit) const
{
    // TODO: Judging by scene geometry and medium boundaries
    
    // If there is the last intersection information,
    // determine the medium conversion based on the surface normal
    if (last_hit) {
        bool entering = glm::dot(ray.direction(), last_hit->normal) < 0;
        if (entering) {
            return last_hit->interior_medium_id;
        } else {
            return last_hit->exterior_medium_id;
        }
    }

    return -1;
}

// Update the medium ID
int Scene::UpdateMediumId(const Ray &ray, const Hit_Payload &hit, int current_medium_id) const
{
    if (hit.interior_medium_id != hit.exterior_medium_id) {
        // Medium Boundary Transition
        if (glm::dot(ray.direction(), hit.normal) > 0) {
            // Rays emitted from the inside
            return hit.exterior_medium_id;
        } else {
            // Rays entering from outside
            return hit.interior_medium_id;
        }
    }
    // No media conversion
    return current_medium_id;
}