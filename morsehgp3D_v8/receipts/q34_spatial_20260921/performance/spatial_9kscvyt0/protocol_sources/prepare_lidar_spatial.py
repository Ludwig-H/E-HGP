#!/usr/bin/env python3
"""Prepare seven exact spatial partitions of one complete raw KITTI scan.

Input is little-endian float32 (x,y,z,reflectance), in the untransformed sensor
frame. Quantization uses integer arithmetic on each decoded float's exact
ratio. No clipping, jitter, pose, subsampling or prefix selection is applied.
Global lexicographic u16 sites are deduplicated, but every raw return retains
its full-site mapping. Splits use QUANTIZED centers, not the original signs.

Preparation validates and reconstructs every payload before creating output.
A later failure preserves the partial directory and a failed COMPLETION when
the filesystem permits it. Reading reconstructs everything from the raw scan;
artifact hashes alone never stand in for a partition or quantization check.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import signal
import struct
import sys
from typing import Any


SCHEMA = "mhgp8_lidar_spatial_manifest_v1"
COMPLETION_SCHEMA = "mhgp8_lidar_spatial_completion_v1"
U16_ORIGIN = 32768
U32_LIMIT = 1 << 32
DATASET_NAMES = (
    "full", "half_x_neg", "half_x_nonneg",
    "quarter_x_neg_y_neg", "quarter_x_neg_y_nonneg",
    "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg",
)
QUARTER_NAMES = DATASET_NAMES[3:]
RAW_MAP_NAME = "raw_to_full.u32le"
RAW_RECORD = struct.Struct("<ffff")
SITE_RECORD = struct.Struct("<HHH")
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


def quantize_coordinate(value: float) -> int:
    """Exact floor(50*x + 32768 + 1/2), with no float multiplication."""
    require(type(value) is float and math.isfinite(value), "coordinate must be a finite decoded float")
    numerator, denominator = value.as_integer_ratio()
    result = (100*numerator + 65537*denominator) // (2*denominator)
    require(0 <= result <= 65535, "coordinate lies outside the fixed 20 mm u16 grid")
    return result


def _ids_bytes(ids: list[int]) -> bytes:
    result = bytearray(ID_RECORD.size*len(ids))
    for position, identifier in enumerate(ids):
        ID_RECORD.pack_into(result, ID_RECORD.size*position, identifier)
    return bytes(result)


def _sites_bytes(sites: list[tuple[int, int, int]], ids: list[int]) -> bytes:
    result = bytearray(SITE_RECORD.size*len(ids))
    for position, identifier in enumerate(ids):
        SITE_RECORD.pack_into(result, SITE_RECORD.size*position, *sites[identifier])
    return bytes(result)


def reconstruct(raw: bytes) -> tuple[dict[str, Any], dict[str, bytes]]:
    """Validate one whole scan; return deterministic metadata and exact payloads."""
    require(type(raw) is bytes and len(raw) > 0 and len(raw) % RAW_RECORD.size == 0,
            "KITTI scan must be a nonempty complete array of 16-byte xyzi records")
    raw_count = len(raw)//RAW_RECORD.size
    require(raw_count <= U32_LIMIT, "raw record count exceeds the u32 mapping domain")
    raw_sites: list[tuple[int, int, int]] = []
    # Per site: multiplicity and bitset of original (unquantized) quadrants.
    groups: dict[tuple[int, int, int], list[int]] = {}
    raw_quadrants, quantized_quadrants = [0]*4, [0]*4
    raw_zero_x = raw_zero_y = changed_x = changed_y = changed_quadrant = 0
    quantized_zero_x_returns = quantized_zero_y_returns = 0
    for raw_id, values in enumerate(RAW_RECORD.iter_unpack(raw)):
        require(all(math.isfinite(value) for value in values), f"raw return {raw_id}: non-finite xyzi component")
        try:
            site = tuple(quantize_coordinate(value) for value in values[:3])
        except InvalidPreparation as error:
            raise InvalidPreparation(f"raw return {raw_id}: {error}") from error
        raw_sites.append(site)
        rx, ry = values[0] >= 0, values[1] >= 0
        qx, qy = site[0] >= U16_ORIGIN, site[1] >= U16_ORIGIN
        rq, qq = 2*int(rx)+int(ry), 2*int(qx)+int(qy)
        raw_quadrants[rq] += 1
        quantized_quadrants[qq] += 1
        raw_zero_x += values[0] == 0
        raw_zero_y += values[1] == 0
        changed_x += rx != qx
        changed_y += ry != qy
        changed_quadrant += rq != qq
        quantized_zero_x_returns += site[0] == U16_ORIGIN
        quantized_zero_y_returns += site[1] == U16_ORIGIN
        group = groups.setdefault(site, [0, 0])
        group[0] += 1
        group[1] |= 1 << rq
    sites = sorted(groups)
    require(len(sites) <= U32_LIMIT, "full-site count exceeds the u32 ID domain")
    full_ids = {site: identifier for identifier, site in enumerate(sites)}
    raw_mapping = [full_ids[site] for site in raw_sites]
    ids = {name: [] for name in DATASET_NAMES}
    for identifier, site in enumerate(sites):
        ids["full"].append(identifier)
        x_side = int(site[0] >= U16_ORIGIN)
        y_side = int(site[1] >= U16_ORIGIN)
        ids[DATASET_NAMES[1+x_side]].append(identifier)
        ids[QUARTER_NAMES[2*x_side+y_side]].append(identifier)
    site_count = len(sites)
    halves = [*ids["half_x_neg"], *ids["half_x_nonneg"]]
    quarters = [identifier for name in QUARTER_NAMES for identifier in ids[name]]
    require(sorted(halves) == sorted(quarters) == ids["full"], "partition does not reconstruct full IDs exactly")
    for side in range(2):
        require(sorted(ids[QUARTER_NAMES[2*side]] + ids[QUARTER_NAMES[2*side+1]]) == ids[DATASET_NAMES[1+side]],
                "quarter pair does not reconstruct its half")
    require(set(raw_mapping) == set(ids["full"]), "raw-to-full map is not surjective")
    require(all(all(a < b for a, b in zip(values, values[1:])) for values in ids.values()),
            "local-to-full IDs are not strictly increasing")
    payloads = {RAW_MAP_NAME: _ids_bytes(raw_mapping)}
    datasets = {}
    for name in DATASET_NAMES:
        points_name, map_name = name + ".u16le", name + ".site_ids.u32le"
        payloads[points_name] = _sites_bytes(sites, ids[name])
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
    metadata = dict(
        counts=dict(raw_returns=raw_count, unique_sites=site_count, merged_returns=raw_count-site_count,
                    sites_with_multiple_returns=multiple_returns),
        boundaries=dict(
            raw_quadrant_returns=dict(zip(QUARTER_NAMES, raw_quadrants)),
            quantized_quadrant_returns=dict(zip(QUARTER_NAMES, quantized_quadrants)),
            quantized_quadrant_sites={name: len(ids[name]) for name in QUARTER_NAMES},
            raw_x_negative_returns=sum(raw_quadrants[:2]), raw_y_negative_returns=raw_quadrants[0]+raw_quadrants[2],
            quantized_x_negative_returns=sum(quantized_quadrants[:2]),
            quantized_y_negative_returns=quantized_quadrants[0]+quantized_quadrants[2],
            raw_on_x_plane_returns=raw_zero_x, raw_on_y_plane_returns=raw_zero_y,
            quantized_on_x_plane_returns=quantized_zero_x_returns, quantized_on_y_plane_returns=quantized_zero_y_returns,
            quantized_on_x_plane_sites=sum(site[0] == U16_ORIGIN for site in sites),
            quantized_on_y_plane_sites=sum(site[1] == U16_ORIGIN for site in sites),
            raw_to_quantized_x_partition_changes=changed_x, raw_to_quantized_y_partition_changes=changed_y,
            raw_to_quantized_quadrant_changes=changed_quadrant,
            sites_mixing_raw_x_sides=mixed_x, sites_mixing_raw_y_sides=mixed_y, sites_mixing_raw_quadrants=mixed_quadrants),
        partition_proof=dict(full_sites=site_count, half_sites=sum(len(ids[name]) for name in DATASET_NAMES[1:3]),
            quarter_sites=sum(len(ids[name]) for name in QUARTER_NAMES), half_overlap_sites=0, quarter_overlap_sites=0,
            half_union_equals_full=True, quarter_union_equals_full=True, quarter_pairs_equal_halves=True,
            local_to_full_ids_strictly_increasing=True, raw_mapping_entries=len(raw_mapping),
            raw_mapping_distinct_full_ids=len(set(raw_mapping)), raw_mapping_surjective=True,
            raw_returns_discarded=0, subsampled_sites=0),
        datasets=datasets,
        raw_to_full=dict(file=RAW_MAP_NAME, entries=raw_count, bytes=len(payloads[RAW_MAP_NAME]),
            sha256=sha256(payloads[RAW_MAP_NAME]), encoding="little_endian_u32_full_site_id_in_raw_return_order"),
    )
    return metadata, payloads


def _manifest(input_path: Path, raw: bytes, script_hash: str, metadata: dict[str, Any]) -> dict[str, Any]:
    return dict(schema=SCHEMA, status="prepared", scope="whole_raw_sensor_scan_and_exact_quantized_spatial_partitions",
        profile="quantized_u16_input_only", public_status="not_claimed", dataset_order=list(DATASET_NAMES),
        raw=dict(path=str(input_path), bytes=len(raw), sha256=sha256(raw),
            format="little_endian_float32_x_y_z_reflectance", finite_validation="all_four_components",
            reflectance="not_quantized_or_filtered; retained in raw source and addressed by raw ID"),
        script=dict(name=Path(__file__).name, sha256=script_hash),
        coordinate_frame="raw_sensor_xyz_no_pose_no_rotation_no_translation",
        quantization=dict(grid_mm=20, units="metres", scale_per_metre=50, origin_u16=[32768,32768,32768],
            formula="floor(50*x+32768+1/2)", arithmetic="exact_integer_ratio_of_decoded_float32",
            integer_formula="(100*numerator+65537*denominator)//(2*denominator)",
            range_policy="reject_entire_scan_if_any_quantized_coordinate_is_outside_0_65535",
            clipping=False, jitter=False, adaptive_scale=False),
        site_identity=dict(deduplication="global_equal_quantized_xyz", full_order="lexicographic_u16_x_y_z",
            id_encoding="u32_little_endian", all_raw_returns_mapped=True),
        partition=dict(stage="after_global_quantization_and_deduplication", threshold_u16=32768,
            raw_reference_planes=[dict(axis="x", value_metres=0), dict(axis="y", value_metres=0)],
            half_rule="x_u16<32768 versus x_u16>=32768",
            quarter_rule="within_each_half: y_u16<32768 versus y_u16>=32768",
            boundary_owner="nonnegative_side", equivalence_to_raw_sign_partition=False,
            prefixes_or_subsampling=False), **metadata)


def _write_bytes(path: Path, data: bytes) -> None:
    with path.open("xb") as output:
        output.write(data)


def _current_hash(path: Path) -> str | None:
    try:
        return sha256(path.read_bytes())
    except OSError:
        return None


def prepare(input_path: Path, output_path: Path) -> dict[str, Any]:
    input_path, output_path = Path(input_path).resolve(), Path(output_path).absolute()
    require(not output_path.exists() and not output_path.is_symlink(), "output must not already exist")
    require(input_path.is_file(), "raw input must be an existing regular file")
    script_path = Path(__file__).resolve()
    script_hash, raw = sha256(script_path.read_bytes()), input_path.read_bytes()
    metadata, payloads = reconstruct(raw)
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
        manifest_sha256=sha256(manifest_bytes), completion_sha256=sha256(canonical_json(completion)))


def read(path: Path) -> dict[str, Any]:
    path = Path(path).absolute()
    require(path.is_dir() and not path.is_symlink(), "prepared path must be a real directory")
    expected_names = {"MANIFEST.json", "COMPLETION.json", RAW_MAP_NAME,
        *(name + suffix for name in DATASET_NAMES for suffix in (".u16le", ".site_ids.u32le"))}
    require({entry.name for entry in path.iterdir()} == expected_names and
            all((path / name).is_file() and not (path / name).is_symlink() for name in expected_names),
            "missing, extra, nonregular or linked preparation artifact")
    manifest_bytes, completion_bytes = (path / "MANIFEST.json").read_bytes(), (path / "COMPLETION.json").read_bytes()
    manifest, completion = load_json(manifest_bytes), load_json(completion_bytes)
    require(type(manifest.get("raw")) is dict and type(manifest["raw"].get("path")) is str, "missing raw provenance")
    input_path = Path(manifest["raw"]["path"])
    require(input_path.is_absolute() and input_path.resolve() == input_path and input_path.is_file(), "raw provenance path unavailable/noncanonical")
    raw, script_path = input_path.read_bytes(), Path(__file__).resolve()
    script_hash = sha256(script_path.read_bytes())
    metadata, payloads = reconstruct(raw)
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
        boundary_counts=metadata["boundaries"], manifest_sha256=sha256(manifest_bytes), completion_sha256=sha256(completion_bytes),
        verified_by="complete_raw_reconstruction_not_artifact_hashes_only")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="operation", required=True)
    prepare_parser = commands.add_parser("prepare")
    prepare_parser.add_argument("--input", type=Path, required=True)
    prepare_parser.add_argument("--output", type=Path, required=True)
    reader = commands.add_parser("read")
    reader.add_argument("--path", type=Path, required=True)
    args = parser.parse_args()
    def interrupted(signum: int, _frame: Any) -> None:
        raise InterruptedError(f"signal {signum}")
    signal.signal(signal.SIGTERM, interrupted)
    try:
        result = prepare(args.input, args.output) if args.operation == "prepare" else read(args.path)
        print(json.dumps(result, sort_keys=True, allow_nan=False))
        return 0
    except (Exception, KeyboardInterrupt) as error:
        print(json.dumps(dict(status="failed", error=f"{type(error).__name__}: {error}"), sort_keys=True, allow_nan=False), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
