/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"

class Sphere : public Hittable{
public:
    Sphere() {}
    Sphere(const Vector3f center, float radius, std::shared_ptr<Material> material = nullptr)
        : center(center), radius(std::fmax(0, radius)), mat(material) {
            Vector3f rvec = Vector3f(radius, radius, radius);
            bbox = AABB(center - rvec, center + rvec);
        }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override {
        return bbox;
    }
    
private:
    Vector3f center;
    float radius;
    std::shared_ptr<Material> mat;
    AABB bbox;
};

class Quad : public Hittable{
public:
    Quad() {}
    // Define a rectangle by center point, normal vector, up vector and size
    Quad(const Vector3f& center, const Vector3f& normal, const Vector3f& up, float width, float height, std::shared_ptr<Material> material = nullptr)
        : center(center), normal(glm::normalize(normal)), width(width), height(height), mat(material)
    {
        // Calculate the right and up vectors, making sure they are orthogonal
        right = glm::normalize(glm::cross(up, normal));
        up_vector = glm::normalize(glm::cross(normal, right));
        
        // Calculate half width and half height
        half_width = width * 0.5f;
        half_height = height * 0.5f;

        // Calculate bbox
        Vector3f extent_u = right * half_width;
        Vector3f extent_v = up_vector * half_height;

        // Corner points
        Vector3f corners[4] = {
            center - extent_u - extent_v,
            center + extent_u - extent_v,
            center + extent_u + extent_v,
            center - extent_u + extent_v};
            
        Vector3f min_point = corners[0];
        Vector3f max_point = corners[0];
        for (int i = 1; i < 4; i++) {
            min_point = Vector3f(std::fmin(min_point.x, corners[i].x),
                               std::fmin(min_point.y, corners[i].y),
                               std::fmin(min_point.z, corners[i].z));
            max_point = Vector3f(std::fmax(max_point.x, corners[i].x),
                               std::fmax(max_point.y, corners[i].y),
                               std::fmax(max_point.z, corners[i].z));
        }
        
        bbox = AABB(min_point,max_point);                
    }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override {
        return bbox;
    }
    
private:
    Vector3f center;     
    Vector3f normal;    
    Vector3f right;      
    Vector3f up_vector;  
    float width;       
    float height;      
    float half_width;  
    float half_height;
    std::shared_ptr<Material> mat;
    AABB bbox;
};

class Box : public Hittable {
public:
    Box() {}
    Box(const Vector3f& center, const Vector3f& dimensions, std::shared_ptr<Material> material = nullptr) 
        : center(center), dimensions(dimensions), mat(material)
    {
        // Calculate minimum and maximum points
        min_corner = center - dimensions * 0.5f;
        max_corner = center + dimensions * 0.5f;
        bbox = AABB(min_corner, max_corner);
        // Create 6 faces
        CreateSides();
    }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override { return bbox; }

private:
    Vector3f center;      // The center point of the cuboid
    Vector3f dimensions;  // The dimensions of the cuboid (x=width, y=height, z=length)
    Vector3f min_corner;  // Minimum point (for internal calculation)
    Vector3f max_corner;  // Maximum point (for internal calculation)
    std::shared_ptr<Material> mat;
    AABB bbox;
    
    // The 6 faces of a cuboid (stored as rectangles)
    std::vector<std::shared_ptr<Quad>> sides;
    
    // Auxiliary functions for calculating 6 faces
    void CreateSides();
};