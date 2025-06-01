/*
    Created by Yinghao He on 2025-05-31
*/
#include "Filter.hpp"


std::shared_ptr<Filter> Filter::Create(const FilterType& type) {
    switch (type) {
        case FilterType::UNIFORM:
            return std::make_shared<UniformFilter>();
        case FilterType::GAUSSIAN:
            return std::make_shared<GaussianFilter>();
        case FilterType::TENT:
            return std::make_shared<TentFilter>();
    }
	return NULL;
}

Vector2f UniformFilter::SampleOffset(const Vector2f &sample)
{
    return sample - Vector2f(0.5f, 0.5f);
}

Vector2f GaussianFilter::SampleOffset(const Vector2f &sample)
{
    float r1 = std::max(1e-6f, sample.x);  // 避免log(0)
    float r = std::sqrt(-2.0f * std::log(r1));
    float theta = 2.0f * PI * sample.y;
    Vector2f gaussian_sample = r * Vector2f(std::cos(theta), std::sin(theta));
    
    return gaussian_sample * 0.375f;
}

Vector2f TentFilter::SampleOffset(const Vector2f &sample)
{
    Vector2f j = sample * 2.0f;
    
    float x = (j.x < 1.0f) ? std::sqrt(j.x) - 1.0f : 1.0f - std::sqrt(2.0f - j.x);
    float y = (j.y < 1.0f) ? std::sqrt(j.y) - 1.0f : 1.0f - std::sqrt(2.0f - j.y);
    
    return Vector2f(x, y);
}