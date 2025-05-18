/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"

class Sphere : public Hittable{
public:
    Sphere() {}
    Sphere(const Vector3f center, float radius) : center(center), radius(std::fmax(0,radius)) {}

    bool isHit(const Ray &r, float t_min, float t_max, Hit_Record &rec) const override;

private:
    Vector3f center;
    float radius;
};

class Quad : public Hittable{
public:
    Quad() {}
    // 通过中心点、法向量、上向量和尺寸定义矩形
    Quad(const Vector3f& center, const Vector3f& normal, const Vector3f& up, 
             float width, float height)
        : center(center), normal(glm::normalize(normal)), width(width), height(height)
    {
        // 计算右向量和上向量，确保它们正交
        right = glm::normalize(glm::cross(up, normal));
        up_vector = glm::normalize(glm::cross(normal, right));
        
        // 计算半宽和半高
        half_width = width * 0.5f;
        half_height = height * 0.5f;
    }

    bool isHit(const Ray &r, float t_min, float t_max, Hit_Record &rec) const override;

private:
    Vector3f center;     // 矩形中心
    Vector3f normal;     // 法向量
    Vector3f right;      // 右方向向量
    Vector3f up_vector;  // 上方向向量
    float width;         // 宽度
    float height;        // 高度
    float half_width;    // 半宽
    float half_height;   // 半高
};

class Box : public Hittable {
public:
    Box() {}
    
    // 唯一的构造方法：使用中心点和尺寸
    Box(const Vector3f& center, const Vector3f& dimensions) : center(center), dimensions(dimensions)
    {
        // 计算最小和最大点（用于内部计算和调试）
        min_corner = center - dimensions * 0.5f;
        max_corner = center + dimensions * 0.5f;
        
        // 创建6个面
        CreateSides();
    }

    bool isHit(const Ray &r, float t_min, float t_max, Hit_Record &rec) const override;

private:
    Vector3f center;      // 长方体的中心点
    Vector3f dimensions;  // 长方体的尺寸 (x=宽, y=高, z=长)
    Vector3f min_corner;  // 最小点 (内部计算用)
    Vector3f max_corner;  // 最大点 (内部计算用)
    
    // 长方体的6个面（存储为矩形）
    std::vector<std::shared_ptr<Quad>> sides;
    
    // 计算6个面的辅助函数
    void CreateSides();
};