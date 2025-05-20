/*
    Created by Yinghao He on 2025-05-15
*/
#include "Camera.hpp"


void Camera::Create(const CameraParams &params)
{
    aspect_ratio = params.aspect_ratio;
    viewport_height = params.viewport_height;
    focal_length = params.focal_length;
    image_width = params.image_width;
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;
    Initialize();
}

void Camera::Initialize()
{
    // Calculate viewport basis vectors
    float viewport_width = viewport_height * (static_cast<float>(image_width) / image_height);
    auto viewport_u = Vector3f(viewport_width, 0, 0);
    auto viewport_v = Vector3f(0, -viewport_height, 0);
    // Calculate delta pixel of u and v
    pixel_delta_u = viewport_u / float(image_width);
    pixel_delta_v = viewport_v / float(image_height);
    // Calculate the top left corner of the viewport
    auto viewport_upper_left = cameraPos - Vector3f(0, 0, focal_length) - viewport_u/2.0f - viewport_v/2.0f;
    // Calculate the center of the first pixel
    pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
}

void Camera::Move(Vector3f pos)
{
    cameraPos = pos;
}

Ray Camera::GenerateRay(int u, int v, const Vector2f& offset)
{
    // Calculate pixel center position    
    Vector3f pixel_center = pixel00_loc 
                            + ((float(u) + offset.x) * pixel_delta_u) 
                            + ((float(v) + offset.y) * pixel_delta_v);
    // Generate ray: from camera center to pixel
    Vector3f ray_direction = pixel_center - cameraPos;
    return Ray(cameraPos, ray_direction);
}
