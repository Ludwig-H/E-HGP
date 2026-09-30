#!/usr/bin/env python3
"""Prepare a complete scene: lossless float32 by default, explicit grid optional.

Lossless means the decoded finite XYZ coordinates stored in the input file,
not physical sensor accuracy. Signed zeros are one geometric coordinate.
An optional fixed isotropic decimal grid defaults to 1 mm. Grid coordinates
are rounded exactly, then translated by the full scene's integer minima
into u32 storage; neither scale nor translation changes between pieces.

Explicit source port of the mapping/partition and failure-preserving I/O
architecture of prepare_lidar_spatial.py, SHA256
d5bc8af10dad6c14f304f5737b52f2c0a6e452ac9a674957bd909f062c32c898.
The frozen predecessor is not imported, modified, or monkeypatched.
These new f32/u32 inputs are NOT accepted by the current u16 native engine.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import re
from fractions import Fraction
from pathlib import Path
import signal
import struct
import sys
from typing import Any


SCHEMA = "mhgp8_lidar_precision_manifest_v2"
COMPLETION_SCHEMA = "mhgp8_lidar_precision_completion_v2"
PREDECESSOR = "morsehgp3D_v8/bench/prepare_lidar_spatial.py"
PREDECESSOR_SHA256 = "d5bc8af10dad6c14f304f5737b52f2c0a6e452ac9a674957bd909f062c32c898"
DEFAULT_PROFILE = "float32"
DEFAULT_PRECISION_MM = "1"
PROFILES = ("float32", "grid")
U32_LIMIT = 1 << 32
DATASET_NAMES = (
    "full", "half_x_neg", "half_x_nonneg",
    "quarter_x_neg_y_neg", "quarter_x_neg_y_nonneg",
    "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg",
)
QUARTER_NAMES = DATASET_NAMES[3:]
RAW_MAP_NAME = "raw_to_full.u32le"
RAW_RECORD = struct.Struct("<ffff")
SITE_RECORDS = {"float32": struct.Struct("<fff"), "grid": struct.Struct("<III")}
ID_RECORD = struct.Struct("<I")


class InvalidPreparation(ValueError):
    """The complete input or its recorded preparation does not meet the contract."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise InvalidPreparation(message)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def stamp() -> str:
    return datetime.now(timezone.utc).isoformat()


def canonical_json(value: Any) -> bytes:
    return (json.dumps(value, sort_keys=True, indent=2, ensure_ascii=False, allow_nan=False) + "\n").encode("utf-8")


def _object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _nonfinite(value: str) -> None:
    raise InvalidPreparation(f"non-finite JSON token: {value}")


def load_json(data: bytes) -> dict[str, Any]:
    value = json.loads(data.decode("utf-8"), object_pairs_hook=_object, parse_constant=_nonfinite)
    require(type(value) is dict, "metadata must be a JSON object")
    return value


def parameters(profile: str = DEFAULT_PROFILE, precision_mm: str | None = None) -> dict[str, Any]:
    require(type(profile) is str and profile in PROFILES, "profile must be float32 or grid")
    if profile == "float32":
        require(precision_mm is None, "precision-mm is forbidden for the lossless float32 profile")
        return dict(profile=profile, precision_mm=None)
    text = DEFAULT_PRECISION_MM if precision_mm is None else precision_mm
    require(type(text) is str and re.fullmatch(r"[+]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?", text) is not None,
            "precision-mm must be a finite positive decimal text")
    try:
        value = Fraction(text)
    except (ValueError, OverflowError, ZeroDivisionError) as error:
        raise InvalidPreparation("invalid exact decimal precision") from error
    require(value > 0, "precision-mm must be positive")
    return dict(profile=profile, precision_mm=text)


def ratio(value: Fraction) -> dict[str, int]:
    return dict(numerator=value.numerator, denominator=value.denominator)


def quantize_coordinate(value: float, precision_mm: str = DEFAULT_PRECISION_MM) -> int:
    """Untranslated signed grid index: exact floor(x / step_metres + 1/2)."""
    require(type(value) is float and math.isfinite(value), "coordinate must be finite")
    config = parameters("grid", precision_mm)
    step = Fraction(config["precision_mm"])/1000
    numerator, denominator = value.as_integer_ratio()
    return (2*numerator*step.denominator + denominator*step.numerator)//(2*denominator*step.numerator)


