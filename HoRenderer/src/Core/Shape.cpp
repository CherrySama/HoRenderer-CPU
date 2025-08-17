/*
    Created by Yinghao He on 2025-05-18
*/
#include "Shape.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


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
            rec.interior_medium_id = interior_id;
            rec.exterior_medium_id = exterior_id;
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
            rec.interior_medium_id = interior_id;
            rec.exterior_medium_id = exterior_id;
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
    rec.interior_medium_id = interior_id;
    rec.exterior_medium_id = exterior_id;

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

void Box::CreateSides(const Vector3f& center, const Vector3f& dimensions, const Transform& transform)
{
    sides.clear();
    Vector3f half_dim = dimensions * 0.5f;
    
    // bottom (y-)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y - half_dim.y, center.z - half_dim.z), // Q
        Vector3f(dimensions.x, 0, 0),                                                   // u
        Vector3f(0, 0, dimensions.z),                                                   // v
        transform, mat, interior_id, exterior_id));

    // top (y+)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y + half_dim.y, center.z + half_dim.z), // Q
        Vector3f(dimensions.x, 0, 0),                                                   // u
        Vector3f(0, 0, -dimensions.z),                                                  // v
        transform, mat, interior_id, exterior_id));

    // front (z+)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y - half_dim.y, center.z + half_dim.z), // Q
        Vector3f(dimensions.x, 0, 0),                                                   // u
        Vector3f(0, dimensions.y, 0),                                                   // v
        transform, mat, interior_id, exterior_id));

    // back (z-)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x + half_dim.x, center.y - half_dim.y, center.z - half_dim.z), // Q
        Vector3f(-dimensions.x, 0, 0),                                                  // u
        Vector3f(0, dimensions.y, 0),                                                   // v
        transform, mat, interior_id, exterior_id));

    // right (x+)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x + half_dim.x, center.y - half_dim.y, center.z - half_dim.z), // Q
        Vector3f(0, 0, dimensions.z),                                                   // u
        Vector3f(0, dimensions.y, 0),                                                   // v
        transform, mat, interior_id, exterior_id));

    // left (x-)
    sides.push_back(std::make_shared<Quad>(
        Vector3f(center.x - half_dim.x, center.y - half_dim.y, center.z + half_dim.z), // Q
        Vector3f(0, 0, -dimensions.z),                                                  // u
        Vector3f(0, dimensions.y, 0),                                                   // v
        transform, mat, interior_id, exterior_id));

    if (!sides.empty()) {
        bbox = sides[0]->getBoundingBox();
        for (size_t i = 1; i < sides.size(); i++) {
            Vector3f side_min = sides[i]->getBoundingBox().min();
            Vector3f side_max = sides[i]->getBoundingBox().max();
            
            Vector3f current_min = bbox.min();
            Vector3f current_max = bbox.max();
            
            min_corner = Vector3f(std::min(current_min.x, side_min.x),
                                 std::min(current_min.y, side_min.y),
                                 std::min(current_min.z, side_min.z));
            max_corner = Vector3f(std::max(current_max.x, side_max.x),
                                 std::max(current_max.y, side_max.y),
                                 std::max(current_max.z, side_max.z));
            bbox = AABB(min_corner, max_corner);
        }
    }
}

Mesh::Mesh(const std::string &obj_path, const Transform &transform, std::shared_ptr<Material> material, int interior_medium_id, int exterior_medium_id) : mat(material), interior_id(interior_medium_id), exterior_id(exterior_medium_id)
{
    // initial Embree device
    embree_device = rtcNewDevice(nullptr);
    if (!embree_device) {
        std::cerr << "Failed to create Embree device" << std::endl;
        return;
    }

    // load obj
    if (!LoadOBJ(obj_path)) {
        std::cerr << "Failed to load OBJ file: " << obj_path << std::endl;
        return;
    }

    // calculate surface normal 
    if (normals.empty()) {
        CalculateFaceNormals();
    }

    // apply transform
    ApplyTransform(transform);
    // calculate AABB
    CalculateBoundingBox();
    // setup Embree
    CommitEmbree();
}

