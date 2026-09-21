#!/usr/bin/env python3
"""Apply a whole-frame ground mask without changing retained coordinates.

Explicit runtime reuse of prepare_lidar_precision.reconstruct, not a port of
its geometry. Raw geometry/quantization is prepared BEFORE selection, so the
grid origin and original site IDs remain those of the complete source frame.
The mask is an input, not a proof that semantic ground was correctly detected.
Both scripts, the raw source and the mask/descriptor are pinned before/after.
No u16 engine, HGP census, hierarchy, or performance contract is qualified.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import importlib.util
from pathlib import Path
import signal
import struct
import sys

_spec = importlib.util.spec_from_file_location("mhgp8_ground_precision", Path(__file__).with_name("prepare_lidar_precision.py"))
base = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(base)

SCHEMA = "mhgp8_lidar_ground_preparation_v1"
COMPLETION_SCHEMA = "mhgp8_lidar_ground_preparation_completion_v1"
MASK_SCHEMA = "mhgp8_lidar_ground_mask_v1"
ENCODING = "u8_0_unknown_1_ground_2_nonground"
DATASET_NAMES = base.DATASET_NAMES
SOURCES = (Path(__file__).resolve(), Path(base.__file__).resolve())
InvalidPreparation = base.InvalidPreparation
require, sha256, canonical_json = base.require, base.sha256, base.canonical_json
load_json, stamp, _current_hash = base.load_json, base.stamp, base._current_hash


def descriptor(raw: bytes, mask: bytes, producer: dict) -> dict:
    return dict(schema=MASK_SCHEMA, input_sha256=sha256(raw), mask_sha256=sha256(mask),
                raw_returns=len(raw)//16, encoding=ENCODING, producer=producer)


def reconstruct(raw: bytes, mask: bytes, mask_metadata: dict,
                profile: str = "float32", precision_mm: str | None = None) -> tuple[dict, dict[str, bytes]]:
    require(type(raw) is bytes and len(raw) > 0 and len(raw) % 16 == 0, "raw frame must be nonempty complete xyzi")
    n = len(raw)//16
    require(type(mask) is bytes and len(mask) == n and all(value in (0, 1, 2) for value in mask),
            "mask must contain exactly one declared u8 status per original return")
    require(type(mask_metadata) is dict and set(mask_metadata) ==
            {"schema", "input_sha256", "mask_sha256", "raw_returns", "encoding", "producer"}, "mask descriptor fields")
    require(mask_metadata["schema"] == MASK_SCHEMA and mask_metadata["encoding"] == ENCODING and
            type(mask_metadata["raw_returns"]) is int and mask_metadata["raw_returns"] == n and
            mask_metadata["input_sha256"] == sha256(raw) and mask_metadata["mask_sha256"] == sha256(mask) and
            type(mask_metadata["producer"]) is dict, "mask descriptor does not bind this raw frame and mask")
    # Reject non-JSON/non-finite producer metadata. Own a normalized copy.
    mask_metadata = load_json(canonical_json(mask_metadata))
    original, original_payloads = base.reconstruct(raw, profile, precision_mm)
    source_map = original_payloads[base.RAW_MAP_NAME]
    raw_to_original = [value[0] for value in struct.iter_unpack("<I", source_map)]
    total_sites = original["counts"]["unique_sites"]
    flags = bytearray(total_sites)
    kept_returns, removed_returns = [], []
    for identifier, (site, state) in enumerate(zip(raw_to_original, mask, strict=True)):
        flags[site] |= 1 if state == 1 else 2
        (removed_returns if state == 1 else kept_returns).append(identifier)
    retained = [site for site, flag in enumerate(flags) if flag & 2]
    original_kept = bytes(int(bool(flag & 2)) for flag in flags)
    retained_id = {site: i for i, site in enumerate(retained)}
    suffix = ".f32le" if profile == "float32" else ".u32le"
    full = original_payloads["full"+suffix]
    payloads = {"mask.u8": mask, "raw_to_original.u32le": source_map,
                "retained_to_original.u32le": base._ids_bytes(retained), "original_kept.u8": original_kept,
                "kept_return_ids.u32le": base._ids_bytes(kept_returns),
                "removed_return_ids.u32le": base._ids_bytes(removed_returns)}
    datasets = {}
    for name in DATASET_NAMES:
        candidates = struct.iter_unpack("<I", original_payloads[name+".site_ids.u32le"])
        selected = [site for (site,) in candidates if original_kept[site]]
        local = [retained_id[site] for site in selected]
        points = b"".join(full[12*site:12*site+12] for site in selected)
        points_name, ids_name, original_name = name+suffix, name+".site_ids.u32le", name+".original_site_ids.u32le"
        payloads.update({points_name: points, ids_name: base._ids_bytes(local), original_name: base._ids_bytes(selected)})
        datasets[name] = dict(sites=len(local), points_file=points_name, point_bytes=len(points), points_sha256=sha256(points),
                             site_ids_file=ids_name, original_site_ids_file=original_name,
                             site_ids_sha256=sha256(payloads[ids_name]), original_site_ids_sha256=sha256(payloads[original_name]),
                             local_order="increasing_retained_site_ID_same_order_as_original_site_ID")
    counts = dict(unknown=mask.count(0), ground=mask.count(1), nonground=mask.count(2))
    metadata = dict(parameters=original["parameters"], profile=original["profile"], raw_preparation=original,
        mask=mask_metadata, ground=dict(counts=counts, retained_returns=len(kept_returns), removed_returns=len(removed_returns),
            original_sites=total_sites, retained_sites=len(retained), removed_sites=total_sites-len(retained),
            mixed_decision_sites=sum(flag == 3 for flag in flags),
            policy="only_ground_returns_removed_site_kept_if_any_return_kept",
            geometric_exactness="coordinates_only_not_semantic_ground_or_HGP_qualification"),
        mappings=dict(raw_to_original="raw_to_original.u32le", retained_to_original="retained_to_original.u32le",
            original_kept="original_kept.u8", raw_order_preserved=True, sentinel_used=False,
            ground_return_with_retained_duplicate="raw_to_original_still_maps_to_the_retained_original_site"),
        partition=dict(mask_scope="whole_original_frame_before_spatial_partition",
            planes=original["partition"]["raw_reference_planes"], encoded_sensor_origin=original["partition"]["encoded_sensor_origin"],
            inherited_original_partitions=True, subsampling=False, empty_retained_cloud_allowed=True), datasets=datasets)
    return metadata, payloads


def _write_bytes(path: Path, data: bytes) -> None:
    with path.open("xb") as output:
        output.write(data)


def _pins(paths) -> dict:
    return {str(Path(path)): _current_hash(Path(path)) for path in paths}


def _manifest(paths, sources, metadata) -> dict:
    return dict(schema=SCHEMA, status="prepared", public_status="not_claimed", scope="input_mask_and_seven_partitions_not_HGP",
                input_paths=[str(path) for path in paths], input_sha256=_pins(paths), source_sha256=sources,
                reuse="prepare_lidar_precision.reconstruct_on_complete_raw_before_mask", **metadata)


def prepare(input_path: Path, mask_path: Path, mask_receipt_path: Path, output_path: Path,
            profile: str = "float32", precision_mm: str | None = None) -> dict:
    paths = tuple(Path(path).resolve(strict=True) for path in (input_path, mask_path, mask_receipt_path))
    output = Path(output_path).absolute()
    require(all(path.is_file() for path in paths) and len(set(paths)) == 3, "distinct input, mask and descriptor files required")
    require(not output.exists() and not output.is_symlink(), "fresh output directory required")
    before = _pins((*paths, *SOURCES))
    require(all(type(value) is str and len(value) == 64 for value in before.values()), "missing input or preparation source")
    raw, mask, descriptor_bytes = (path.read_bytes() for path in paths)
    metadata, payloads = reconstruct(raw, mask, load_json(descriptor_bytes), profile, precision_mm)
    manifest = _manifest(paths, _pins(SOURCES), metadata)
    manifest_bytes = canonical_json(manifest)
    require(_pins(before) == before, "input or source changed during reconstruction")
    expected = {name: sha256(value) for name, value in payloads.items()}
    output.mkdir(parents=True, exist_ok=False)
    started, error = stamp(), None
    try:
        _write_bytes(output/"MANIFEST.json", manifest_bytes)
        for name, data in payloads.items():
            _write_bytes(output/name, data)
        require({p.name for p in output.iterdir()} == {"MANIFEST.json", *payloads}, "unexpected output artifact")
        require(all((output/name).read_bytes() == data for name, data in payloads.items()), "written output differs")
    except BaseException as cause:
        error = cause
    after, artifact_after = _pins(before), {name: _current_hash(output/name) for name in payloads}
    try:
        require(after == before and artifact_after == expected and _current_hash(output/"MANIFEST.json") == sha256(manifest_bytes),
                "preparation source/input/output closure changed")
    except BaseException as cause:
        if error is None: error = cause
    completion = dict(schema=COMPLETION_SCHEMA, status="passed" if error is None else "failed", started_utc=started,
        finished_utc=stamp(), error=None if error is None else f"{type(error).__name__}: {error}",
        manifest_sha256=_current_hash(output/"MANIFEST.json"), input_source_sha256_after=after, output_sha256=artifact_after)
    _write_bytes(output/"COMPLETION.json", canonical_json(completion))
    if error is not None: raise error
    return _summary(output, manifest_bytes, canonical_json(completion), metadata)


def _summary(path, manifest_bytes, completion_bytes, metadata) -> dict:
    return dict(schema=SCHEMA, status="passed", path=str(path.resolve()), datasets=7, parameters=metadata["parameters"],
                ground=metadata["ground"], dataset_sites={name: value["sites"] for name, value in metadata["datasets"].items()},
                manifest_sha256=sha256(manifest_bytes), completion_sha256=sha256(completion_bytes))


def read(path: Path) -> dict:
    path = Path(path).absolute()
    require(path.is_dir() and not path.is_symlink(), "prepared path must be a real directory")
    require(all((path/name).is_file() and not (path/name).is_symlink() for name in ("MANIFEST.json", "COMPLETION.json")),
            "missing or linked metadata")
    manifest_bytes, completion_bytes = ((path/name).read_bytes() for name in ("MANIFEST.json", "COMPLETION.json"))
    manifest, completion = load_json(manifest_bytes), load_json(completion_bytes)
    require(type(manifest.get("input_paths")) is list and all(type(value) is str for value in manifest["input_paths"]) and
            type(manifest.get("parameters")) is dict and set(manifest["parameters"]) == {"profile", "precision_mm"},
            "input/configuration shape differs")
    paths = tuple(Path(value) for value in manifest["input_paths"])
    require(len(paths) == len(set(paths)) == 3 and all(p.is_absolute() and p.resolve() == p and p.is_file() for p in paths),
            "noncanonical or unavailable source paths")
    before = _pins((*paths, *SOURCES))
    require(all(type(value) is str and len(value) == 64 for value in before.values()), "missing input or preparation source")
    config = base.parameters(**manifest["parameters"])
    raw, mask, descriptor_bytes = (p.read_bytes() for p in paths)
    metadata, payloads = reconstruct(raw, mask, load_json(descriptor_bytes), **config)
    expected = _manifest(paths, _pins(SOURCES), metadata)
    require(manifest_bytes == canonical_json(expected), "manifest differs from complete raw/mask reconstruction")
    names = {"MANIFEST.json", "COMPLETION.json", *payloads}
    require({p.name for p in path.iterdir()} == names and
            all((path/name).is_file() and not (path/name).is_symlink() for name in names), "artifact inventory/type differs")
    require(all((path/name).read_bytes() == data for name, data in payloads.items()), "payload differs from reconstruction")
    require(all(type(completion.get(field)) is str for field in ("started_utc", "finished_utc")), "timestamp types differ")
    started, finished = (datetime.fromisoformat(completion[field]) for field in ("started_utc", "finished_utc"))
    require(started.utcoffset() == finished.utcoffset() == timezone.utc.utcoffset(None) and started <= finished,
            "timestamps must be ordered UTC")
    expected_completion = dict(schema=COMPLETION_SCHEMA, status="passed", started_utc=completion["started_utc"],
        finished_utc=completion["finished_utc"], error=None, manifest_sha256=sha256(manifest_bytes),
        input_source_sha256_after=before, output_sha256={name: sha256(data) for name, data in payloads.items()})
    require(completion_bytes == canonical_json(expected_completion), "completion differs from exact closed preparation")
    require(_pins(before) == before and (path/"MANIFEST.json").read_bytes() == manifest_bytes and
            (path/"COMPLETION.json").read_bytes() == completion_bytes and {p.name for p in path.iterdir()} == names and
            all((path/name).is_file() and not (path/name).is_symlink() and (path/name).read_bytes() == data for name, data in payloads.items()),
            "inputs/sources/artifacts changed during reading")
    return _summary(path, manifest_bytes, completion_bytes, metadata)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="operation", required=True)
    writer = commands.add_parser("prepare")
    for name in ("input", "mask", "mask-receipt", "output"):
        writer.add_argument("--"+name, type=Path, required=True)
    writer.add_argument("--profile", choices=base.PROFILES, default="float32")
    writer.add_argument("--precision-mm")
    reader = commands.add_parser("read")
    reader.add_argument("--path", type=Path, required=True)
    args = parser.parse_args()
    def interrupted(signum, _frame):
        raise InterruptedError(f"signal {signum}")
    signal.signal(signal.SIGTERM, interrupted)
    try:
        result = (prepare(args.input, args.mask, args.mask_receipt, args.output, args.profile, args.precision_mm)
                  if args.operation == "prepare" else read(args.path))
        print(canonical_json(result).decode(), end="")
        return 0
    except (Exception, KeyboardInterrupt) as error:
        print(canonical_json(dict(status="failed", error=f"{type(error).__name__}: {error}")).decode(), end="", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
