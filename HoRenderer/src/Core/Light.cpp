/*
    Created by Yinghao He on 2025-06-22
*/
#include "Light.hpp"
#include "Sampler.hpp"
#include "Material.hpp"


Vector3f QuadAreaLight::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const
{
    Vector2f uv = sampler.get_2d_sample();
    Vector3f light_point = quad->get_Q() + uv.x * quad->get_u() + uv.y * quad->get_v();
    Vector3f light_to_surface = light_point - rec.p;
    float distance_sq = glm::dot(light_to_surface, light_to_surface); 
    if (distance_sq < Epsilon) {
        pdf = 0.0f;
        return Vector3f(0);
    }
    float distance = std::sqrt(distance_sq);
    light_direction = light_to_surface / distance;

    Vector3f light_normal = glm::normalize(glm::cross(quad->get_u(), quad->get_v()));
    float cos_theta = glm::dot(-light_direction, light_normal);
    if (cos_theta <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0);
    }

    pdf = distance_sq / (area * cos_theta);
    return quad->get_mat()->Emit(uv);
}

Vector3f QuadAreaLight::Evaluate(const Ray &r_in, const Hit_Payload &rec, float &pdf) const
{
    Vector3f light_normal = glm::normalize(glm::cross(quad->get_u(), quad->get_v()));
    float cos_theta = glm::dot(-r_in.direction(), light_normal);
    if (cos_theta <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0);
    }

    float distance = glm::length(rec.p - r_in.origin());
    float distance_sq = distance * distance;
    pdf = distance_sq / (area * cos_theta);

    return quad->get_mat()->Emit(rec.uv);
}

float QuadAreaLight::GetPower() const
{
    Vector3f emission = quad->get_mat()->Emit(Vector2f(0.5f, 0.5f));                   
    float luminance = 0.299f * emission.r + 0.587f * emission.g + 0.114f * emission.b; 
    return area * luminance * PI;
}

std::shared_ptr<Hittable> QuadAreaLight::GetShape() const
{
    return quad;
}