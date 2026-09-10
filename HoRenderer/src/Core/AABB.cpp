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
    for (int axis = 0; axis < 3; ++axis) {
        // A fixed 1e-4 padding rounds away at large scene coordinates.
        const float scale = std::max(std::abs(p_min[axis]), std::abs(p_max[axis]));
        const float delta = std::max(1e-4f, 8.0f * std::numeric_limits<float>::epsilon() * scale);
        if (p_max[axis] - p_min[axis] < delta) {
            const float center = p_min[axis] + (p_max[axis] - p_min[axis]) * 0.5f;
            p_min[axis] = center - delta * 0.5f;
            p_max[axis] = center + delta * 0.5f;
        }
    }
}
