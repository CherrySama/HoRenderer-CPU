/*
    Created by Yinghao He on 2025-05-20
*/
#include "Sampler.hpp"

Vector3f Sampler::sample_square() const
{
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return Vector3f(random_float() - 0.5f, random_float() - 0.5f, 0.0f);
}

Vector3f Sampler::scale_color(const Vector3f &pixel_color) const
{
    auto r = pixel_color.r * pixel_samples_scale;
    auto g = pixel_color.g * pixel_samples_scale;
    auto b = pixel_color.b * pixel_samples_scale;

    // 进行gamma=2的校正 (简单取平方根)
    // r = r > 0.0f ? std::sqrt(r) : 0.0f;
    // g = g > 0.0f ? std::sqrt(g) : 0.0f;
    // b = b > 0.0f ? std::sqrt(b) : 0.0f;

    // 将颜色限制在[0, 1]范围内
    return Vector3f(std::clamp(r, 0.0f, 0.999f),
                    std::clamp(g, 0.0f, 0.999f),
                    std::clamp(b, 0.0f, 0.999f));
}