def _ids_bytes(ids: list[int]) -> bytes:
    result = bytearray(ID_RECORD.size*len(ids))
    for position, identifier in enumerate(ids):
        ID_RECORD.pack_into(result, ID_RECORD.size*position, identifier)
    return bytes(result)


def _sites_bytes(sites: list[tuple[Any, Any, Any]], ids: list[int], profile: str) -> bytes:
    encoding = SITE_RECORDS[profile]
    result = bytearray(encoding.size*len(ids))
    for position, identifier in enumerate(ids):
        encoding.pack_into(result, encoding.size*position, *sites[identifier])
    return bytes(result)


def reconstruct(raw: bytes, profile: str = DEFAULT_PROFILE,
                precision_mm: str | None = None) -> tuple[dict[str, Any], dict[str, bytes]]:
    """Validate all XYZ and compute all seven complete scene datasets in memory."""
    config = parameters(profile, precision_mm)
    require(type(raw) is bytes and len(raw) > 0 and len(raw) % RAW_RECORD.size == 0,
            "KITTI scan must be a nonempty complete array of 16-byte xyzi records")
    raw_count = len(raw)//RAW_RECORD.size
    require(raw_count <= U32_LIMIT, "raw record count exceeds the u32 mapping domain")
    step = Fraction(config["precision_mm"])/1000 if profile == "grid" else None
    raw_sites = []
    raw_unique_xyz = set()
    raw_low, raw_high = [math.inf]*3, [-math.inf]*3
    error_maxima = [Fraction(0)]*3
    error_coordinates_verified = 0
    float32_roundtrip_coordinates_verified = 0
    groups = {}
    raw_quadrants, represented_quadrants = [0]*4, [0]*4
    raw_zero_x = raw_zero_y = changed_x = changed_y = changed_quadrant = 0
    represented_zero_x_returns = represented_zero_y_returns = 0
    negative_zero_components = 0
    nonfinite_reflectance_returns = 0
    for raw_id, values in enumerate(RAW_RECORD.iter_unpack(raw)):
        require(all(math.isfinite(value) for value in values[:3]), f"raw return {raw_id}: non-finite XYZ component")
        nonfinite_reflectance_returns += not math.isfinite(values[3])
        xyz = tuple(0.0 if value == 0 else value for value in values[:3])
        negative_zero_components += sum(value == 0 and math.copysign(1.0, value) < 0 for value in values[:3])
        raw_unique_xyz.add(xyz)
        for axis, value in enumerate(xyz):
            raw_low[axis], raw_high[axis] = min(raw_low[axis], value), max(raw_high[axis], value)
        if step is None:
            site = xyz
            require(SITE_RECORDS["float32"].unpack(SITE_RECORDS["float32"].pack(*site)) == xyz,
                    "finite XYZ value changed during float32 serialization")
            float32_roundtrip_coordinates_verified += 3
        else:
            point = []
            for axis, value in enumerate(xyz):
                numerator, denominator = value.as_integer_ratio()
                scaled_n, scaled_d = numerator*step.denominator, denominator*step.numerator
                q = (2*scaled_n + scaled_d)//(2*scaled_d)
                error = abs(Fraction(q)*step-Fraction(numerator, denominator))
                require(2*error <= step, "rounding error exceeds half the declared grid cell")
                error_maxima[axis] = max(error_maxima[axis], error)
                error_coordinates_verified += 1
                point.append(q)
            site = tuple(point)
        raw_sites.append(site)
        rx, ry = xyz[0] >= 0, xyz[1] >= 0
        sx, sy = site[0] >= 0, site[1] >= 0
        rq, sq = 2*int(rx)+int(ry), 2*int(sx)+int(sy)
        raw_quadrants[rq] += 1
        represented_quadrants[sq] += 1
        raw_zero_x += xyz[0] == 0
        raw_zero_y += xyz[1] == 0
        changed_x += rx != sx
        changed_y += ry != sy
        changed_quadrant += rq != sq
        represented_zero_x_returns += site[0] == 0
        represented_zero_y_returns += site[1] == 0
        group = groups.setdefault(site, [0, 0])
        group[0] += 1
        group[1] |= 1 << rq
    signed_sites = sorted(groups)
    require(len(signed_sites) <= U32_LIMIT, "full-site count exceeds the u32 ID domain")
    represented_min = [min(site[axis] for site in signed_sites) for axis in range(3)]
    represented_max = [max(site[axis] for site in signed_sites) for axis in range(3)]
    translation = [-value for value in represented_min] if step is not None else [0.0]*3
    if step is not None:
        require(all(high-low < U32_LIMIT for low, high in zip(represented_min, represented_max, strict=True)),
                "full scene exceeds the fixed u32 grid span; no clipping, scale adaptation, or subset")
        stored_sites = [tuple(site[axis]+translation[axis] for axis in range(3)) for site in signed_sites]
    else:
        stored_sites = signed_sites
    full_ids = {site: identifier for identifier, site in enumerate(signed_sites)}
    raw_mapping = [full_ids[site] for site in raw_sites]
    ids = {name: [] for name in DATASET_NAMES}
    for identifier, site in enumerate(signed_sites):
        ids["full"].append(identifier)
        x_side, y_side = int(site[0] >= 0), int(site[1] >= 0)
        ids[DATASET_NAMES[1+x_side]].append(identifier)
        ids[QUARTER_NAMES[2*x_side+y_side]].append(identifier)
    halves = [*ids["half_x_neg"], *ids["half_x_nonneg"]]
    quarters = [identifier for name in QUARTER_NAMES for identifier in ids[name]]
    require(sorted(halves) == sorted(quarters) == ids["full"], "partition does not reconstruct full IDs exactly")
    for side in range(2):
        require(sorted(ids[QUARTER_NAMES[2*side]] + ids[QUARTER_NAMES[2*side+1]]) == ids[DATASET_NAMES[1+side]],
                "quarter pair does not reconstruct its half")
    require(set(raw_mapping) == set(ids["full"]), "raw-to-full map is not surjective")
    require(all(all(a < b for a,b in zip(values, values[1:])) for values in ids.values()),
            "local-to-full IDs are not strictly increasing")
    suffix = ".f32le" if step is None else ".u32le"
    payloads = {RAW_MAP_NAME: _ids_bytes(raw_mapping)}
    datasets = {}
    for name in DATASET_NAMES:
        points_name, map_name = name+suffix, name+".site_ids.u32le"
        payloads[points_name] = _sites_bytes(stored_sites, ids[name], profile)
        payloads[map_name] = _ids_bytes(ids[name])
        datasets[name] = dict(sites=len(ids[name]), points_file=points_name, point_bytes=len(payloads[points_name]),
            points_sha256=sha256(payloads[points_name]), site_ids_file=map_name, site_id_bytes=len(payloads[map_name]),
            site_ids_sha256=sha256(payloads[map_name]), local_order="strictly_increasing_full_site_id")
    mixed_x = mixed_y = mixed_quadrants = multiple_returns = 0
    for multiplicity, mask in groups.values():
        multiple_returns += multiplicity > 1
        mixed_quadrants += mask.bit_count() > 1
        mixed_x += bool(mask & 0b0011) and bool(mask & 0b1100)
        mixed_y += bool(mask & 0b0101) and bool(mask & 0b1010)
    unique = len(signed_sites)
    metadata = dict(parameters=config,
        profile="lossless_float32_input_only" if step is None else "quantized_u32_fixed_grid_input_only",
        representation=dict(coordinates="little_endian_float32_xyz" if step is None else "little_endian_u32_xyz",
            point_bytes=12, suffix=suffix, signed_zero="normalize_negative_zero_to_positive_zero",
            raw_coordinate_values_preserved=step is None, physical_sensor_accuracy_claimed=False,
            current_u16_engine_input_compatible=False, native_full_qualified=False),
        quantization=dict(applied=step is not None, precision_mm_text=config["precision_mm"],
            step_metres=None if step is None else ratio(step),
            formula=None if step is None else "floor(x/step_metres+1/2)",
            arithmetic="exact_decoded_float32" if step is None else "exact_integer_ratio_and_rational_decimal_grid",
            clipping=False, jitter=False, adaptive_scale=False),
        translation=dict(applied=step is not None, vector=translation,
            policy="none" if step is None else "minus_componentwise_minimum_signed_grid_index_of_entire_scene",
            common_to_all_seven_datasets=True, preserves_grid_distances=True),
        counts=dict(raw_returns=raw_count, unique_sites=unique, merged_returns=raw_count-unique,
            sites_with_multiple_returns=multiple_returns, exact_float32_unique_xyz=len(raw_unique_xyz),
            repeated_raw_xyz_returns=raw_count-len(raw_unique_xyz),
            distinct_raw_sites_merged_by_quantization=len(raw_unique_xyz)-unique,
            negative_zero_xyz_components=negative_zero_components,
            nonfinite_reflectance_returns=nonfinite_reflectance_returns),
        coordinate_bounds=dict(raw_exact_float32_min_m=raw_low, raw_exact_float32_max_m=raw_high,
            signed_represented_min=represented_min, signed_represented_max=represented_max,
            encoded_min=[min(site[axis] for site in stored_sites) for axis in range(3)],
            encoded_max=[max(site[axis] for site in stored_sites) for axis in range(3)]),
        quantization_error_proof=dict(coordinates_verified=error_coordinates_verified,
            float32_roundtrip_coordinates_verified=float32_roundtrip_coordinates_verified,
            max_abs_error_metres_by_axis=[ratio(value) for value in error_maxima],
            bound_metres=ratio(Fraction(0) if step is None else step/2),
            exact_raw_coordinate_values_preserved=step is None,
            topology_preservation_claimed=False,
            physical_sensor_accuracy_claimed=False,
            source_multiplicity_not_preserved_as_distinct_sites=True),
        boundaries=dict(raw_quadrant_returns=dict(zip(QUARTER_NAMES, raw_quadrants)),
            represented_quadrant_returns=dict(zip(QUARTER_NAMES, represented_quadrants)),
            represented_quadrant_sites={name: len(ids[name]) for name in QUARTER_NAMES},
            raw_on_x_plane_returns=raw_zero_x, raw_on_y_plane_returns=raw_zero_y,
            represented_on_x_plane_returns=represented_zero_x_returns,
            represented_on_y_plane_returns=represented_zero_y_returns,
            represented_on_x_plane_sites=sum(site[0] == 0 for site in signed_sites),
            represented_on_y_plane_sites=sum(site[1] == 0 for site in signed_sites),
            raw_to_represented_x_partition_changes=changed_x, raw_to_represented_y_partition_changes=changed_y,
            raw_to_represented_quadrant_changes=changed_quadrant,
            sites_mixing_raw_x_sides=mixed_x, sites_mixing_raw_y_sides=mixed_y, sites_mixing_raw_quadrants=mixed_quadrants),
        partition=dict(stage="after_global_coordinate_deduplication",
            raw_reference_planes=[dict(axis="x", value_metres=0), dict(axis="y", value_metres=0)],
            encoded_sensor_origin=translation,
            half_rule="encoded_x<encoded_sensor_origin_x versus >=; equivalently signed_x<0 versus >=0",
            quarter_rule="within_each_half: encoded_y<encoded_sensor_origin_y versus >=; equivalently signed_y<0 versus >=0",
            boundary_owner="nonnegative_side", equivalence_to_raw_sign_partition=step is None,
            origin_may_lie_outside_u32_storage=True if step is not None else False,
            prefixes_or_subsampling=False),
        partition_proof=dict(full_sites=unique, half_sites=len(halves), quarter_sites=len(quarters),
            half_overlap_sites=0, quarter_overlap_sites=0, half_union_equals_full=True,
            quarter_union_equals_full=True, quarter_pairs_equal_halves=True,
            local_to_full_ids_strictly_increasing=True, raw_mapping_entries=len(raw_mapping),
            raw_mapping_distinct_full_ids=len(set(raw_mapping)), raw_mapping_surjective=True,
            raw_returns_discarded=0, subsampled_sites=0),
        datasets=datasets, raw_to_full=dict(file=RAW_MAP_NAME, entries=raw_count, bytes=len(payloads[RAW_MAP_NAME]),
            sha256=sha256(payloads[RAW_MAP_NAME]), encoding="little_endian_u32_full_site_id_in_raw_return_order"))
    return metadata, payloads


