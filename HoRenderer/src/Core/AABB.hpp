/*
    Created by Yinghao He on 2025-05-25
*/
#pragma once

#include "Util.hpp"

class AABB {
public:
    AABB() {}
    AABB(const Vector3f& a, const Vector3f& b);

    bool isHit(const Ray &r, Vector2f &t_interval) const;

    inline Vector3f min() const {
        return p_min;
    }
    inline Vector3f max() const {
        return p_max;
    }

private:
    void pad_to_minimums();
        
private:
    Vector3f p_min;
    Vector3f p_max;
};

inline AABB calculateSurroundingBox(AABB box0, AABB box1) {
    Vector3f pmin(std::min(box0.min().x, box1.min().x),
                      std::min(box0.min().y, box1.min().y),
                      std::min(box0.min().z, box1.min().z));

    Vector3f pmax(std::max(box0.max().x, box1.max().x),
                    std::max(box0.max().y, box1.max().y),
                    std::max(box0.max().z, box1.max().z));
    
    return AABB(pmin, pmax);
}