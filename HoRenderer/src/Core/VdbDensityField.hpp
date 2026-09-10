#pragma once

#include "DensityField.hpp"
#include "Transform.hpp"

// Immutable after loading, so concurrent density queries share no mutable cache.
class VdbDensityField final : public DensityField {
public:
    explicit VdbDensityField(const std::string& filename,
                             const Transform& volume_to_world = Transform(),
                             const std::string& grid_name = "density");
    ~VdbDensityField() override;

    float Density(const Vector3f& world_position) const override;
    float MaxDensity() const override;
    const AABB& Bounds() const override;

private:
    struct Data;
    std::unique_ptr<Data> data;
};