def _manifest(input_path: Path, raw: bytes, script_hash: str, metadata: dict[str, Any]) -> dict[str, Any]:
    return dict(schema=SCHEMA, status="prepared", public_status="not_claimed", dataset_order=list(DATASET_NAMES),
        scope="whole_sensor_scene_lossless_float32_or_explicit_fixed_grid_not_native_HGP_qualification",
        algorithm_reuse=dict(kind="explicit_source_port_not_runtime_import",
            predecessor=PREDECESSOR, predecessor_sha256=PREDECESSOR_SHA256),
        raw=dict(path=str(input_path), bytes=len(raw), sha256=sha256(raw),
            format="little_endian_float32_x_y_z_reflectance", finite_validation="XYZ_only",
            reflectance="all_original_bits_retained_in_raw_source_addressed_by_raw_ID_never_used_geometrically",
            differs_from_predecessor="nonfinite_reflectance_is_now_allowed"),
        script=dict(name=Path(__file__).name, sha256=script_hash),
        coordinate_frame="raw_sensor_xyz_no_pose_no_rotation_no_physical_translation", **metadata)


def _write_bytes(path: Path, data: bytes) -> None:
    with path.open("xb") as output:
        output.write(data)


def _current_hash(path: Path) -> str | None:
    try:
        return sha256(path.read_bytes())
    except OSError:
        return None


