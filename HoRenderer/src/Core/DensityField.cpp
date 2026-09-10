/*
    Spatial density fields for participating media.
*/
#include "DensityField.hpp"
#include "Shape.hpp"


namespace {

bool Contains(const AABB& bounds, const Vector3f& p)
{
    const Vector3f p_min = bounds.min();
    const Vector3f p_max = bounds.max();
    return p.x >= p_min.x && p.x <= p_max.x &&
           p.y >= p_min.y && p.y <= p_max.y &&
           p.z >= p_min.z && p.z <= p_max.z;
}

float ValidateDensity(float density)
{
    if (!std::isfinite(density) || density < 0.0f) {
        throw std::invalid_argument("Density must be finite and non-negative");
    }
    return density;
}

} // namespace

ConstantDensityField::ConstantDensityField(const AABB& bounds, float density)
    : bounds(bounds), density(ValidateDensity(density))
{
}

float ConstantDensityField::Density(const Vector3f& world_position) const
{
    return Contains(bounds, world_position) ? density : 0.0f;
}

LinearDensityField::LinearDensityField(const AABB& bounds,
                                       int axis,
                                       float density_at_min,
                                       float density_at_max)
    : bounds(bounds),
      axis(axis),
      density_at_min(ValidateDensity(density_at_min)),
      density_at_max(ValidateDensity(density_at_max)),
      max_density(std::max(this->density_at_min, this->density_at_max))
{
    if (axis < 0 || axis > 2) {
        throw std::invalid_argument("Density axis must be 0, 1, or 2");
    }
}

float LinearDensityField::Density(const Vector3f& world_position) const
{
    if (!Contains(bounds, world_position)) {
        return 0.0f;
    }

    const float axis_min = bounds.min()[axis];
    const float axis_extent = bounds.max()[axis] - axis_min;
    if (axis_extent <= Epsilon) {
        return density_at_min;
    }

    const float u = glm::clamp((world_position[axis] - axis_min) / axis_extent,
                               0.0f,
                               1.0f);
    return glm::mix(density_at_min, density_at_max, u);
}

GridDensityField::GridDensityField(const AABB& bounds,
                                   const Vector3i& resolution,
                                   std::vector<float> densities)
    : bounds(bounds),
      resolution(resolution),
      densities(std::move(densities)),
      max_density(0.0f)
{
    if (resolution.x < 2 || resolution.y < 2 || resolution.z < 2) {
        throw std::invalid_argument("Density-grid resolution must be at least 2 on every axis");
    }

    const size_t expected_size = static_cast<size_t>(resolution.x) *
                                 static_cast<size_t>(resolution.y) *
                                 static_cast<size_t>(resolution.z);
    if (this->densities.size() != expected_size) {
        throw std::invalid_argument("Density-grid sample count does not match its resolution");
    }

    for (float& density : this->densities) {
        density = ValidateDensity(density);
        max_density = std::max(max_density, density);
    }
}

size_t GridDensityField::Index(int x, int y, int z) const
{
    return static_cast<size_t>(x) +
           static_cast<size_t>(resolution.x) *
               (static_cast<size_t>(y) +
                static_cast<size_t>(resolution.y) * static_cast<size_t>(z));
}

float GridDensityField::Density(const Vector3f& world_position) const
{
    if (!Contains(bounds, world_position)) {
        return 0.0f;
    }

    const Vector3f extent = bounds.max() - bounds.min();
    const Vector3f normalized = (world_position - bounds.min()) / extent;
    const Vector3f grid_position = normalized *
        Vector3f(resolution.x - 1, resolution.y - 1, resolution.z - 1);
    const Vector3i lower(glm::floor(grid_position));
    const Vector3i upper(std::min(lower.x + 1, resolution.x - 1),
                         std::min(lower.y + 1, resolution.y - 1),
                         std::min(lower.z + 1, resolution.z - 1));
    const Vector3f fraction = grid_position - Vector3f(lower);

    const float c000 = densities[Index(lower.x, lower.y, lower.z)];
    const float c100 = densities[Index(upper.x, lower.y, lower.z)];
    const float c010 = densities[Index(lower.x, upper.y, lower.z)];
    const float c110 = densities[Index(upper.x, upper.y, lower.z)];
    const float c001 = densities[Index(lower.x, lower.y, upper.z)];
    const float c101 = densities[Index(upper.x, lower.y, upper.z)];
    const float c011 = densities[Index(lower.x, upper.y, upper.z)];
    const float c111 = densities[Index(upper.x, upper.y, upper.z)];

    const float c00 = glm::mix(c000, c100, fraction.x);
    const float c10 = glm::mix(c010, c110, fraction.x);
    const float c01 = glm::mix(c001, c101, fraction.x);
    const float c11 = glm::mix(c011, c111, fraction.x);
    const float c0 = glm::mix(c00, c10, fraction.y);
    const float c1 = glm::mix(c01, c11, fraction.y);
    return glm::mix(c0, c1, fraction.z);
}