Mesh::~Mesh()
{
    if (embree_geometry) rtcReleaseGeometry(embree_geometry);
    if (embree_scene) rtcReleaseScene(embree_scene);
    if (embree_device) rtcReleaseDevice(embree_device);
}

bool Mesh::LoadOBJ(const std::string& filepath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str());

    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << "ERR: " << err << std::endl;
    }

    if (!ret) {
        std::cerr << "Failed to load OBJ file: " << filepath << std::endl;
        return false;
    }

    std::map<std::tuple<int, int, int>, int> vertex_map;
    int vertex_count = 0;

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.indices.size(); f += 3) {
            Vector3i triangle;

            for (int v = 0; v < 3; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f + v];

                auto key = std::make_tuple(idx.vertex_index, idx.normal_index, idx.texcoord_index);

                auto it = vertex_map.find(key);
                if (it != vertex_map.end()) {
                    triangle[v] = it->second;
                } else {
                    triangle[v] = vertex_count;
                    vertex_map[key] = vertex_count;
                    
                    if (idx.vertex_index >= 0) {
                        vertices.push_back(Vector3f(
                            attrib.vertices[3 * idx.vertex_index + 0],
                            attrib.vertices[3 * idx.vertex_index + 1],
                            attrib.vertices[3 * idx.vertex_index + 2]
                        ));
                    } else {
                        vertices.push_back(Vector3f(0.0f));
                    }
                    
                    if (idx.normal_index >= 0 && !attrib.normals.empty()) {
                        normals.push_back(Vector3f(
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        ));
                    } else {
                        normals.push_back(Vector3f(0.0f)); 
                    }
                    
                    if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                        texcoords.push_back(Vector2f(
                            attrib.texcoords[2 * idx.texcoord_index + 0],
                            attrib.texcoords[2 * idx.texcoord_index + 1]
                        ));
                    } else {
                        texcoords.push_back(Vector2f(0.0f, 0.0f));
                    }
                    
                    vertex_count++;
                }
            }
            
            indices.push_back(triangle);
        }
    }

    if (texcoords.size() == vertices.size()) {
        bool has_valid_texcoords = false;
        for (const auto& tc : texcoords) {
            if (tc.x != 0.0f || tc.y != 0.0f) {
                has_valid_texcoords = true;
                break;
            }
        }
        if (!has_valid_texcoords) {
            texcoords.clear();
        }
    }

    std::cout << "Loaded OBJ: " << attrib.vertices.size() / 3 << " original vertices, " 
              << vertices.size() << " mesh vertices, " << indices.size() << " triangles" << std::endl;
    
    return !vertices.empty() && !indices.empty();
}

void Mesh::CommitEmbree()
{
    // create scene
    embree_scene = rtcNewScene(embree_device);
    
    // create triangle mesh geometry
    embree_geometry = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

    // set up geometry buffer
    float* vertex_buffer = (float*)rtcSetNewGeometryBuffer(
        embree_geometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 
        3 * sizeof(float), vertices.size());
    
    for (size_t i = 0; i < vertices.size(); i++) {
        vertex_buffer[3 * i + 0] = vertices[i].x;
        vertex_buffer[3 * i + 1] = vertices[i].y;
        vertex_buffer[3 * i + 2] = vertices[i].z;
    }

    // set up the index buffer
    unsigned int* index_buffer = (unsigned int*)rtcSetNewGeometryBuffer(
        embree_geometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
        3 * sizeof(unsigned int), indices.size());
    
    for (size_t i = 0; i < indices.size(); i++) {
        index_buffer[3 * i + 0] = indices[i].x;
        index_buffer[3 * i + 1] = indices[i].y;
        index_buffer[3 * i + 2] = indices[i].z;
    }

    rtcCommitGeometry(embree_geometry);
    rtcAttachGeometry(embree_scene, embree_geometry);
    rtcCommitScene(embree_scene);
    vertices.clear();
    vertices.shrink_to_fit();
}

