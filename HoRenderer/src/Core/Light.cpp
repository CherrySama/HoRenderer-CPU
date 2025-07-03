/*
    Created by Yinghao He on 2025-06-22
*/
#include "Light.hpp"
#include "Sampler.hpp"


Vector3f QuadAreaLight::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const
{
    Vector3f light_point = SampleQuadSurface(sampler); 
    Vector3f light_to_surface = light_point - rec.p;
    float distance_sq = glm::dot(light_to_surface, light_to_surface); 
    if (distance_sq < Epsilon) {
        pdf = 0.0f;
        return Vector3f(0);
    }
    float distance = std::sqrt(distance_sq);
    light_direction = light_to_surface / distance;

    Vector3f light_normal = GetQuadNormal();
    float cos_theta = glm::dot(-light_direction, light_normal);
    if (cos_theta <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0);
    }
    
    // pdf = distance² / (area * cos_theta)
    pdf = distance_sq / (area * cos_theta);

    return color * intensity;
}

Vector3f QuadAreaLight::Evaluate(const Ray &r_in, const Hit_Payload &rec, const Vector3f &light_direction, float &pdf) const
{
    Vector3f light_normal = GetQuadNormal();
    float cos_theta = glm::dot(-light_direction, light_normal);
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