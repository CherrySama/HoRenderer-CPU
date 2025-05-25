/*
	Created by Yinghao He on 2025-05-25
*/
#include "AABB.hpp"
#include "Ray.hpp"

AABB::AABB(const Vector3f &a, const Vector3f &b) {
    p_min = Vector3f(std::fmin(a.x, b.x), std::fmin(a.y, b.y), std::fmin(a.z, b.z));
    p_max = Vector3f(std::fmax(a.x, b.x), std::fmax(a.y, b.y), std::fmax(a.z, b.z));
    pad_to_minimums();
}

bool AABB::isHit(const Ray &ray, Vector2f &t_interval) const {
    auto ray_origin = ray.origin();
    auto ray_dir = ray.direction();
    for (int axis = 0; axis < 3; axis++) {
        const float invD = 1.0f / ray_dir[axis];
        auto t0 = (p_min[axis] - ray_origin[axis]) * invD;
        auto t1 = (p_max[axis] - ray_origin[axis]) * invD;

        if (invD < 0.0f) 
			std::swap(t0,t1);
        t_interval.x = t0 > t_interval.x ? t0 : t_interval.x;
        t_interval.y = t1 < t_interval.y ? t1 : t_interval.y;
        if (t_interval.y <= t_interval.x)
            return false;
	}
	return true;
}

void AABB::pad_to_minimums() {
    float delta = 0.0001f;
    // x axis
    if (p_max.x - p_min.x < delta) {
        float center = (p_max.x + p_min.x) * 0.5f;
        p_min.x = center - delta * 0.5f;
        p_max.x = center + delta * 0.5f;
    }
    // y
    if (p_max.y - p_min.y < delta) {
        float center = (p_max.y + p_min.y) * 0.5f;
        p_min.y = center - delta * 0.5f;
        p_max.y = center + delta * 0.5f;
    }
    // z
    if (p_max.z - p_min.z < delta) {
        float center = (p_max.z + p_min.z) * 0.5f;
        p_min.z = center - delta * 0.5f;
        p_max.z = center + delta * 0.5f;
    }
}

