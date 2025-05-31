/*
    Created by Yinghao He on 2025-05-20
*/
#include "Sampler.hpp"

float Sampler::random_float() const
{
    thread_local static std::mt19937 thread_generator;
    thread_local static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    thread_local static int last_sample = -1;
    thread_local static bool initialized = false;
    thread_local static int thread_id = omp_get_thread_num();

    if (!initialized || last_sample != current_sample) {
        thread_generator.seed(generator_seed + current_sample * 1000 + thread_id * 10000);
        last_sample = current_sample;
        initialized = true;
    }
            
    return distribution(thread_generator);        
}

Vector3f Sampler::random_unit_vector() const
{
    float z = random_float(-1.0f, 1.0f);
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2.0f * PI * random_float();
    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}

Vector3f Sampler::random_unit_2Dvector() const
{
    float theta = random_float(0.0f, 2.0f * PI);
    float r = std::sqrt(random_float());
    return Vector3f(r * std::cos(theta), r * std::sin(theta), 0.0f);
}

Vector3f Sampler::sample_square() const
{
    Vector2f sample = Vector2f(random_float(), random_float());
    sample = filter->SampleOffset(sample);
    return Vector3f(sample.x, sample.y, 0.0f);
}

void Sampler::SetCurrentSample(int sample_index)
{
    current_sample = sample_index;
}

