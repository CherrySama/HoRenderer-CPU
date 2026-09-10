#include "VdbDensityField.hpp"

#include <openvdb/openvdb.h>
#include <openvdb/io/File.h>

using Matrix4d = glm::dmat4;

struct VdbDensityField::Data {
    std::unique_ptr<GridDensityField> index_field;
    Matrix4d world_to_index;
    AABB world_bounds;
};

VdbDensityField::VdbDensityField(const std::string& filename,
                                 const Transform& volume_to_world,
                                 const std::string& grid_name)
    : data(std::make_unique<Data>())
{
    openvdb::initialize();
    openvdb::io::File file(filename);
    file.open();
    if (!file.hasGrid(grid_name)) {
        throw std::runtime_error("VDB grid not found: " + grid_name + " in " + filename);
    }
    const auto base = file.readGrid(grid_name);
    file.close();
    const auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(base);
    if (!grid || grid->getGridClass() != openvdb::GRID_FOG_VOLUME) {
        throw std::invalid_argument("VDB density must be a FloatGrid with fog-volume class");
    }
    if (grid->background() != 0.0f || grid->empty() || !grid->transform().isLinear()) {
        throw std::invalid_argument("VDB requires a nonempty grid, zero background and affine transform");
    }
    // Nonzero inactive values outside active bounds would invalidate both bounds
    // and the majorant. Reject those grids instead of silently dropping density.
    for (auto it = grid->cbeginValueAll(); it; ++it) {
        const float value = *it;
        if (!std::isfinite(value) || value < 0.0f || (!it.isValueOn() && value != 0.0f)) {
            throw std::invalid_argument("VDB contains invalid density or nonzero inactive values");
        }
    }

    const auto box = grid->evalActiveVoxelBoundingBox();
    // Box interpolation has support one voxel beyond the outermost sample
    // centers, not half a voxel. Include zero samples on all six sides.
    Vector3i lower, upper, resolution;
    size_t count = 1;
    constexpr size_t MaxCachedVoxels = 64 * 1024 * 1024;
    for (int axis = 0; axis < 3; ++axis) {
        const int64_t lo = int64_t(box.min()[axis]) - 1;
        const int64_t hi = int64_t(box.max()[axis]) + 1;
        const int64_t n = hi - lo + 1;
        if (lo < std::numeric_limits<int>::min() || hi > std::numeric_limits<int>::max() ||
            n < 2 || uint64_t(n) > MaxCachedVoxels / count) {
            throw std::invalid_argument("VDB exceeds the 256 MiB dense sampling cache limit");
        }
        lower[axis] = int(lo);
        upper[axis] = int(hi);
        resolution[axis] = int(n);
        count *= size_t(n);
    }

    Matrix4d index_to_volume(1.0);
    const auto origin = grid->indexToWorld(openvdb::Vec3d(0));
    for (int axis = 0; axis < 3; ++axis) {
        openvdb::Vec3d unit(0);
        unit[axis] = 1.0;
        const auto basis = grid->indexToWorld(unit) - origin;
        index_to_volume[axis] = Vector4d(basis.x(), basis.y(), basis.z(), 0.0);
    }
    index_to_volume[3] = Vector4d(origin.x(), origin.y(), origin.z(), 1.0);
    const Matrix4d index_to_world = Matrix4d(volume_to_world.Mat()) * index_to_volume;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (!std::isfinite(index_to_world[col][row])) {
                throw std::invalid_argument("Nonfinite VDB instance transform");
            }
        }
    }
    if (index_to_world[0][3] != 0 || index_to_world[1][3] != 0 ||
        index_to_world[2][3] != 0 || index_to_world[3][3] != 1 ||
        glm::determinant(index_to_world) == 0.0) {
        throw std::invalid_argument("VDB instance transform must be affine and invertible");
    }
    data->world_to_index = glm::inverse(index_to_world);
    Vector3f world_min(Infinity), world_max(-Infinity);
    for (int corner = 0; corner < 8; ++corner) {
        const Vector4d index((corner & 1) ? upper.x : lower.x,
                             (corner & 2) ? upper.y : lower.y,
                             (corner & 4) ? upper.z : lower.z, 1.0);
        const Vector3f point(index_to_world * index);
        world_min = glm::min(world_min, point);
        world_max = glm::max(world_max, point);
    }
    data->world_bounds = AABB(world_min, world_max);

    // This asset is small (~8 MiB dense). Cache samples once, with no resampling,
    // preserving negative indices and active tiles. Lookups are then immutable
    // and thread-safe under the integrator's OpenMP workers.
    std::vector<float> values(count);
    const auto accessor = grid->getConstAccessor();
    size_t offset = 0;
    for (int z = 0; z < resolution.z; ++z) {
        for (int y = 0; y < resolution.y; ++y) {
            for (int x = 0; x < resolution.x; ++x) {
                values[offset++] = accessor.getValue(
                    openvdb::Coord(lower.x + x, lower.y + y, lower.z + z));
            }
        }
    }
    data->index_field = std::make_unique<GridDensityField>(
        AABB(Vector3f(lower), Vector3f(upper)), resolution, std::move(values));
}

VdbDensityField::~VdbDensityField() = default;

float VdbDensityField::Density(const Vector3f& world_position) const
{
    const Vector3f index(data->world_to_index * Vector4d(world_position, 1.0));
    return data->index_field->Density(index);
}

float VdbDensityField::MaxDensity() const { return data->index_field->MaxDensity(); }
const AABB& VdbDensityField::Bounds() const { return data->world_bounds; }
