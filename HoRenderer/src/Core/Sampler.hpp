/*
    Created by Yinghao He on 2025-05-20
*/
#pragma once

#include "Util.hpp"
#include "Filter.hpp"

extern const int SobolMatricesDim;      // 1024
extern const int SobolMatricesSize;     // 52
extern const uint32_t SobolMatrices[];  

struct SamplerParams {
    FilterType filter_type;
};

class Sampler {
public:
    Sampler(FilterType filter_type) {
        filter = Filter::Create(filter_type);
        current_sample = 0;
        pixel_x = 0;
        pixel_y = 0;
    }

    inline float random_float(float min, float max) const {
        // Returns a random real in [min,max).
        return min + (max - min) * random_float();
    }

    float random_float() const;
    Vector2f get_2d_sample() const;
    Vector3f random_unit_vector() const;
    Vector3f random_unit_2Dvector() const;
    Vector3f sample_square() const;

    Vector3f CosineSampleHemisphere(const Vector3f &normal) const;
    float CosinePdfHemisphere(float cos_theta) const;

    void SetCurrentSample(int sample_index);
    void SetPixel(int x, int y);
    
private:
    int current_sample;
    int pixel_x, pixel_y;
    std::shared_ptr<Filter> filter;

    mutable int dimension_pair_index = 0;
    mutable bool use_second_sample = false;
    mutable float cached_sample = 0.0f;
    mutable int last_sample = -1;
    mutable int last_pixel_x = -1;
    mutable int last_pixel_y = -1;
};

// Sobol Sampling core function
inline uint32_t SobolSample(uint64_t index, int dimension) {
    if (dimension >= SobolMatricesDim) 
        dimension = dimension % SobolMatricesDim;
    
    uint32_t result = 0;
    for (int bit = 0; bit < SobolMatricesSize && index; index >>= 1, ++bit) 
        if (index & 1) 
            result ^= SobolMatrices[dimension * SobolMatricesSize + bit];
        
    return result;
}