def prepare(input_path: Path, output_path: Path, profile: str = DEFAULT_PROFILE,
            precision_mm: str | None = None) -> dict[str, Any]:
    input_path, output_path = Path(input_path).resolve(), Path(output_path).absolute()
    require(not output_path.exists() and not output_path.is_symlink(), "output must not already exist")
    require(input_path.is_file(), "raw input must be an existing regular file")
    script_path = Path(__file__).resolve()
    script_hash, raw = sha256(script_path.read_bytes()), input_path.read_bytes()
    metadata, payloads = reconstruct(raw, profile, precision_mm)
    manifest = _manifest(input_path, raw, script_hash, metadata)
    manifest_bytes = canonical_json(manifest)
    expected_outputs = {name: sha256(data) for name, data in payloads.items()}
    # All input validation, exact partitioning and binary encodings precede
    # creation of output or its missing parents.
    require(_current_hash(input_path) == manifest["raw"]["sha256"] and _current_hash(script_path) == script_hash,
            "input or preparation script changed during validation")
    output_path.mkdir(parents=True, exist_ok=False)
    started, error = stamp(), None
    try:
        _write_bytes(output_path / "MANIFEST.json", manifest_bytes)
        for name, data in payloads.items():
            _write_bytes(output_path / name, data)
        require({entry.name for entry in output_path.iterdir()} == {"MANIFEST.json", *payloads}, "unexpected output artifact")
        require(all((output_path / name).read_bytes() == data for name, data in payloads.items()), "written payload differs")
        require(_current_hash(input_path) == manifest["raw"]["sha256"] and _current_hash(script_path) == script_hash,
                "input or preparation script changed while writing")
    except BaseException as cause:
        error = cause
    manifest_after = _current_hash(output_path / "MANIFEST.json")
    raw_after, script_after = _current_hash(input_path), _current_hash(script_path)
    output_after = {name: _current_hash(output_path / name) for name in payloads}
    try:
        require(manifest_after == sha256(manifest_bytes) and raw_after == manifest["raw"]["sha256"] and
                script_after == script_hash and output_after == expected_outputs, "preparation closure differs")
    except BaseException as cause:
        if error is None:
            error = cause
    completion = dict(schema=COMPLETION_SCHEMA, status="passed" if error is None else "failed",
        started_utc=started, finished_utc=stamp(), error=None if error is None else f"{type(error).__name__}: {error}",
        manifest_sha256=manifest_after, raw_sha256_after=raw_after,
        script_sha256_after=script_after, output_sha256=output_after)
    try:
        _write_bytes(output_path / "COMPLETION.json", canonical_json(completion))
    except BaseException as completion_error:
        raise InvalidPreparation(f"preparation status could not be persisted: {completion_error}; original failure: {error}") from completion_error
    if error is not None:
        raise error
    return dict(status="passed", path=str(output_path.resolve()), schema=SCHEMA,
        raw_returns=metadata["counts"]["raw_returns"], unique_sites=metadata["counts"]["unique_sites"], datasets=7,
        parameters=metadata["parameters"], profile=metadata["profile"],
        manifest_sha256=sha256(manifest_bytes), completion_sha256=sha256(canonical_json(completion)))


