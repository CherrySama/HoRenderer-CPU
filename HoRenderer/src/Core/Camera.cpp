/*
    Created by Yinghao He on 2025-05-15
*/
#include "Camera.hpp"
#include "Ray.hpp"


void Camera::Create(const CameraParams &params)
{
    aspect_ratio = params.aspect_ratio;
    // focal_length = params.focal_length;
    vfov = params.vfov;

    image_width = params.image_width;
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;
    float inv_image_width = 1.0f / float(image_width);
    float inv_image_height = 1.0f / float(image_height);

    cameraPos = params.lookfrom;
    // focal_length = glm::length(params.lookfrom - params.lookat);
    w = glm::normalize(params.lookfrom - params.lookat);
    u = glm::normalize(glm::cross(params.vup, w));
    v = glm::cross(w, u);

    float theta = degrees_to_radians(vfov);
    float h = std::tan(theta / 2.0f);
    viewport_height = 2.0f * h * params.focus_dist;


    float viewport_width = viewport_height * image_width * inv_image_height;
    // auto viewport_u = Vector3f(viewport_width, 0, 0);
    // auto viewport_v = Vector3f(0, -viewport_height, 0);
    Vector3f viewport_u = viewport_width * u;
    Vector3f viewport_v = viewport_height * -v;
    // Calculate delta pixel of u and v
    pixel_delta_u = viewport_u * inv_image_width;
    pixel_delta_v = viewport_v * inv_image_height;

    auto viewport_upper_left = cameraPos - (params.focus_dist * w) - viewport_u / 2.0f - viewport_v / 2.0f;
    // Calculate the center of the first pixel
    pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    // Calculate the camera defocus disk basis vectors.
    defocus_angle = params.defocus_angle;
    auto defocus_radius = params.focus_dist * std::tan(degrees_to_radians(params.defocus_angle / 2));
    defocus_disk_u = u * defocus_radius;
    defocus_disk_v = v * defocus_radius;
}

void Camera::Move(Vector3f pos)
{
    cameraPos = pos;
}

Vector3f Camera::DefocusDisk(Sampler &sampler)
{
    auto p = sampler.random_unit_2Dvector();
    return cameraPos + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
}

Ray Camera::GenerateRay(int u, int v, Sampler &sampler, const Vector2f& offset)
{
    // Calculate pixel center position    
    Vector3f pixel_center = pixel00_loc 
                            + ((float(u) + offset.x) * pixel_delta_u) 
                            + ((float(v) + offset.y) * pixel_delta_v);

    Vector3f ray_origin = (defocus_angle <= 0.0f) ? cameraPos : DefocusDisk(sampler);
    Vector3f ray_direction = pixel_center - ray_origin;
    return Ray(ray_origin, ray_direction);
}
