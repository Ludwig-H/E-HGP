#!/usr/bin/env python3
"""Materialized deterministic dyadic clouds for quality campaign v2.

Every PointId is written as three little-endian signed 64-bit numerators with
the common denominator 2^52.  Generation is block-streamed and keeps only a
bounded coordinate block in memory; truth labels are produced outside the
MorseHGP3D producer.
"""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
from typing import Any
import zlib

from campaign_runtime import atomic_publish, canonical_bytes, sha256_bytes


CLOUD_SCHEMA = "morsehgp3d.point_quality.dyadic_cloud_manifest.v2"
LABEL_RLE_SCHEMA = "morsehgp3d.point_quality.label_rle.v2"
GENERATOR_ID = "counter_based_splitmix64_dyadic_cloud_v2"
COORDINATE_RECORD_BYTES = 24
COORDINATE_DENOMINATOR_EXPONENT = 52
COORDINATE_DENOMINATOR = 1 << COORDINATE_DENOMINATOR_EXPONENT
BLOCK_POINT_COUNT = 262_144
RAW_CLOUD_DIGEST_DOMAIN = b"MorseHGP3D/point-quality/raw-dyadic-i64x3/v2\x00"


def _cluster_sizes(dataset: dict[str, Any], point_count: int) -> tuple[int, list[int]]:
    noise = point_count * dataset["true_noise_fraction_numerator"] // dataset["true_noise_fraction_denominator"]
    cluster_count = int(dataset["true_cluster_count"])
    inlier_count = point_count - noise
    if cluster_count <= 0 or inlier_count < cluster_count:
        raise ValueError("dyadic truth generator cannot create nonempty clusters")
    target_ratio = int(dataset["parameters"].get("cluster_size_ratio", 1))
    if target_ratio <= 1:
        quotient, remainder = divmod(inlier_count, cluster_count)
        return noise, [quotient + (1 if label < remainder else 0) for label in range(cluster_count)]
    denominator = cluster_count - 1 + target_ratio
    minimum = inlier_count // denominator
    if minimum <= 0:
        raise ValueError("dyadic truth generator cannot realize the requested size ratio")
    sizes = [minimum] * cluster_count
    sizes[-1] = minimum * target_ratio
    remainder = inlier_count - sum(sizes)
    cursor = 1
    while remainder:
        if cursor >= cluster_count - 1:
            cursor = 1
        capacity = sizes[-1] - sizes[cursor]
        if capacity:
            addition = min(remainder, capacity)
            sizes[cursor] += addition
            remainder -= addition
        cursor += 1
    if sum(sizes) != inlier_count or min(sizes) <= 0 or max(sizes) != target_ratio * min(sizes):
        raise ValueError("dyadic truth size-ratio invariant failed")
    return noise, sizes


def truth_runs(dataset: dict[str, Any], point_count: int) -> list[list[int]]:
    """Return the canonical contiguous truth partition without O(n) storage."""

    noise, sizes = _cluster_sizes(dataset, point_count)
    runs: list[list[int]] = []
    begin = 0
    if noise:
        runs.append([0, noise, -1])
        begin = noise
    for label, size in enumerate(sizes):
        runs.append([begin, begin + size, label])
        begin += size
    if begin != point_count:
        raise ValueError("dyadic truth generator did not cover every PointId")
    return runs