namespace {

bool ContainsAlongDirection(const Mesh& mesh,
                            const Vector3f& point,
                            const Vector3f& direction)
{
    Ray ray(point, glm::normalize(direction));
    int intersection_count = 0;
    constexpr int MaxIntersections = 4096;

    for (; intersection_count < MaxIntersections; ++intersection_count) {
        Hit_Payload hit;
        if (!mesh.isHit(ray, Vector2f(Epsilon, Infinity), hit)) {
            break;
        }
        ray = Ray(hit.p + ray.direction() * (8.0f * Epsilon), ray.direction());
    }

    return intersection_count < MaxIntersections &&
           (intersection_count % 2) == 1;
}

bool RobustlyContains(const Mesh& mesh, const Vector3f& point)
{
    static const std::array<Vector3f, 3> directions = {
        glm::normalize(Vector3f(1.0f, 0.371f, 0.529f)),
        glm::normalize(Vector3f(-0.217f, 1.0f, 0.413f)),
        glm::normalize(Vector3f(0.337f, -0.191f, 1.0f))
    };

    int inside_votes = 0;
    for (const Vector3f& direction : directions) {
        inside_votes += ContainsAlongDirection(mesh, point, direction) ? 1 : 0;
    }
    return inside_votes >= 2;
}

uint32_t HashGridPoint(int x, int y, int z)
{
    uint32_t hash = static_cast<uint32_t>(x) * 0x8da6b343u;
    hash ^= static_cast<uint32_t>(y) * 0xd8163841u;
    hash ^= static_cast<uint32_t>(z) * 0xcb1ab31fu;
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    return hash ^ (hash >> 16);
}

float LatticeNoise(int x, int y, int z)
{
    return static_cast<float>(HashGridPoint(x, y, z) & 0x00ffffffu) /
           static_cast<float>(0x01000000u);
}

float ValueNoise(const Vector3f& p)
{
    const Vector3i lower(glm::floor(p));
    const Vector3f f = p - Vector3f(lower);
    const Vector3f w = f * f * (Vector3f(3.0f) - 2.0f * f);

    float corners[2][2][2];
    for (int z = 0; z < 2; ++z) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                corners[x][y][z] =
                    LatticeNoise(lower.x + x, lower.y + y, lower.z + z);
            }
        }
    }

    const float c00 = glm::mix(corners[0][0][0], corners[1][0][0], w.x);
    const float c10 = glm::mix(corners[0][1][0], corners[1][1][0], w.x);
    const float c01 = glm::mix(corners[0][0][1], corners[1][0][1], w.x);
    const float c11 = glm::mix(corners[0][1][1], corners[1][1][1], w.x);
    return glm::mix(glm::mix(c00, c10, w.y),
                    glm::mix(c01, c11, w.y),
                    w.z);
}

float FractalNoise(Vector3f p)
{
    float value = 0.0f;
    float amplitude = 0.5f;
    float amplitude_sum = 0.0f;
    for (int octave = 0; octave < 4; ++octave) {
        value += amplitude * ValueNoise(p);
        amplitude_sum += amplitude;
        p = p * 2.03f + Vector3f(17.0f, 31.0f, 47.0f);
        amplitude *= 0.5f;
    }
    return value / amplitude_sum;
}

float CellularBillow(const Vector3f& p)
{
    const Vector3i cell(glm::floor(p));
    float minimum_distance_squared = Infinity;

    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const Vector3i neighbor = cell + Vector3i(dx, dy, dz);
                const Vector3f offset(
                    LatticeNoise(neighbor.x, neighbor.y, neighbor.z),
                    LatticeNoise(neighbor.y + 37, neighbor.z - 17, neighbor.x + 11),
                    LatticeNoise(neighbor.z - 29, neighbor.x + 23, neighbor.y + 41));
                const Vector3f feature_point = Vector3f(neighbor) + offset;
                const Vector3f delta = feature_point - p;
                minimum_distance_squared =
                    std::min(minimum_distance_squared, glm::dot(delta, delta));
            }
        }
    }

    return std::exp(-4.0f * minimum_distance_squared);
}

} // namespace

