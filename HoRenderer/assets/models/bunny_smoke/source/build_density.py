"""Build the billowy bunny density asset. Blender only supplies OpenVDB/NumPy."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np
import openvdb

ROOT = Path(__file__).resolve().parent.parent


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--relief", type=float, default=1.0)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    args.output = args.output.resolve()
    if args.output == ROOT / "bunny_smoke.vdb" or args.output.exists():
        raise FileExistsError(f"Refusing to replace an existing asset: {args.output}")
    if not 0.1 <= args.relief <= 1.5:
        raise ValueError("Relief must be between 0.1 and 1.5")
    return args


def make_distance_field():
    source = openvdb.read(str(ROOT / "source/base_density.vdb"), "density")
    # Keep the source isovalue fixed so density iterations preserve alignment.
    points, triangles, quads = source.convertToPolygons(isovalue=0.035, adaptivity=0.0)
    spacing = float(source.transform.voxelSize()[0]) / 4.0
    distance = openvdb.FloatGrid.createLevelSetFromPolygons(
        points, triangles, quads, openvdb.createLinearTransform(voxelSize=spacing), halfWidth=32.0)
    lo, hi = distance.evalActiveVoxelBoundingBox()
    shape = tuple(hi[i] - lo[i] + 1 for i in range(3))
    if np.prod(shape, dtype=np.int64) > 60 * 1024 * 1024:
        raise ValueError(f"Density field exceeds renderer cache limit: {shape}")
    values = np.empty(shape, dtype=np.float32)
    distance.copyToArray(values, ijk=lo)
    print("DISTANCE", shape, spacing, float(values.min()), float(values.max()), flush=True)
    return values, lo, spacing


def noise(x, y, z, wavelength, seed):
    """Quintic-interpolated lattice noise, fixed in the VDB's world coordinates."""
    coords = [v / wavelength for v in (x, y, z)]
    cells = [np.floor(v).astype(np.int32) for v in coords]
    frac = [v - c for v, c in zip(coords, cells)]
    smooth = [f * f * f * (f * (f * 6 - 15) + 10) for f in frac]

    def lattice(dx, dy, dz):
        a, b, c = [(v + d).astype(np.uint32) for v, d in zip(cells, (dx, dy, dz))]
        h = a * np.uint32(73856093) ^ b * np.uint32(19349663) ^ c * np.uint32(83492791) ^ np.uint32(seed)
        h = (h ^ (h >> 16)) * np.uint32(0x7FEB352D)
        h = (h ^ (h >> 15)) * np.uint32(0x846CA68B)
        h ^= h >> 16
        return (h >> 8).astype(np.float32) * np.float32(1.0 / 16777216)

    result = np.zeros(np.broadcast_shapes(x.shape, y.shape, z.shape), dtype=np.float32)
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                weight = (smooth[0] if dx else 1 - smooth[0]) * (smooth[1] if dy else 1 - smooth[1])
                weight = weight * (smooth[2] if dz else 1 - smooth[2])
                result += lattice(dx, dy, dz) * weight
    return result


def build_density(distance, lo, spacing, relief):
    density = np.zeros_like(distance)
    x = ((np.arange(distance.shape[0], dtype=np.float32) + lo[0]) * spacing)[:, None, None]
    y = ((np.arange(distance.shape[1], dtype=np.float32) + lo[1]) * spacing)[None, :, None]
    for start in range(0, distance.shape[2], 12):
        stop = min(start + 12, distance.shape[2])
        z = ((np.arange(start, stop, dtype=np.float32) + lo[2]) * spacing)[None, None, :]
        coarse = noise(x, y, z, 0.012, 5101)
        medium = noise(x, y, z, 0.0045, 5102)
        fine = noise(x, y, z, 0.0016, 5103)
        # Offsets are distances, not opacity multiplication: valleys remain
        # visible in an optically thick medium and receive real self-shadowing.
        offset = relief * (0.028 * (coarse - 0.5) + 0.011 * (medium - 0.5) + 0.004 * (fine - 0.5))
        offset = np.clip(offset + 0.0015, -0.0035, 0.011)
        shell_distance = distance[:, :, start:stop] - offset
        coverage = np.clip(0.5 - shell_distance / 0.0009, 0, 1)
        coverage = coverage * coverage * (3 - 2 * coverage)
        core = coverage * np.clip(0.56 + 0.3 * coarse + 0.16 * medium, 0, 1)
        # Thin, lower-density irregular wisps outside the displaced surface.
        gate = np.clip((0.6 * fine + 0.4 * medium - 0.48) / 0.25, 0, 1)
        wisps = 0.15 * gate**2 * np.exp(-np.maximum(shell_distance, 0) / 0.0018)
        wisps *= shell_distance < 0.007
        density[:, :, start:stop] = np.maximum(core, wisps)
    density[density < 0.001] = 0
    return density


def save_density(args, density, lo, spacing):
    assert np.isfinite(density).all() and density.min() >= 0 and density.max() <= 1
    assert all(not np.any(np.take(density, edge, axis=axis)) for axis in range(3) for edge in (0, -1)), "Clipped density at generation boundary"
    grid = openvdb.FloatGrid(0.0)
    grid.name = "density"
    grid.gridClass = openvdb.GridClass.FOG_VOLUME
    grid.transform = openvdb.createLinearTransform(voxelSize=spacing)
    grid.copyFromArray(density, ijk=lo, tolerance=0.0)
    grid.prune(tolerance=0.0)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    openvdb.write(str(args.output), grid)
    restored = openvdb.read(str(args.output), "density")
    restored_values = np.empty_like(density)
    restored.copyToArray(restored_values, ijk=lo)
    assert np.array_equal(density, restored_values), "VDB round-trip changed density values"
    active = density[density > 0]
    report = {
        "file": args.output.name, "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
        "source": "source/base_density.vdb", "source_isovalue": 0.035,
        "relief": args.relief, "voxel_size": spacing,
        "positive_voxels": int(active.size), "density_min_max": [float(active.min()), float(active.max())],
        "positive_density_percentiles_10_50_90": np.percentile(active, [10, 50, 90]).tolist(),
        "index_bbox_inclusive": [list(v) for v in restored.evalActiveVoxelBoundingBox()],
        "finite_nonnegative": True, "generation_boundary_clear": True, "exact_vdb_roundtrip": True,
        "appearance_accepted": False,
    }
    args.output.with_suffix(".json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2), flush=True)


def main():
    args = parse_arguments()
    distance, lo, spacing = make_distance_field()
    density = build_density(distance, lo, spacing, args.relief)
    save_density(args, density, lo, spacing)


if __name__ == "__main__":
    main()
