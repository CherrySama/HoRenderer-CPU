/*
    Created by Yinghao He on 2025-06-22
*/
#include "Light.hpp"
#include "Sampler.hpp"


LightSampleInfo QuadAreaLight::Sample(const Vector3f& surface_pos, Sampler& sampler) const {
    LightSampleInfo result;

    Vector3f light_point = SampleQuadSurface(sampler);
    Vector3f light_to_surface = light_point - surface_pos;
    float distance_sq = glm::dot(light_to_surface, light_to_surface);
    
    if (distance_sq < Epsilon) {
        result.valid = false;
        return result;
    }
    
    result.distance = std::sqrt(distance_sq);
    result.direction = light_to_surface / result.distance;
    
    Vector3f light_normal = GetQuadNormal();
    float cos_theta = glm::dot(-result.direction, light_normal);
    
    if (cos_theta <= 0.0f) {
        result.valid = false;
        return result;
    }

    result.pdf = distance_sq / (area * cos_theta);
    result.radiance = color * intensity;
    result.valid = true;
    
    return result;
}

Vector3f QuadAreaLight::Evaluate(const Vector3f& surface_pos, const Vector3f& light_dir, float& pdf) const {
    Ray ray(surface_pos, light_dir);
    Hit_Payload rec;
    
    if (!quad->isHit(ray, Vector2f(Epsilon, Infinity), rec)) {
        pdf = 0.0f;
        return Vector3f(0);
    }
    
    Vector3f light_normal = GetQuadNormal();
    float cos_theta = glm::dot(-light_dir, light_normal);
    
    if (cos_theta <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0);
    }
    
    float distance_sq = rec.t * rec.t;
    pdf = distance_sq / (area * cos_theta);
    
    return color * intensity;
}

bool QuadAreaLight::IsHit(const Ray& ray, float max_distance, Vector3f& radiance) const {
    Hit_Payload rec;
    if (quad->isHit(ray, Vector2f(Epsilon, max_distance), rec)) {
        radiance = color * intensity;
        return true;
    }
    return false;
}

Vector3f QuadAreaLight::SampleQuadSurface(Sampler& sampler) const {
    Vector2f uv = sampler.get_2d_sample();
    return quad->get_Q() + uv.x * quad->get_u() + uv.y * quad->get_v();
}

Vector3f QuadAreaLight::GetQuadNormal() const {
    return glm::normalize(glm::cross(quad->get_u(), quad->get_v()));
}