def _lattice_center(index: int, dimensions: tuple[int, int, int], spacing: int) -> tuple[int, int, int]:
    x_index = index % dimensions[0]
    y_index = (index // dimensions[0]) % dimensions[1]
    z_index = index // (dimensions[0] * dimensions[1])
    if z_index >= dimensions[2]:
        raise ValueError("dyadic center lattice is too small")
    return tuple((2 * item - (side - 1)) * spacing // 2 for item, side in zip((x_index, y_index, z_index), dimensions))


def _bridge_counts(dataset: dict[str, Any], sizes: list[int]) -> list[int]:
    numerator = int(dataset["parameters"]["bridge_fraction_numerator"])
    denominator = int(dataset["parameters"]["bridge_fraction_denominator"])
    total = sum(sizes) * numerator // denominator
    counts = [total * size // sum(sizes) for size in sizes]
    residues = sorted(range(len(sizes)), key=lambda label: (-(total * sizes[label] % sum(sizes)), label))
    for label in residues[: total - sum(counts)]:
        counts[label] += 1
    if sum(counts) != total or any(count > size for count, size in zip(counts, sizes)):
        raise ValueError("dyadic bridge allocation invariant failed")
    return counts


def geometry_contract(dataset: dict[str, Any], point_count: int) -> dict[str, Any]:
    """Return exact, independently checkable geometry properties for a profile."""

    _noise, sizes = _cluster_sizes(dataset, point_count)
    parameters = dataset["parameters"]
    profile = dataset["generator"]
    size_profile = {"maximum": max(sizes), "minimum": min(sizes), "sizes": sizes, "target_ratio": int(parameters.get("cluster_size_ratio", 1))}
    if size_profile["target_ratio"] > 1 and size_profile["maximum"] != size_profile["target_ratio"] * size_profile["minimum"]:
        raise ValueError("cluster-size ratio is not observable")
    if profile == "dyadic_separated_balls_v2":
        side = int(parameters["center_grid_side"])
        radius = 1 << (COORDINATE_DENOMINATOR_EXPONENT - int(parameters["ball_radius_denominator_exponent"]))
        minimum_separation = 1 << (COORDINATE_DENOMINATOR_EXPONENT - int(parameters["minimum_center_separation_denominator_exponent"]))
        spacing = 1 << 51
        if side ** 3 != len(sizes) or spacing - 2 * radius < minimum_separation:
            raise ValueError("separated-ball geometry contract failed")
        shape = {"center_grid_dimensions": [side, side, side], "center_spacing_numerator": spacing, "euclidean_ball_radius_upper_bound_numerator": radius, "kind": "separated_balls", "minimum_surface_separation_numerator": minimum_separation, "offset_construction": "inscribed_cube_linf_radius_floor_r_over_2"}
    elif profile == "dyadic_balanced_multiscale_clusters_v2":
        side = int(parameters["center_grid_side"])
        level_count = int(parameters["radius_scale_level_count"])
        ratio = int(parameters["radius_scale_ratio"])
        radius_unit = 1 << 32
        radii = [radius_unit * (7 + 63 * level) for level in range(level_count)]
        spacing = 1 << 50
        if side ** 3 != len(sizes) or level_count != 8 or radii[-1] != ratio * radii[0] or len(set(radii)) != level_count or spacing <= 2 * radii[-1]:
            raise ValueError("multiscale geometry contract failed")
        shape = {"center_grid_dimensions": [side, side, side], "center_spacing_numerator": spacing, "euclidean_radius_upper_bound_numerators": radii, "kind": "multiscale_balls", "offset_construction": "inscribed_cube_linf_radius_floor_r_over_2", "radius_scale_ratio": ratio}
    elif profile == "dyadic_piecewise_linear_tubes_v2":
        dimensions = (4, 3, 2)
        segment_count = int(parameters["segment_count_per_filament"])
        tube_radius = 1 << (COORDINATE_DENOMINATOR_EXPONENT - int(parameters["tube_radius_denominator_exponent"]))
        minimum_separation = 1 << (COORDINATE_DENOMINATOR_EXPONENT - int(parameters["minimum_filament_separation_denominator_exponent"]))
        spacing = 1 << 48
        half_length = 1 << 43
        if math_product(dimensions) != len(sizes) or segment_count != 8 or spacing - 2 * (half_length + tube_radius) < minimum_separation:
            raise ValueError("filament geometry contract failed")
        shape = {"center_grid_dimensions": list(dimensions), "center_spacing_numerator": spacing, "kind": "piecewise_linear_tubes", "longitudinal_half_length_numerator": half_length, "minimum_centerline_separation_numerator": minimum_separation, "segment_count": segment_count, "tube_radius_numerator": tube_radius}
    elif profile == "dyadic_imbalanced_bridged_pairs_v2":
        dimensions = (4, 4, 3)
        pair_count = len(sizes) // 2
        pair_separation = 1 << (COORDINATE_DENOMINATOR_EXPONENT - int(parameters["paired_center_separation_denominator_exponent"]))
        minimum_inter_pair = 1 << (COORDINATE_DENOMINATOR_EXPONENT - int(parameters["inter_pair_separation_denominator_exponent"]))
        tube_radius = 1 << (COORDINATE_DENOMINATOR_EXPONENT - int(parameters["bridge_tube_radius_denominator_exponent"]))
        cluster_radius = 1 << 40
        spacing = 1 << 51
        bridge_counts = _bridge_counts(dataset, sizes)
        if pair_count != math_product(dimensions) or len(sizes) % 2 or spacing - pair_separation - 2 * cluster_radius < minimum_inter_pair or tube_radius * 2 >= pair_separation or sum(bridge_counts) <= 0:
            raise ValueError("bridged-pair geometry contract failed")
        shape = {"bridge_fraction_denominator": int(parameters["bridge_fraction_denominator"]), "bridge_fraction_numerator": int(parameters["bridge_fraction_numerator"]), "bridge_point_count": sum(bridge_counts), "bridge_points_per_cluster": bridge_counts, "bridge_tube_radius_numerator": tube_radius, "cluster_ball_radius_numerator": cluster_radius, "kind": "bridged_pairs", "minimum_inter_pair_separation_numerator": minimum_inter_pair, "pair_center_grid_dimensions": list(dimensions), "pair_center_spacing_numerator": spacing, "paired_center_separation_numerator": pair_separation}
    else:
        raise ValueError("unknown dyadic quality profile")
    return {"cluster_size_profile": size_profile, "coordinate_denominator_exponent": COORDINATE_DENOMINATOR_EXPONENT, "shape": shape}


def math_product(values: tuple[int, int, int]) -> int:
    return values[0] * values[1] * values[2]


def _bounded_offsets(local: Any, radius: int, seed: int, numpy: Any) -> tuple[Any, Any, Any]:
    half = max(1, radius // 2)
    width = 2 * half + 1
    return (
        (local * numpy.int64(2_654_435_761) + seed) % width - half,
        (local * numpy.int64(2_246_822_519) + seed * 3) % width - half,
        (local * numpy.int64(3_266_489_917) + seed * 5) % width - half,
    )


def _coordinate_block(dataset: dict[str, Any], contract: dict[str, Any], label: int, local: Any, numpy: Any) -> Any:
    seed = int(dataset["seed"])
    coordinates = numpy.empty((len(local), 3), dtype="<i8")
    if label < 0:
        half = 1 << 52
        offsets = _bounded_offsets(local, 2 * half, seed, numpy)
        for axis in range(3):
            coordinates[:, axis] = offsets[axis]
        return coordinates
    shape = contract["shape"]
    if shape["kind"] in {"separated_balls", "multiscale_balls"}:
        dimensions = tuple(shape["center_grid_dimensions"])
        center = _lattice_center(label, dimensions, shape["center_spacing_numerator"])
        radius = shape["euclidean_ball_radius_upper_bound_numerator"] if shape["kind"] == "separated_balls" else shape["euclidean_radius_upper_bound_numerators"][label % len(shape["euclidean_radius_upper_bound_numerators"])]
        offsets = _bounded_offsets(local, radius, seed + label * 17, numpy)
        for axis in range(3):
            coordinates[:, axis] = center[axis] + offsets[axis]
        return coordinates
    if shape["kind"] == "piecewise_linear_tubes":
        center = _lattice_center(label, tuple(shape["center_grid_dimensions"]), shape["center_spacing_numerator"])
        half_length = shape["longitudinal_half_length_numerator"]
        longitudinal = (local * numpy.int64(2_654_435_761) + seed) % (2 * half_length + 1) - half_length
        segment_width = max(1, (2 * half_length + 1) // shape["segment_count"])
        segment = (longitudinal + half_length) // segment_width
        segment = numpy.minimum(segment, shape["segment_count"] - 1)
        zigzag = ((segment % 2) * 2 - 1) * (half_length // 16)
        offsets = _bounded_offsets(local, shape["tube_radius_numerator"], seed + label * 19, numpy)
        coordinates[:, 0] = center[0] + longitudinal
        coordinates[:, 1] = center[1] + zigzag + offsets[1]
        coordinates[:, 2] = center[2] + offsets[2]
        return coordinates
    if shape["kind"] == "bridged_pairs":
        pair = label // 2
        pair_center = _lattice_center(pair, tuple(shape["pair_center_grid_dimensions"]), shape["pair_center_spacing_numerator"])
        separation = shape["paired_center_separation_numerator"]
        center = (pair_center[0] + (-separation // 2 if label % 2 == 0 else separation // 2), pair_center[1], pair_center[2])
        bridge_count = shape["bridge_points_per_cluster"][label]
        bridge_mask = local < bridge_count
        offsets = _bounded_offsets(local, shape["cluster_ball_radius_numerator"], seed + label * 23, numpy)
        for axis in range(3):
            coordinates[:, axis] = center[axis] + offsets[axis]
        if numpy.any(bridge_mask):
            bridge_local = local[bridge_mask]
            bridge_offsets = _bounded_offsets(bridge_local, shape["bridge_tube_radius_numerator"], seed + label * 29, numpy)
            coordinates[bridge_mask, 0] = pair_center[0] - separation // 2 + ((bridge_local * numpy.int64(2_654_435_761) + seed) % (separation + 1))
            coordinates[bridge_mask, 1] = pair_center[1] + bridge_offsets[1]
            coordinates[bridge_mask, 2] = pair_center[2] + bridge_offsets[2]
        return coordinates
    raise ValueError("unknown materialized dyadic geometry")


def generate(*, dataset: dict[str, Any], point_count: int, generator_sha256: str, output_directory: Path) -> dict[str, Any]:
    """Materialize every dyadic coordinate and publish its authenticated stream."""

    import numpy as np

    truth = {"point_count": point_count, "runs": truth_runs(dataset, point_count), "schema": LABEL_RLE_SCHEMA, "semantics": "ground_truth"}
    truth_payload = canonical_bytes(truth)
    truth_path = output_directory / "truth-labels.rle.json"
    atomic_publish(truth_path, truth_payload)
    truth_sha256 = sha256_bytes(truth_payload)
    contract = geometry_contract(dataset, point_count)
    coordinate_path = output_directory / "raw-cloud.i64x3.zlib"
    raw_digest = hashlib.sha256(RAW_CLOUD_DIGEST_DOMAIN)
    compressed_digest = hashlib.sha256()
    compressed_size = 0
    compressor = zlib.compressobj(level=1)
    with coordinate_path.open("wb") as stream:
        for begin, end, label in truth["runs"]:
            offset = begin
            while offset < end:
                stop = min(end, offset + BLOCK_POINT_COUNT)
                local = np.arange(offset - begin, stop - begin, dtype=np.int64)
                payload = _coordinate_block(dataset, contract, label, local, np).tobytes(order="C")
                raw_digest.update(payload)
                compressed = compressor.compress(payload)
                if compressed:
                    stream.write(compressed)
                    compressed_digest.update(compressed)
                    compressed_size += len(compressed)
                offset = stop
        compressed = compressor.flush()
        stream.write(compressed)
        compressed_digest.update(compressed)
        compressed_size += len(compressed)
        stream.flush()
        os.fsync(stream.fileno())
    coordinate_descriptor = {"file": coordinate_path.name, "sha256": compressed_digest.hexdigest(), "size_bytes": compressed_size}
    raw_cloud_sha256 = raw_digest.hexdigest()
    cloud = {
        "coordinate_encoding": {"compression": "zlib_level_1", "coordinate_denominator_exponent": COORDINATE_DENOMINATOR_EXPONENT, "point_id_domain": [0, point_count], "record_bytes": COORDINATE_RECORD_BYTES, "record_layout": "little_endian_signed_int64_x_y_z"},
        "coordinate_stream": coordinate_descriptor,
        "dataset": dataset,
        "generator_id": GENERATOR_ID,
        "generator_sha256": generator_sha256,
        "geometry_contract": contract,
        "point_count": point_count,
        "raw_cloud_sha256": raw_cloud_sha256,
        "raw_coordinate_bytes": point_count * COORDINATE_RECORD_BYTES,
        "schema": CLOUD_SCHEMA,
        "truth_rle_sha256": truth_sha256,
    }
    cloud_payload = canonical_bytes(cloud)
    cloud_path = output_directory / "raw-cloud.manifest.json"
    atomic_publish(cloud_path, cloud_payload)
    return {"cloud": {"file": cloud_path.name, "sha256": sha256_bytes(cloud_payload), "size_bytes": len(cloud_payload)}, "coordinates": coordinate_descriptor, "raw_cloud_sha256": raw_cloud_sha256, "truth": {"file": truth_path.name, "sha256": truth_sha256, "size_bytes": len(truth_payload)}}
