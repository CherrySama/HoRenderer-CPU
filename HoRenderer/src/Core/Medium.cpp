/*
    Created by Yinghao He on 2025-06-06
*/
#include "Medium.hpp"
#include "Ray.hpp"
#include "Sampler.hpp"


namespace {

bool IsFiniteNonNegative(const Vector3f& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z) && value.x >= 0.0f &&
           value.y >= 0.0f && value.z >= 0.0f;
}

bool IsGrayscale(const Vector3f& value)
{
    constexpr float tolerance = 1e-6f;
    return std::abs(value.x - value.y) <= tolerance &&
           std::abs(value.x - value.z) <= tolerance;
}

} // namespace

Vector3f HomogeneousMedium::GetSigmaA(const Vector3f &p) const {
    return sigma_a;
}

Vector3f HomogeneousMedium::GetSigmaS(const Vector3f &p) const {
    return sigma_s;
}

Vector3f HomogeneousMedium::GetSigmaT(const Vector3f &p) const {
    return sigma_t;
}

std::shared_ptr<PhaseFunction> HomogeneousMedium::GetPhaseFunction() const {
    return phase_function;
}

bool HomogeneousMedium::IsHomogeneous() const {
    return true;
}

MediumSample HomogeneousMedium::Sample(const Ray& ray, float max_t, Sampler& sampler) const {
    MediumSample sample;
    const float max_sigma_t = std::max({sigma_t.x, sigma_t.y, sigma_t.z});
    if (max_sigma_t <= 0.0f) {
        return sample;
    }

    const int channel = std::min(static_cast<int>(sampler.random_float() * 3.0f), 2);
    const float sampled_sigma_t = sigma_t[channel];
    const float sampled_t = sampled_sigma_t > 0.0f
        ? -std::log(1.0f - sampler.random_float()) / sampled_sigma_t
        : Infinity;
    sample.scattered = sampled_t < max_t;
    sample.t = sample.scattered ? sampled_t : max_t;
    sample.position = ray.at(sample.t);

    const float distance = std::min(sample.t, std::numeric_limits<float>::max());
    const Vector3f transmittance(std::exp(-sigma_t.x * distance),
                                 std::exp(-sigma_t.y * distance),
                                 std::exp(-sigma_t.z * distance));
    const Vector3f density = sample.scattered
        ? sigma_t * transmittance
        : transmittance;
    const float sampling_probability = (density.x + density.y + density.z) / 3.0f;
    if (sampling_probability <= 0.0f || !std::isfinite(sampling_probability)) {
        sample.weight = Vector3f(0.0f);
        return sample;
    }

    sample.weight = sample.scattered
        ? transmittance * sigma_s / sampling_probability
        : transmittance / sampling_probability;
    return sample;
}

Vector3f HomogeneousMedium::Transmittance(const Ray& ray,
                                          float max_t,
                                          Sampler&) const {
    return Vector3f(std::exp(-sigma_t.x * max_t),
                    std::exp(-sigma_t.y * max_t),
                    std::exp(-sigma_t.z * max_t));
}

HeterogeneousMedium::HeterogeneousMedium(std::shared_ptr<DensityField> density_field,
                                         const Vector3f& sigma_s_base,
                                         const Vector3f& sigma_a_base,
                                         std::shared_ptr<PhaseFunction> phase)
    : density_field(std::move(density_field)),
      sigma_s_base(sigma_s_base),
      sigma_a_base(sigma_a_base),
      sigma_t_base(sigma_s_base + sigma_a_base),
      phase_function(std::move(phase)),
      majorant(0.0f)
{
    if (!this->density_field) {
        throw std::invalid_argument("HeterogeneousMedium requires a density field");
    }
    if (!phase_function) {
        throw std::invalid_argument("HeterogeneousMedium requires a phase function");
    }
    if (!IsFiniteNonNegative(sigma_s_base) || !IsFiniteNonNegative(sigma_a_base)) {
        throw std::invalid_argument("Medium coefficients must be finite and non-negative");
    }
    if (!IsGrayscale(sigma_s_base) || !IsGrayscale(sigma_a_base)) {
        throw std::invalid_argument("First heterogeneous-medium version supports grayscale coefficients only");
    }

    const float max_density = this->density_field->MaxDensity();
    if (!std::isfinite(max_density) || max_density < 0.0f) {
        throw std::invalid_argument("Density majorant must be finite and non-negative");
    }
    majorant = max_density * sigma_t_base.x;
}

float HeterogeneousMedium::LookupDensity(const Vector3f& p) const
{
    const float density = density_field->Density(p);
    if (!std::isfinite(density)) {
        return 0.0f;
    }
    return glm::clamp(density, 0.0f, density_field->MaxDensity());
}

bool HeterogeneousMedium::IntersectDensityBounds(const Ray& ray,
                                                 float max_t,
                                                 Vector2f& interval) const
{
    interval = Vector2f(0.0f, max_t);
    return density_field->Bounds().isHit(ray, interval);
}

Vector3f HeterogeneousMedium::GetSigmaA(const Vector3f& p) const
{
    return LookupDensity(p) * sigma_a_base;
}

Vector3f HeterogeneousMedium::GetSigmaS(const Vector3f& p) const
{
    return LookupDensity(p) * sigma_s_base;
}

Vector3f HeterogeneousMedium::GetSigmaT(const Vector3f& p) const
{
    return LookupDensity(p) * sigma_t_base;
}

std::shared_ptr<PhaseFunction> HeterogeneousMedium::GetPhaseFunction() const
{
    return phase_function;
}

MediumSample HeterogeneousMedium::Sample(const Ray& ray,
                                         float max_t,
                                         Sampler& sampler) const
{
    MediumSample sample;
    Vector2f interval;
    if (majorant <= Epsilon || !IntersectDensityBounds(ray, max_t, interval)) {
        return sample;
    }

    float t = interval.x;
    while (true) {
        const float u = sampler.random_float();
        t += -std::log(1.0f - u) / majorant;
        if (t >= interval.y) {
            return sample;
        }

        const float sigma_t = LookupDensity(ray.at(t)) * sigma_t_base.x;
        const float acceptance_probability = glm::clamp(sigma_t / majorant,
                                                         0.0f,
                                                         1.0f);
        if (sampler.random_float() < acceptance_probability) {
            sample.scattered = true;
            sample.t = t;
            sample.position = ray.at(t);
            sample.weight = sigma_t_base.x > Epsilon
                ? sigma_s_base / sigma_t_base.x
                : Vector3f(0.0f);
            return sample;
        }
    }
}

Vector3f HeterogeneousMedium::Transmittance(const Ray& ray,
                                            float max_t,
                                            Sampler& sampler) const
{
    Vector2f interval;
    if (majorant <= Epsilon || !IntersectDensityBounds(ray, max_t, interval)) {
        return Vector3f(1.0f);
    }

    Vector3f transmittance(1.0f);
    float t = interval.x;
    while (true) {
        const float u = sampler.random_float();
        t += -std::log(1.0f - u) / majorant;
        if (t >= interval.y) {
            return transmittance;
        }

        const Vector3f sigma_t = GetSigmaT(ray.at(t));
        transmittance *= Vector3f(1.0f) - sigma_t / majorant;
    }
}
