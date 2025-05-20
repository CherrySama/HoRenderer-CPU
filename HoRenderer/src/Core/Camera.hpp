/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"

struct CameraParams
{
    float aspect_ratio;
    float viewport_height;     
    float focal_length;        
    int image_width;
};


class Camera {
public:
    Camera(Vector3f pos) : cameraPos(pos) {}
    ~Camera() {};

    void Create(const CameraParams& params);
    void Initialize();
    void Move(Vector3f pos);

    Ray GenerateRay(int u, int v);

private:
    Vector3f cameraPos;        // 相机位置
    float aspect_ratio;        // 宽高比
    float viewport_height;     // 视口高度
    float focal_length;        // 焦距

public:
    int image_width, image_height;
    Vector3f pixel_delta_u;  // Offset to pixel to the right
    Vector3f pixel_delta_v;  // Offset to pixel below
    Vector3f pixel00_loc;    // Location of pixel 0, 0
};