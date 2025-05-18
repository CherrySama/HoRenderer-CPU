/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "HittableList.hpp"
#include "Hittable.hpp"
#include "Shape.hpp"

class Integrator{
public:
    Integrator(int width, int height);
    ~Integrator();

    void RenderImage();
    uint8_t *GetPixels();

    // 简单相机参数
    Vector3f camera_center{0, 0, 0};  // 相机位置
    float viewport_height = 2.0;     // 视口高度
    float focal_length = 1.0;        // 焦距

    // World
    HittableList world;

    // 计算射线颜色（背景）
    Vector3f ray_color(const Ray &r, const Hittable &world);

    float hit_sphere(const Vector3f& center, float radius, const Ray& r);

    void Clean();

private:
    int width, height;
    uint8_t *pixels;
};