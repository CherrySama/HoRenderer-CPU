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
    Vector3f surface_to_light = light_point - rec.p;
    float distance_sq = glm::dot(surface_to_light, surface_to_light); 
    if (distance_sq < Epsilon) {
        pdf = 0.0f;
        return Vector3f(0);
    }
    float distance = std::sqrt(distance_sq);
    light_direction = surface_to_light / distance;

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
    float cos_theta = glm::dot(-glm::normalize(r_in.direction()), light_normal);
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

Vector3f SphereAreaLight::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const
{
    Vector3f dir_to_center = center - rec.p;
    float dist_sq = glm::dot(dir_to_center, dir_to_center);
    float inv_dist = 1.0f / std::sqrt(dist_sq);
    dir_to_center *= inv_dist;
    float distance = dist_sq * inv_dist;

    float sin_theta_max = radius * inv_dist;

    if (sin_theta_max >= 1.0f) {
        Vector3f surface_point = SampleSphereSurface(sampler);
        light_direction = glm::normalize(surface_point - rec.p);
        float dist = glm::length(surface_point - rec.p);

        Vector3f surface_normal = glm::normalize(surface_point - center);
        float cos_theta = glm::dot(-light_direction, surface_normal);
        
        if (cos_theta <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }
        
        pdf = 1.0f / area;
        pdf *= dist * dist / cos_theta;
        
        Vector2f uv = sphere->getSphereUV(surface_point);  
        return sphere->get_mat()->Emit(uv); 
    }

    float cos_theta_max = std::sqrt(1.0f - sin_theta_max * sin_theta_max);

    Vector2f sample = sampler.get_2d_sample();
    float cos_theta = 1.0f - sample.x + sample.x * cos_theta_max;
    float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
    float phi = 2.0f * PI * sample.y;

    Vector3f local_dir = Vector3f(sin_theta * std::cos(phi), 
                                  sin_theta * std::sin(phi), 
                                  cos_theta);
    
    light_direction = ToWorld(local_dir, dir_to_center);
    Ray light_ray(rec.p, light_direction);
    Hit_Payload light_hit;
    if (!sphere->isHit(light_ray, Vector2f(Epsilon, Infinity), light_hit)) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    
    pdf = 1.0f / (2.0f * PI * (1.0f - cos_theta_max));
    
    return sphere->get_mat()->Emit(light_hit.uv);
}

Vector3f SphereAreaLight::Evaluate(const Ray &r_in, const Hit_Payload &rec, float &pdf) const
{
    Vector3f dir_to_center = center - r_in.origin();
    float dist_sq = glm::dot(dir_to_center, dir_to_center);
    float sin_theta_max_sq = radius * radius / dist_sq;
    
    if (sin_theta_max_sq >= 1.0f) {
        float distance = glm::length(rec.p - r_in.origin());
        Vector3f surface_normal = glm::normalize(rec.p - center);
        float cos_theta = glm::dot(-glm::normalize(r_in.direction()), surface_normal);
        
        if (cos_theta <= 0.0f) {
            pdf = 0.0f;
            return Vector3f(0.0f);
        }
        
        pdf = distance * distance / (area * cos_theta);
        return sphere->get_mat()->Emit(rec.uv);
    }

    float cos_theta_max = std::sqrt(1.0f - sin_theta_max_sq);
    pdf = 1.0f / (2.0f * PI * (1.0f - cos_theta_max));
    
    return sphere->get_mat()->Emit(rec.uv);
}

float SphereAreaLight::GetPower() const
{
    Vector3f emission = sphere->get_mat()->Emit(Vector2f(0.5f, 0.5f));
    float luminance = 0.299f * emission.r + 0.587f * emission.g + 0.114f * emission.b;
    return area * luminance * PI; 
}

std::shared_ptr<Hittable> SphereAreaLight::GetShape() const
{
    return sphere;
}

Vector3f SphereAreaLight::SampleSphereSurface(Sampler &sampler) const
{
    Vector2f sample = sampler.get_2d_sample();
    float z = 1.0f - 2.0f * sample.x;
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2.0f * PI * sample.y;
    
    Vector3f local_point = Vector3f(r * std::cos(phi), r * std::sin(phi), z);
    return center + radius * local_point;
}

InfiniteAreaLight::InfiniteAreaLight(std::shared_ptr<HDRTexture> hdr, float scale) : hdr_texture(hdr), scale(scale)
{
    if (!hdr_texture) {
        throw std::invalid_argument("Environment light requires an HDR texture");
    }

    int width = hdr_texture->getWidth();
    int height = hdr_texture->getHeight();
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Cannot create an environment light from an invalid HDR texture");
    }

    std::vector<float> weights(width * height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float u = (x + 0.5f) / width;
            float v = (y + 0.5f) / height;
            
            Vector3f color = hdr_texture->GetColor(u, v);
            float luminance = Luminance(color);

            float theta = v * PI;
            float sin_theta = std::sin(theta);
            
            weights[y * width + x] = luminance * sin_theta;
        }
    }

    table = AliasTable2D(weights, width, height);
}

Vector3f InfiniteAreaLight::Sample(const Ray &r_in, const Hit_Payload &rec, Vector3f &light_direction, float &pdf, Sampler &sampler) const
{
    if (table.Sum() <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector2f sample_2d = sampler.get_2d_sample();
    Vector2f marginal_sample = sampler.get_2d_sample();
    Vector2i pixel = table.Sample(sample_2d, marginal_sample);
    if (pixel.x < 0 || pixel.y < 0) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    
    int width = hdr_texture->getWidth();
    int height = hdr_texture->getHeight();
    
    float u = (pixel.x + 0.5f) / width;
    float v = (pixel.y + 0.5f) / height;
    light_direction = SphericalToCartesian(u, v);

    Vector3f color = hdr_texture->GetColor(u, v);
    float theta = v * PI;
    float sin_theta = std::sin(theta);
    
    if (sin_theta <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    
    float luminance = Luminance(color);
    pdf = luminance / table.Sum() * width * height / (2.0f * PI * PI);
    
    return color * scale;
}

Vector3f InfiniteAreaLight::Evaluate(const Ray &r_in, const Hit_Payload &rec, float &pdf) const {
    if (table.Sum() <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }

    Vector3f dir = glm::normalize(r_in.direction());
    Vector2f uv = CartesianToSpherical(dir);
    
    Vector3f color = hdr_texture->GetColor(uv.x, uv.y);

    float theta = uv.y * PI;
    float sin_theta = std::sin(theta);
    
    if (sin_theta <= 0.0f) {
        pdf = 0.0f;
        return Vector3f(0.0f);
    }
    
    float luminance = Luminance(color);
    int width = hdr_texture->getWidth();
    int height = hdr_texture->getHeight();
    
    pdf = luminance / table.Sum() * width * height / (2.0f * PI * PI);
    
    return color * scale;
}

float InfiniteAreaLight::GetPower() const
{
    int width = hdr_texture->getWidth();
    int height = hdr_texture->getHeight();
    if (width <= 0 || height <= 0) {
        return 0.0f;
    }
    return table.Sum() * scale * 2.0f * PI * PI / (width * height);  
}

std::shared_ptr<Hittable> InfiniteAreaLight::GetShape() const
{
    return nullptr;  
}
