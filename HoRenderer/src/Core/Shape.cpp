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
            rec.mat = mat;
            rec.uv = getSphereUV(rec.p);
            return true;
        }
        root = (h + sqrt_d) / a;
        if (isInInterval(t_interval, root)){
            rec.t = root;
            rec.p = r.at(rec.t);
            Vector3f outward_normal = (rec.p - center) / radius;
            rec.set_face_normal(r, outward_normal);
            rec.mat = mat;
            rec.uv = getSphereUV(rec.p);
            return true;
        }
    }

    return false;
}

Vector2f Sphere::getSphereUV(const Vector3f &hit_point) const
{
    Vector3f unit_p = glm::normalize(hit_point - center);
    float theta = std::acos(glm::clamp(unit_p.y, -1.0f, 1.0f));
    float phi = std::atan2(unit_p.z, unit_p.x);

    if (phi < 0)
        phi += 2.0f * PI;

    float u = phi / (2.0f * PI);
    float v = theta / PI;
    return Vector2f(u, v);
}

bool Quad::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const
{
    // Compute the intersection of a ray and a plane
    float denom = glm::dot(normal, r.direction());
    
    // If the ray is parallel or nearly parallel to the plane, there is no intersection
    if (std::fabs(denom) < 1e-6)
        return false;
    
    // Calculate the intersection parameter t
    float t = (D - glm::dot(normal, r.origin())) / denom;
    
    // Check if t is in the valid range
    if (t < t_interval.x || t > t_interval.y)
        return false;
    
    // Calculate intersection points
    Vector3f hit_point = r.at(t);
    
    // Calculate the vector from the intersection point to the center
    Vector3f hit_vec = hit_point - Q;
    
    // Compute parameter coordinates (alpha, beta)
    float alpha = glm::dot(w, glm::cross(hit_vec, v));
    float beta = glm::dot(w, glm::cross(u, hit_vec));
    
    // Check if it is within the rectangle
    if (alpha < -Epsilon || alpha > 1 + Epsilon || beta < -Epsilon || beta > 1 + Epsilon)
        return false;
    
    rec.t = t;
    rec.p = hit_point;
    rec.set_face_normal(r, normal);
    rec.mat = mat;
    rec.uv = Vector2f(alpha, beta);

    return true;
}

bool Box::isHit(const Ray &r, Vector2f t_interval, Hit_Payload &rec) const {
    Hit_Payload temp_rec;
    bool hit_anything = false;
    float closest_t = t_interval.y;
    
    // Check the intersection of the ray with each face
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

    // Calculate the half size (the distance from the center point in all directions)
    Vector3f half_dim = dimensions * 0.5f;

    // Front (z+)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y - half_dim.y, center.z + half_dim.z), 
        Vector3f(dimensions.x, 0, 0),                                                  
        Vector3f(0, dimensions.y, 0),                                                  
        mat));

    // back (z-)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x + half_dim.x, center.y - half_dim.y, center.z - half_dim.z),
        Vector3f(-dimensions.x, 0, 0),
        Vector3f(0, dimensions.y, 0),
        mat));

    // top (y+)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y + half_dim.y, center.z + half_dim.z),
        Vector3f(dimensions.x, 0, 0),
        Vector3f(0, 0, -dimensions.z),
        mat));

    // bottom (y-)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y - half_dim.y, center.z - half_dim.z),
        Vector3f(dimensions.x, 0, 0),
        Vector3f(0, 0, dimensions.z),
        mat));

    // right (x+)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x + half_dim.x, center.y - half_dim.y, center.z + half_dim.z),
        Vector3f(0, 0, -dimensions.z),
        Vector3f(0, dimensions.y, 0),
        mat));

    // left (x-)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y - half_dim.y, center.z - half_dim.z),
        Vector3f(0, 0, dimensions.z),
        Vector3f(0, dimensions.y, 0),
        mat));
}
