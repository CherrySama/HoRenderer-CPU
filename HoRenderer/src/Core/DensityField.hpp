/*
    Spatial density fields for participating media.
*/
#pragma once

#include "AABB.hpp"


class DensityField {
public:
    virtual ~DensityField() = default;

    // Returns a non-negative scalar density in world space.
    virtual float Density(const Vector3f& world_position) const = 0;
    virtual float MaxDensity() const = 0;
    virtual const AABB& Bounds() const = 0;
};

class ConstantDensityField : public DensityField {
public:
    ConstantDensityField(const AABB& bounds, float density);

    float Density(const Vector3f& world_position) const override;
    float MaxDensity() const override { return density; }
    const AABB& Bounds() const override { return bounds; }

private:
    AABB bounds;
    float density;
};

class LinearDensityField : public DensityField {
public:
    LinearDensityField(const AABB& bounds,
                       int axis,
                       float density_at_min,
                       float density_at_max);

    float Density(const Vector3f& world_position) const override;
    float MaxDensity() const override { return max_density; }
    const AABB& Bounds() const override { return bounds; }

private:
    AABB bounds;
    int axis;
    float density_at_min;
    float density_at_max;
    float max_density;
};

class GridDensityField : public DensityField {
public:
    GridDensityField(const AABB& bounds,
                     const Vector3i& resolution,
                     std::vector<float> densities);

    float Density(const Vector3f& world_position) const override;
    float MaxDensity() const override { return max_density; }
    const AABB& Bounds() const override { return bounds; }
    const Vector3i& Resolution() const { return resolution; }

private:
    size_t Index(int x, int y, int z) const;

private:
    AABB bounds;
    Vector3i resolution;
    std::vector<float> densities;
    float max_density;
};

std::shared_ptr<GridDensityField> BuildFuzzyMeshDensityField(
    const Mesh& mesh,
    const Vector3i& resolution,
    float padding_fraction,
    int blur_iterations,
    float noise_frequency,
    float noise_strength);