void Mesh::CalculateFaceNormals()
{
    bool needs_calculation = false;
    for (const auto& normal : normals) {
        if (glm::length(normal) < Epsilon) {
            needs_calculation = true;
            break;
        }
    }

    if (!needs_calculation)
        return;

    // Calculate the face normal of each triangle
    for (size_t i = 0; i < indices.size(); i++) {
        const Vector3i& triangle = indices[i];
        Vector3f v0 = vertices[triangle.x];
        Vector3f v1 = vertices[triangle.y];
        Vector3f v2 = vertices[triangle.z];

        Vector3f face_normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        // set surface normal for each point of each triangle
        normals[triangle.x] = face_normal;
        normals[triangle.y] = face_normal;
        normals[triangle.z] = face_normal;
    }
}

void Mesh::ApplyTransform(const Transform& transform)
{
    if (transform.IsIdentity())
        return;

    for (auto& vertex : vertices) 
        vertex = transform.TransformPoint(vertex);

    for (auto& normal : normals) {
        if (glm::length(normal) > Epsilon) {
            normal = transform.TransformNormal(normal);
        }
    }
}

void Mesh::CalculateBoundingBox()
{
    if (vertices.empty()) {
        bbox = AABB();
        return;
    }

    Vector3f min_point = vertices[0];
    Vector3f max_point = vertices[0];

    for (const auto& vertex : vertices) {
        min_point = Vector3f(std::fmin(min_point.x, vertex.x),
                            std::fmin(min_point.y, vertex.y),
                            std::fmin(min_point.z, vertex.z));
        max_point = Vector3f(std::fmax(max_point.x, vertex.x),
                            std::fmax(max_point.y, vertex.y),
                            std::fmax(max_point.z, vertex.z));
    }

    bbox = AABB(min_point, max_point);
}

Vector3f Mesh::InterpolateNormal(int triangle_id, float u, float v) const
{
    const Vector3i& triangle = indices[triangle_id];
    float w = 1.0f - u - v;

    Vector3f normal = w * normals[triangle.x] + u * normals[triangle.y] + v * normals[triangle.z];
    return glm::normalize(normal);
}

Vector2f Mesh::InterpolateTexCoord(int triangle_id, float u, float v) const
{
    const Vector3i& triangle = indices[triangle_id];
    float w = 1.0f - u - v;

    return w * texcoords[triangle.x] + u * texcoords[triangle.y] + v * texcoords[triangle.z];
}

bool Mesh::isHit(const Ray& r, Vector2f t_interval, Hit_Payload& rec) const
{
    RTCRayHit rayhit;
    rayhit.ray.org_x = r.origin().x;
    rayhit.ray.org_y = r.origin().y;
    rayhit.ray.org_z = r.origin().z;
    rayhit.ray.dir_x = r.direction().x;
    rayhit.ray.dir_y = r.direction().y;
    rayhit.ray.dir_z = r.direction().z;
    rayhit.ray.tnear = t_interval.x;
    rayhit.ray.tfar = t_interval.y;
    rayhit.ray.mask = 0xFFFFFFFF;
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(embree_scene, &rayhit);

    if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) 
        return false;

    rec.t = rayhit.ray.tfar;
    rec.p = r.at(rec.t);
    Vector3f interpolated_normal = InterpolateNormal(rayhit.hit.primID, rayhit.hit.u, rayhit.hit.v);
    rec.set_face_normal(r, interpolated_normal);

    if (!texcoords.empty()) {
        rec.uv = InterpolateTexCoord(rayhit.hit.primID, rayhit.hit.u, rayhit.hit.v);
    } else {
        rec.uv = Vector2f(rayhit.hit.u, rayhit.hit.v);
    }

    rec.mat = mat;
    rec.interior_medium_id = interior_id;
    rec.exterior_medium_id = exterior_id;

    return true;
}