/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"

class Integrator{
public:
    Integrator(int width, int height);
    ~Integrator();

    void RenderImage();
    uint8_t *GetPixels();

    // 简单相机参数
    Vector3f camera_center{0, 0, 0};  // 相机位置
    double viewport_height = 2.0;     // 视口高度
    double focal_length = 1.0;        // 焦距

    // 计算射线颜色（背景）
    Vector3f ray_color(const Ray& r) {
        // 获取射线的单位方向向量
        Vector3f unit_direction = glm::normalize(r.direction());
        // 基于y分量做线性插值，范围从0到1
        float t = 0.5 * (unit_direction.y + 1.0);
        // 从白色(1,1,1)到蓝色(0.5,0.7,1.0)的线性插值
        return (1.0f - t) * Vector3f(1.0, 1.0, 1.0) + t * Vector3f(0.5, 0.7, 1.0);
    }

    void Clean();

private:
    int width, height;
    uint8_t *pixels;
};