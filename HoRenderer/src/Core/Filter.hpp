/*
    Created by Yinghao He on 2025-05-31
*/
#pragma once

#include "Util.hpp"

enum class FilterType {
    UNIFORM,  // Original uniform distribution
    GAUSSIAN, // Gaussian Distribution
    TENT      // Triangular distribution
};

class Filter {
public:
    Filter(FilterType type) : filter_type(type) {}
    virtual Vector2f SampleOffset(const Vector2f &sample) = 0;
    static std::shared_ptr<Filter> Create(const FilterType &type);

private:
    FilterType filter_type;
};

class Uniform : public Filter {
public:
    Uniform() : Filter(FilterType::UNIFORM) {}
    virtual Vector2f SampleOffset(const Vector2f& sample) override;
};

class Gaussian : public Filter {
public:
    Gaussian() : Filter(FilterType::GAUSSIAN) {}
    virtual Vector2f SampleOffset(const Vector2f& sample) override;
};

class Tent : public Filter {
public:
    Tent() : Filter(FilterType::TENT) {}
    virtual Vector2f SampleOffset(const Vector2f& sample) override;
};