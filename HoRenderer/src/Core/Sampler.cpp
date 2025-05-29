/*
    Created by Yinghao He on 2025-05-20
*/
#include "Sampler.hpp"

Vector3f Sampler::sample_square() const
{
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return Vector3f(random_float() - 0.5f, random_float() - 0.5f, 0.0f);
}

Vector3f Sampler::scale_color_single_sample(const Vector3f &pixel_color) const
{
    // For progressive rendering, don't scale by samples_per_pixel
    // Just apply gamma correction
    auto r = pixel_color.r;
    auto g = pixel_color.g;
    auto b = pixel_color.b;

    return Vector3f(glm::clamp(r, 0.0f, 0.999f),
                    glm::clamp(g, 0.0f, 0.999f),
                    glm::clamp(b, 0.0f, 0.999f));
}

void Sampler::SetCurrentSample(int sample_index)
{
    current_sample = sample_index;
}

