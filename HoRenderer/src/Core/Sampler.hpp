/*
    Created by Yinghao He on 2025-05-20
*/
#pragma once

#include "Util.hpp"

class Sampler {
public:
    Sampler(int sample) : samples_per_pixel(sample) {
        pixel_samples_scale = 1.0f / float(samples_per_pixel);
        // Initialize with a basic seed
        std::random_device rd;
        generator_seed = rd();
    }
    
    inline float random_float() const {
        // Using thread_local to ensure each thread has its own generator
        thread_local static std::mt19937 thread_generator(generator_seed + omp_get_thread_num() * 1000);
        thread_local static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        
        return distribution(thread_generator);
    }

    inline float random_float(float min, float max) const {
        // Returns a random real in [min,max).
        return min + (max - min) * random_float();
    }

    inline int get_samples_per_pixel() const { return samples_per_pixel; }
    
    Vector3f sample_square() const;
    Vector3f scale_color(const Vector3f &pixel_color) const;

private:
    int samples_per_pixel; // Count of random samples for each pixel
    float pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    unsigned int generator_seed; // Only used to initialize thread-local generators
};