def read(path: Path) -> dict[str, Any]:
    path = Path(path).absolute()
    require(path.is_dir() and not path.is_symlink(), "prepared path must be a real directory")
    require(all((path / name).is_file() and not (path / name).is_symlink()
                for name in ("MANIFEST.json", "COMPLETION.json")), "missing or linked metadata")
    manifest_bytes, completion_bytes = (path / "MANIFEST.json").read_bytes(), (path / "COMPLETION.json").read_bytes()
    manifest, completion = load_json(manifest_bytes), load_json(completion_bytes)
    config = manifest.get("parameters")
    require(type(config) is dict and set(config) == {"profile", "precision_mm"}, "profile parameter inventory")
    config = parameters(**config)
    suffix = ".f32le" if config["profile"] == "float32" else ".u32le"
    expected_names = {"MANIFEST.json", "COMPLETION.json", RAW_MAP_NAME,
        *(name + extension for name in DATASET_NAMES for extension in (suffix, ".site_ids.u32le"))}
    require({entry.name for entry in path.iterdir()} == expected_names and
            all((path / name).is_file() and not (path / name).is_symlink() for name in expected_names),
            "missing, extra, nonregular or linked preparation artifact")
    require(type(manifest.get("raw")) is dict and type(manifest["raw"].get("path")) is str, "missing raw provenance")
    input_path = Path(manifest["raw"]["path"])
    require(input_path.is_absolute() and input_path.resolve() == input_path and input_path.is_file(), "raw provenance path unavailable/noncanonical")
    raw, script_path = input_path.read_bytes(), Path(__file__).resolve()
    script_hash = sha256(script_path.read_bytes())
    metadata, payloads = reconstruct(raw, **config)
    expected = _manifest(input_path, raw, script_hash, metadata)
    require(manifest_bytes == canonical_json(expected), "manifest metadata/hash/type differs from complete raw reconstruction")
    for name, data in payloads.items():
        require((path / name).read_bytes() == data, f"{name}: payload differs from complete raw reconstruction")
    require(set(completion) == {"schema", "status", "started_utc", "finished_utc", "error", "manifest_sha256",
            "raw_sha256_after", "script_sha256_after", "output_sha256"}, "completion fields differ")
    for field in ("started_utc", "finished_utc"):
        require(type(completion[field]) is str, "completion timestamp must be text")
    try:
        started, finished = (datetime.fromisoformat(completion[field]) for field in ("started_utc", "finished_utc"))
        require(started.utcoffset() == finished.utcoffset() == timezone.utc.utcoffset(None) and started <= finished,
                "completion timestamps are not ordered UTC")
    except (TypeError, ValueError) as error:
        raise InvalidPreparation(f"invalid completion timestamps: {error}") from error
    expected_completion = dict(schema=COMPLETION_SCHEMA, status="passed", started_utc=completion["started_utc"],
        finished_utc=completion["finished_utc"], error=None, manifest_sha256=sha256(manifest_bytes),
        raw_sha256_after=sha256(raw), script_sha256_after=script_hash,
        output_sha256={name: sha256(data) for name, data in payloads.items()})
    require(completion_bytes == canonical_json(expected_completion), "completion did not close the exact successful preparation")
    require({entry.name for entry in path.iterdir()} == expected_names and
            all((path / name).is_file() and not (path / name).is_symlink() for name in expected_names) and
            {name: _current_hash(path / name) for name in payloads} == expected_completion["output_sha256"],
            "payload artifacts changed during reading")
    require(_current_hash(input_path) == expected["raw"]["sha256"] and _current_hash(script_path) == script_hash and
            (path / "MANIFEST.json").read_bytes() == manifest_bytes and (path / "COMPLETION.json").read_bytes() == completion_bytes,
            "input, script or metadata changed during reading")
    return dict(status="passed", path=str(path.resolve()), schema=SCHEMA, raw_returns=metadata["counts"]["raw_returns"],
        unique_sites=metadata["counts"]["unique_sites"], datasets=7, artifacts=len(expected_names),
        parameters=config, profile=metadata["profile"], counts=metadata["counts"],
        boundary_counts=metadata["boundaries"], manifest_sha256=sha256(manifest_bytes), completion_sha256=sha256(completion_bytes),
        verified_by="complete_raw_reconstruction_not_artifact_hashes_only")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="operation", required=True)
    prepare_parser = commands.add_parser("prepare")
    prepare_parser.add_argument("--input", type=Path, required=True)
    prepare_parser.add_argument("--output", type=Path, required=True)
    prepare_parser.add_argument("--profile", choices=PROFILES, default=DEFAULT_PROFILE)
    prepare_parser.add_argument("--precision-mm", help="exact positive decimal text; grid only, default 1 mm")
    reader = commands.add_parser("read")
    reader.add_argument("--path", type=Path, required=True)
    args = parser.parse_args()
    def interrupted(signum: int, _frame: Any) -> None:
        raise InterruptedError(f"signal {signum}")
    signal.signal(signal.SIGTERM, interrupted)
    try:
        result = prepare(args.input, args.output, args.profile, args.precision_mm) if args.operation == "prepare" else read(args.path)
        print(json.dumps(result, sort_keys=True, allow_nan=False))
        return 0
    except (Exception, KeyboardInterrupt) as error:
        print(json.dumps(dict(status="failed", error=f"{type(error).__name__}: {error}"), sort_keys=True, allow_nan=False), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
