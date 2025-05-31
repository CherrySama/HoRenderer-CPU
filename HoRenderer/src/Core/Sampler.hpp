/*
    Created by Yinghao He on 2025-05-20
*/
#pragma once

#include "Util.hpp"
#include "Filter.hpp"


class Sampler {
public:
    Sampler(FilterType filter_type) {
        // Initialize with a basic seed
        std::random_device rd;
        generator_seed = rd();
        filter = Filter::Create(filter_type);
    }

    inline float random_float(float min, float max) const {
        // Returns a random real in [min,max).
        return min + (max - min) * random_float();
    }   

    inline Vector3f Reflect(const Vector3f& v,const Vector3f& n) {
        return v - 2.0f * glm::dot(v, n) * n;
    }

    inline Vector3f Refract(const Vector3f& uv, const Vector3f& n, float etai_over_etat) {
        auto cos_theta = glm::fmin(glm::dot(-uv, n), 1.0f);
        Vector3f r_out_perp = etai_over_etat * (uv + cos_theta * n);
        Vector3f r_out_parallel = -std::sqrt(std::fabs(1.0f - glm::length2(r_out_perp))) * n;
        return r_out_perp + r_out_parallel;
    }

    float random_float() const;
    // Generate random unit vectors for Lambertian reflection
    Vector3f random_unit_vector() const;
    Vector3f random_unit_2Dvector() const;

    Vector3f sample_square() const;
    void SetCurrentSample(int sample_index);
    
private:
    unsigned int generator_seed; // Only used to initialize thread-local generators
    int current_sample = 0;
    std::shared_ptr<Filter> filter;
};