std::shared_ptr<GridDensityField> BuildFuzzyMeshDensityField(
    const Mesh& mesh,
    const Vector3i& resolution,
    float padding_fraction,
    int blur_iterations,
    float noise_frequency,
    float noise_strength)
{
    if (resolution.x < 2 || resolution.y < 2 || resolution.z < 2) {
        throw std::invalid_argument("Fuzzy mesh grid must have at least 2 samples per axis");
    }
    if (!std::isfinite(padding_fraction) || padding_fraction < 0.0f ||
        blur_iterations < 0 || !std::isfinite(noise_frequency) ||
        noise_frequency < 0.0f || !std::isfinite(noise_strength) ||
        noise_strength < 0.0f || noise_strength > 1.0f) {
        throw std::invalid_argument("Invalid fuzzy mesh density parameters");
    }

    const AABB mesh_bounds = mesh.getBoundingBox();
    const Vector3f mesh_extent = mesh_bounds.max() - mesh_bounds.min();
    const float padding = std::max({mesh_extent.x, mesh_extent.y, mesh_extent.z}) *
                          padding_fraction;
    const AABB grid_bounds(mesh_bounds.min() - Vector3f(padding),
                           mesh_bounds.max() + Vector3f(padding));
    const Vector3f grid_extent = grid_bounds.max() - grid_bounds.min();
    const size_t sample_count = static_cast<size_t>(resolution.x) *
                                static_cast<size_t>(resolution.y) *
                                static_cast<size_t>(resolution.z);
    std::vector<float> densities(sample_count, 0.0f);

    auto index = [&resolution](int x, int y, int z) {
        return static_cast<size_t>(x) +
               static_cast<size_t>(resolution.x) *
                   (static_cast<size_t>(y) +
                    static_cast<size_t>(resolution.y) * static_cast<size_t>(z));
    };

    for (int z = 0; z < resolution.z; ++z) {
        for (int y = 0; y < resolution.y; ++y) {
            for (int x = 0; x < resolution.x; ++x) {
                const Vector3f normalized(
                    static_cast<float>(x) / static_cast<float>(resolution.x - 1),
                    static_cast<float>(y) / static_cast<float>(resolution.y - 1),
                    static_cast<float>(z) / static_cast<float>(resolution.z - 1));
                const Vector3f p = grid_bounds.min() + normalized * grid_extent;
                densities[index(x, y, z)] = RobustlyContains(mesh, p) ? 1.0f : 0.0f;
            }
        }
    }

    std::vector<float> blurred(sample_count);
    for (int iteration = 0; iteration < blur_iterations; ++iteration) {
        for (int z = 0; z < resolution.z; ++z) {
            for (int y = 0; y < resolution.y; ++y) {
                for (int x = 0; x < resolution.x; ++x) {
                    float sum = 0.0f;
                    int count = 0;
                    for (int dz = -1; dz <= 1; ++dz) {
                        const int nz = z + dz;
                        if (nz < 0 || nz >= resolution.z) {
                            continue;
                        }
                        for (int dy = -1; dy <= 1; ++dy) {
                            const int ny = y + dy;
                            if (ny < 0 || ny >= resolution.y) {
                                continue;
                            }
                            for (int dx = -1; dx <= 1; ++dx) {
                                const int nx = x + dx;
                                if (nx < 0 || nx >= resolution.x) {
                                    continue;
                                }
                                sum += densities[index(nx, ny, nz)];
                                ++count;
                            }
                        }
                    }
                    blurred[index(x, y, z)] = sum / static_cast<float>(count);
                }
            }
        }
        densities.swap(blurred);
    }

    for (int z = 0; z < resolution.z; ++z) {
        for (int y = 0; y < resolution.y; ++y) {
            for (int x = 0; x < resolution.x; ++x) {
                const Vector3f normalized(
                    static_cast<float>(x) / static_cast<float>(resolution.x - 1),
                    static_cast<float>(y) / static_cast<float>(resolution.y - 1),
                    static_cast<float>(z) / static_cast<float>(resolution.z - 1));
                const float coarse_billow =
                    CellularBillow(normalized * (0.58f * noise_frequency));
                const float fine_billow =
                    CellularBillow(normalized * (1.45f * noise_frequency) +
                                   Vector3f(13.0f, 7.0f, 19.0f));
                const float turbulence =
                    FractalNoise(normalized * (0.9f * noise_frequency));
                const float detail = glm::clamp(
                    0.62f * coarse_billow +
                    0.23f * fine_billow +
                    0.15f * turbulence,
                    0.0f,
                    1.0f);
                float& density = densities[index(x, y, z)];
                const float support = glm::clamp(4.0f * density, 0.0f, 1.0f);
                const float displaced_shape = glm::clamp(
                    density + support * noise_strength * 0.62f *
                                  (coarse_billow - 0.38f),
                    0.0f,
                    1.0f);
                const float puff_mask = glm::smoothstep(0.12f,
                                                        0.68f,
                                                        coarse_billow);
                const float modulation =
                    (0.04f + 0.96f * puff_mask) *
                    (0.62f + 0.48f * detail);
                density = glm::clamp(displaced_shape * modulation,
                                     0.0f,
                                     1.0f);
            }
        }
    }

    return std::make_shared<GridDensityField>(grid_bounds,
                                              resolution,
                                              std::move(densities));
}
