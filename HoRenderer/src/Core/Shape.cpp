/*
    Created by Yinghao He on 2025-05-18
*/
#include "Shape.hpp"

bool Sphere::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const
{
    Vector3f oc = center - r.origin();
    float a = glm::dot(r.direction(), r.direction());
    float h = glm::dot(r.direction(), oc);   // make b = -2h
    float c = glm::dot(oc, oc) - radius*radius;
    float discriminant = h * h -  a * c; //
    

    if (discriminant > 0) { 
        float sqrt_d = std::sqrtf(discriminant);
        float root = (h - sqrt_d) / a; 
        if (isInInterval(t_interval, root)){
            rec.t = root;
            rec.p = r.at(rec.t);
            // rec.normal = (rec.p - center) / radius;
            Vector3f outward_normal = (rec.p - center) / radius;
            rec.set_face_normal(r, outward_normal);
            return true;
        }
        root = (h + sqrt_d) / a;
        if (isInInterval(t_interval, root)){
            rec.t = root;
            rec.p = r.at(rec.t);
            Vector3f outward_normal = (rec.p - center) / radius;
            rec.set_face_normal(r, outward_normal);
            return true;
        }
    }

    return false;
}

bool Quad::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const
{
    // 计算光线与平面的交点
    float denom = glm::dot(normal, r.direction());
    
    // 如果光线与平面平行或接近平行，则无交点
    if (std::fabs(denom) < 1e-6)
        return false;
    
    // 计算交点参数t
    float t = glm::dot(center - r.origin(), normal) / denom;
    
    // 检查t是否在有效范围内
    if (t < t_interval.x || t > t_interval.y)
        return false;
    
    // 计算交点
    Vector3f hit_point = r.at(t);
    
    // 计算交点到中心的向量
    Vector3f to_hit = hit_point - center;
    
    // 计算在局部坐标系中的位置
    float u = glm::dot(to_hit, right);
    float v = glm::dot(to_hit, up_vector);
    
    // 检查是否在矩形范围内
    if (std::fabs(u) > half_width || std::fabs(v) > half_height)
        return false;
    
    // 填充记录
    rec.t = t;
    rec.p = hit_point;
    rec.set_face_normal(r, normal);
    
    return true;
}

bool Box::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Hit_Payload temp_rec;
    bool hit_anything = false;
    float closest_t = t_interval.y;
    
    // 检查射线与各个面的交点
    for (const auto& side : sides) {
        if (side->isHit(r, Vector2f(t_interval.x, closest_t), temp_rec)) {
            hit_anything = true;
            closest_t = temp_rec.t;
            rec = temp_rec;
        }
    }
    
    return hit_anything;
}

void Box::CreateSides()
{
    sides.clear();
    
    // 计算半尺寸(从中心点到各个方向的距离)
    Vector3f half_dim = dimensions * 0.5f;
    
    // 1. 前面 (z方向正面)
    sides.push_back(std::make_shared<Quad>(
        center + Vector3f(0, 0, half_dim.z),  // 中心点 + z方向偏移
        Vector3f(0, 0, 1),                    // 法向量(z轴正方向)
        Vector3f(0, 1, 0),                    // 上向量(y轴正方向)
        dimensions.x, dimensions.y            // 宽度和高度
    ));
    
    // 2. 后面 (z方向负面)
    sides.push_back(std::make_shared<Quad>(
        center - Vector3f(0, 0, half_dim.z),  // 中心点 - z方向偏移
        Vector3f(0, 0, -1),                   // 法向量(z轴负方向)
        Vector3f(0, 1, 0),                    // 上向量(y轴正方向)
        dimensions.x, dimensions.y            // 宽度和高度
    ));
    
    // 3. 顶面 (y方向正面)
    sides.push_back(std::make_shared<Quad>(
        center + Vector3f(0, half_dim.y, 0),  // 中心点 + y方向偏移
        Vector3f(0, 1, 0),                    // 法向量(y轴正方向)
        Vector3f(0, 0, 1),                    // 上向量(z轴正方向)
        dimensions.x, dimensions.z            // 宽度和长度
    ));
    
    // 4. 底面 (y方向负面)
    sides.push_back(std::make_shared<Quad>(
        center - Vector3f(0, half_dim.y, 0),  // 中心点 - y方向偏移
        Vector3f(0, -1, 0),                   // 法向量(y轴负方向)
        Vector3f(0, 0, 1),                    // 上向量(z轴正方向)
        dimensions.x, dimensions.z            // 宽度和长度
    ));
    
    // 5. 右面 (x方向正面)
    sides.push_back(std::make_shared<Quad>(
        center + Vector3f(half_dim.x, 0, 0),  // 中心点 + x方向偏移
        Vector3f(1, 0, 0),                    // 法向量(x轴正方向)
        Vector3f(0, 1, 0),                    // 上向量(y轴正方向)
        dimensions.z, dimensions.y            // 长度和高度
    ));
    
    // 6. 左面 (x方向负面)
    sides.push_back(std::make_shared<Quad>(
        center - Vector3f(half_dim.x, 0, 0),  // 中心点 - x方向偏移
        Vector3f(-1, 0, 0),                   // 法向量(x轴负方向)
        Vector3f(0, 1, 0),                    // 上向量(y轴正方向)
        dimensions.z, dimensions.y            // 长度和高度
    ));
}
