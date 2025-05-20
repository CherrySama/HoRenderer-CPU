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

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;

private:
    Vector3f center;
    float radius;
};

class Quad : public Hittable{
public:
    Quad() {}
    // Define a rectangle by center point, normal vector, up vector and size
    Quad(const Vector3f& center, const Vector3f& normal, const Vector3f& up, float width, float height)
        : center(center), normal(glm::normalize(normal)), width(width), height(height)
    {
        // Calculate the right and up vectors, making sure they are orthogonal
        right = glm::normalize(glm::cross(up, normal));
        up_vector = glm::normalize(glm::cross(normal, right));
        
        // Calculate half width and half height
        half_width = width * 0.5f;
        half_height = height * 0.5f;
    }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;

private:
    Vector3f center;     
    Vector3f normal;    
    Vector3f right;      
    Vector3f up_vector;  
    float width;       
    float height;      
    float half_width;  
    float half_height; 
};

class Box : public Hittable {
public:
    Box() {}
    Box(const Vector3f& center, const Vector3f& dimensions) : center(center), dimensions(dimensions)
    {
        // Calculate minimum and maximum points
        min_corner = center - dimensions * 0.5f;
        max_corner = center + dimensions * 0.5f;
        
        // Create 6 faces
        CreateSides();
    }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;

private:
    Vector3f center;      // The center point of the cuboid
    Vector3f dimensions;  // The dimensions of the cuboid (x=width, y=height, z=length)
    Vector3f min_corner;  // Minimum point (for internal calculation)
    Vector3f max_corner;  // Maximum point (for internal calculation)
    
    // The 6 faces of a cuboid (stored as rectangles)
    std::vector<std::shared_ptr<Quad>> sides;
    
    // Auxiliary functions for calculating 6 faces
    void CreateSides();
};