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
        thread_local static std::mt19937 thread_generator;
        thread_local static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        thread_local static int last_sample = -1;
        thread_local static bool initialized = false;

        if (!initialized || last_sample != current_sample) {
            thread_generator.seed(generator_seed + current_sample * 1000 + omp_get_thread_num() * 10000);
            last_sample = current_sample;
            initialized = true;
        }
                
        return distribution(thread_generator);
    }

    inline float random_float(float min, float max) const {
        // Returns a random real in [min,max).
        return min + (max - min) * random_float();
    }

    // Generate random unit vectors for Lambertian reflection
    inline Vector3f random_unit_vector() const {
        // Generate random points in the unit sphere using rejection sampling
        while (true) {
            auto p = Vector3f(
                random_float(-1.0f, 1.0f),
                random_float(-1.0f, 1.0f),
                random_float(-1.0f, 1.0f)
            );
            
            auto len_squared = glm::length2(p);
            // Avoid vectors very close to zero, 
            // and also make sure to stay within the unit sphere
            if (len_squared > 1e-6f && len_squared <= 1.0f)
                return p / std::sqrt(len_squared);
        }
    }

    inline Vector3f random_unit_2Dvector() const {
            float theta = random_float(0.0f, 2.0f * PI);
            float r = std::sqrt(random_float());
            return Vector3f(r * std::cos(theta), r * std::sin(theta), 0.0f);
    }

    inline Vector3f Reflect(const Vector3f& v,const Vector3f& n) {
        return v - 2 * glm::dot(v, n) * n;
    }

    inline Vector3f Refract(const Vector3f& uv, const Vector3f& n, float etai_over_etat) {
        auto cos_theta = glm::fmin(glm::dot(-uv, n), 1.0f);
        Vector3f r_out_perp = etai_over_etat * (uv + cos_theta * n);
        Vector3f r_out_parallel = -std::sqrt(std::fabs(1.0f - glm::length2(r_out_perp))) * n;
        return r_out_perp + r_out_parallel;
    }

    inline int get_samples_per_pixel() const { return samples_per_pixel; }
    
    Vector3f sample_square() const;
    Vector3f scale_color_single_sample(const Vector3f &pixel_color) const;
    void SetCurrentSample(int sample_index);
    
private:
    int samples_per_pixel; // Count of random samples for each pixel
    float pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    unsigned int generator_seed; // Only used to initialize thread-local generators

    int current_sample = 0;
};