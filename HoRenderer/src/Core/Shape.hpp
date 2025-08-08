/*
    Created by Yinghao He on 2025-05-18
*/
#pragma once

#include "Util.hpp"
#include "Hittable.hpp"
#include "Transform.hpp"

class Sphere : public Hittable {
public:
    Sphere() {}
    Sphere(const Vector3f center, float radius, const Transform &transform = Transform(), std::shared_ptr<Material> material = nullptr, int interior_medium_id = -1, int exterior_medium_id = -1) :
        center(center), radius(std::fmax(0, radius)), mat(material), interior_id(interior_medium_id), exterior_id(exterior_medium_id) {
        this->center = transform.TransformPoint(center);
        Vector3f scale_x = transform.TransformVector(Vector3f(1,0,0));
        Vector3f scale_y = transform.TransformVector(Vector3f(0,1,0));
        Vector3f scale_z = transform.TransformVector(Vector3f(0, 0, 1));
        float max_scale = std::max({glm::length(scale_x), glm::length(scale_y), glm::length(scale_z)});
        this->radius = radius * max_scale;
        Vector3f rvec = Vector3f(this->radius, this->radius, this->radius);
        bbox = AABB(this->center - rvec, this->center + rvec);
    }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override {
        return bbox;
    }
    Vector2f getSphereUV(const Vector3f &hit_point) const;
    Vector3f getCenter() const { return center; }
    float getRadius() const { return radius; }
    std::shared_ptr<Material> get_mat() const {return mat;}

private:
    Vector3f center;
    float radius;
    std::shared_ptr<Material> mat;
    AABB bbox;
    int interior_id = -1;  
    int exterior_id = -1;
};

class Quad : public Hittable {
public:
    Quad() {}
    Quad(const Vector3f &Q, const Vector3f &u, const Vector3f &v, const Transform &transform = Transform(), std::shared_ptr<Material> material = nullptr, int interior_medium_id = -1, int exterior_medium_id = -1) :
        Q(Q), u(u), v(v), mat(material), interior_id(interior_medium_id), exterior_id(exterior_medium_id) {
        this->Q = transform.TransformPoint(Q);
        this->u = transform.TransformVector(u);
        this->v = transform.TransformVector(v);

        normal = glm::normalize(glm::cross(this->u, this->v));
        D = glm::dot(normal, this->Q);
        Vector3f n = glm::cross(this->u, this->v);
        w = n / glm::dot(n, n);

        // Calculate bounding box
        Vector3f corners[4] = {this->Q,
                               this->Q + this->u,
                               this->Q + this->v,
                               this->Q + this->u + this->v};

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
        bbox = AABB(min_point, max_point);
    }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override {
        return bbox;
    }

    Vector3f get_u() const {return u;}
    Vector3f get_v() const {return v;}
    Vector3f get_Q() const {return Q;}
    std::shared_ptr<Material> get_mat() const {return mat;}
    
private:
    Vector3f Q;    // The base point of the quadrilateral (a corner point)
    Vector3f u, v; // Two edge vectors starting from point Q
    Vector3f normal;
    Vector3f w; // Auxiliary vectors for parameter coordinate calculation
    float D;    // Plane equation D value
    std::shared_ptr<Material> mat;
    AABB bbox;
    int interior_id = -1;  
    int exterior_id = -1;
};

class Box : public Hittable {
public:
    Box() {}
    Box(const Vector3f &center, const Vector3f &dimensions, const Transform& transform = Transform(), std::shared_ptr<Material> material = nullptr, int interior_medium_id = -1, int exterior_medium_id = -1) :
        center(center), dimensions(dimensions), mat(material), interior_id(interior_medium_id), exterior_id(exterior_medium_id) {
        // Create 6 faces
        CreateSides(center, dimensions, transform);
    }

    bool isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const override;
    AABB getBoundingBox() const override {
        return bbox;
    }

private:
    Vector3f center;     // The center point of the cuboid
    Vector3f dimensions; // The dimensions of the cuboid (x=width, y=height, z=length)
    Vector3f min_corner; // Minimum point (for internal calculation)
    Vector3f max_corner; // Maximum point (for internal calculation)
    std::shared_ptr<Material> mat;
    AABB bbox;
    int interior_id = -1;  
    int exterior_id = -1;

    // The 6 faces of a cuboid (stored as rectangles)
    std::vector<std::shared_ptr<Quad>> sides;

    // Auxiliary functions for calculating 6 faces
    void CreateSides(const Vector3f& center, const Vector3f& dimensions, const Transform& transform);
};