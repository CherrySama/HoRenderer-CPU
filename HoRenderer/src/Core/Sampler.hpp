/*
    Created by Yinghao He on 2025-05-20
*/
#pragma once

#include "Util.hpp"
#include "Filter.hpp"

struct SamplerParams {
    FilterType filter_type;
};

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