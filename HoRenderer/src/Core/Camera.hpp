/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include "Util.hpp"
#include "Sampler.hpp"

struct CameraParams
{
    float aspect_ratio;
    int image_width;
    float vfov;
    Vector3f lookfrom;
    Vector3f lookat;
    Vector3f vup;
    float defocus_angle;
    float focus_dist;
};


class Camera {
public:
    Camera() {}
    ~Camera() {};

    void Create(const CameraParams& params);
    void MoveTo(Vector3f pos);

    Vector3f DefocusDisk(Sampler &sampler) const;

    Ray GenerateRay(int u, int v, Sampler &sampler, const Vector2f& offset = Vector2f(0, 0)) const;

private:
    Vector3f cameraPos;        // pos of camera center point
    float aspect_ratio;        
    float viewport_height;     
    float vfov;                // field of view
    Vector3f u, v, w;
    float defocus_angle;
    Vector3f defocus_disk_u;   // Defocus disk horizontal radius
    Vector3f defocus_disk_v;   // Defocus disk vertical radius

public:
    int image_width, image_height;
    Vector3f pixel_delta_u;  // Offset to pixel to the right
    Vector3f pixel_delta_v;  // Offset to pixel below
    Vector3f pixel00_loc;    // Location of pixel 0, 0
};