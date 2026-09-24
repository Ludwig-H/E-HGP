#!/usr/bin/env python3
"""Independent, fail-closed reader of C's published raw CPU pin receipt.

This reads captured files and versioned inputs; it does not execute the engine.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
RECEIPT = ROOT / "morsehgp3D_v9/audits/c_raw_pins_20260924"
INPUT_DIR = ROOT / "morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i"
BASE = "093d943cee7bd2a465a034f8f9bae879a0cd5f5b"
PINS_SHA256 = "0487f1f80eb44926f8661289bda2ffc7dd919ea2acb9c5a289ad9f692f30bf6c"
PROBE_SHA256 = "1afdf9948624949f6b593db055ca95e3b1454a5ca168af6dacfd6dcf0b0aa3b1"
SCENES = {
    "b00": ("08/000000", "scene_00_000000_grid", 123389,
            "233cc4ea8cac6e0b1155ea845af57b32e5236764bf2e119557aeab5bac76c172", "4120701a6194c19b"),
    "b01": ("08/000100", "scene_01_000100_grid", 124479,
            "de45e8dcaf5610cd71a369b613f16914d5713e77cbe1532122ec2e823bc0b4ad", "d2bd37fb9befdd7d"),
    "b02": ("08/000200", "scene_02_000200_grid", 125526,
            "37a7be399fae909a1291cddfc3ca5b972d3effdfa8dc8cd87a41accd5fa0f5f7", "583db2f3deafe8e9"),
}
ARMS = ("engine", "batch")
ORDERS = (5, 10)
BATCH_LEVERS = ("q34_batch_filter", "q34_batch_certificates", "q34_batch_q3", "q34_batch_q4")
GPU_LEVERS = ("q34_gpu_filter", "q34_gpu_certificates", "q34_gpu_q3")


class ReaderError(Exception):
    pass


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ReaderError(message)


def equal(actual: object, expected: object, message: str) -> None:
    require(type(actual) is type(expected) and actual == expected,
            f"{message}: got {actual!r}, expected {expected!r}")


def object_no_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_constant(value: str) -> None:
    raise ReaderError(f"non-JSON numeric constant: {value}")


def load_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=object_no_duplicates,
                       parse_constant=reject_constant)
    require(type(value) is dict, f"{path}: top level is not an object")
    return value


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def manifest_names() -> set[str]:
    names = {"BASE.txt", "PINS_RAW.json", "README.md", "pin_raw.sh", "probe.sha256"}
    for scene in SCENES:
        for k in ORDERS:
            for arm in ARMS:
                for suffix in ("json", "time"):
                    names.add(f"probes/{scene}_k{k}_{arm}.{suffix}")
    return names


def check_manifest(receipt: Path) -> None:
    lines = (receipt / "SHA256SUMS").read_text(encoding="utf-8").splitlines()
    found: dict[str, str] = {}
    for line in lines:
        match = re.fullmatch(r"([0-9a-f]{64})  \./([^\s]+)", line)
        require(match is not None, f"malformed SHA256SUMS line: {line!r}")
        digest, name = match.groups()
        require(name not in found, f"duplicate manifest path: {name}")
        found[name] = digest
    equal(set(found), manifest_names(), "manifest file set")
    for name, digest in found.items():
        path = receipt / name
        require(path.is_file() and not path.is_symlink(), f"missing or linked receipt file: {name}")
        equal(sha256(path), digest, f"SHA256SUMS {name}")
    equal(found["PINS_RAW.json"], PINS_SHA256, "published PINS_RAW SHA256")


def fnv_word(hash_value: int, word: int) -> int:
    for _ in range(8):
        hash_value = ((hash_value ^ (word & 255)) * 1099511628211) & ((1 << 64) - 1)
        word >>= 8
    return hash_value


def read_inputs(root: Path) -> dict[str, tuple[int, str, str]]:
    metadata = {}
    input_dir = root / INPUT_DIR.relative_to(ROOT)
    for scene, (_, folder, sites, expected_sha, expected_fnv) in SCENES.items():
        path = input_dir / folder / "full.u32le"
        require(path.is_file() and not path.is_symlink(), f"missing versioned input: {path}")
        data = path.read_bytes()
        equal(len(data), sites * 12, f"{scene} input bytes")
        actual_sha = hashlib.sha256(data).hexdigest()
        equal(actual_sha, expected_sha, f"{scene} input SHA256")
        h = fnv_word(14695981039346656037, sites)
        for (word,) in struct.iter_unpack("<I", data):
            require(word < (1 << 18), f"{scene} coordinate exceeds u18")
            h = fnv_word(h, word)
        actual_fnv = f"{h:016x}"
        equal(actual_fnv, expected_fnv, f"{scene} input FNV")
        metadata[scene] = (sites, actual_sha, actual_fnv)
    return metadata


def check_time(path: Path, scene: str, k: int, arm: str) -> None:
    body = path.read_text(encoding="utf-8")
    codes = re.findall(r"^\s*Exit status: ([0-9]+)\s*$", body, flags=re.MULTILINE)
    equal(codes, ["0"], f"{path.name} GNU time exit")
    commands = re.findall(r'^\s*Command being timed: "(.+)"\s*$', body, flags=re.MULTILINE)
    require(len(commands) == 1, f"{path.name} GNU time command missing or repeated")
    args = shlex.split(commands[0])
    require(len(args) >= 5 and Path(args[0]).name == "mhgp9_tower_probe", f"{path.name} executable")
    folder = SCENES[scene][1]
    expected_suffix = f"/morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/{folder}/full.u32le"
    require(args[1].endswith(expected_suffix), f"{path.name} input command mismatch")
    equal(args[2:5], [str(k), "8", "--catalogue-digest"], f"{path.name} K/W/digest command")
    expected_levers = [f"--lever={name}=1" for name in BATCH_LEVERS] if arm == "batch" else []
    equal(args[5:], expected_levers, f"{path.name} arm command")


def check_case(data: dict, scene: str, k: int, arm: str, metadata: tuple[int, str, str]) -> None:
    label = f"{scene} K{k} {arm}"
    sites, _, fnv = metadata
    equal(data.get("schema"), "mhgp9_tower_probe_v26", f"{label} schema")
    equal(data.get("status"), "complete_relative", f"{label} status")
    equal(data.get("reason"), "complete_relative_to_cross_checked_catalogue", f"{label} reason")
    input_row = data.get("input")
    require(type(input_row) is dict, f"{label} input object")
    for key, value in {"format": "u32le", "grid": "unspecified", "sites": sites, "hash": fnv}.items():
        equal(input_row.get(key), value, f"{label} input.{key}")
    options = data.get("options")
    require(type(options) is dict, f"{label} options object")
    for key, value in {"K": k, "K_effective": k, "s": 8, "workers": 8,
                       "tower_static_threads": 8, "run_tower": True}.items():
        equal(options.get(key), value, f"{label} options.{key}")
    levers = options.get("levers")
    require(type(levers) is dict, f"{label} levers object")
    for key in BATCH_LEVERS:
        equal(levers.get(key), arm == "batch", f"{label} lever {key}")
    for key in GPU_LEVERS:
        equal(levers.get(key), False, f"{label} lever {key}")
    for key in ("tower_digest", "catalogue_digest", "presentation_digest"):
        require(type(data.get(key)) is str and re.fullmatch(r"[0-9a-f]{16}", data[key]) is not None,
                f"{label} invalid {key}")
    catalogue = data.get("catalogue")
    work = data.get("tower_work")
    orders = data.get("orders")
    require(type(catalogue) is dict and type(work) is dict and type(orders) is list,
            f"{label} catalogue/work/orders shape")
    balls = catalogue.get("balls")
    require(type(balls) is int and balls > 0, f"{label} positive balls")
    equal(catalogue.get("unique_keys"), balls, f"{label} unique keys")
    equal(work.get("records"), balls, f"{label} tower work records")
    equal(len(orders), k, f"{label} order count")
    for order_number, order in enumerate(orders, 1):
        require(type(order) is dict, f"{label} order {order_number} object")
        equal(set(order), {"K", "nodes", "births", "merges", "parents", "contributions"},
              f"{label} order {order_number} fields")
        equal(order["K"], order_number, f"{label} order index")
        for field in ("nodes", "births", "merges", "parents", "contributions"):
            require(type(order[field]) is int and order[field] >= 0, f"{label} order {order_number} {field}")
        equal(order["nodes"], order["births"] + order["merges"], f"{label} order node accounting")
        equal(order["parents"], order["nodes"] - 1, f"{label} order parent accounting")
    euler = catalogue.get("euler")
    require(type(euler) is dict, f"{label} Euler object")
    equal(euler.get("status"), "holds", f"{label} Euler status")
    equal(euler.get("checkable_max_k"), k - 2, f"{label} Euler checkable bound")
    by_k = euler.get("by_k")
    require(type(by_k) is list, f"{label} Euler by_k")
    equal(len(by_k), k, f"{label} Euler by_k count")
    for index in range(k - 2):
        equal(by_k[index], 1, f"{label} Euler K{index + 1}")
    batch = data.get("q34_batch")
    require(type(batch) is dict, f"{label} q34_batch object")
    equal(batch.get("used"), arm == "batch", f"{label} batch used")
    backend = "cpu" if arm == "batch" else ""
    for field in ("backend", "certificate_backend", "lanes_backend"):
        equal(batch.get(field), backend, f"{label} q34_batch.{field}")
    require(type(batch.get("lanes_deferred")) is int and batch["lanes_deferred"] >= 0,
            f"{label} lanes deferred")
    require(type(data.get("chain_cpu_s")) in (int, float) and data["chain_cpu_s"] > 0,
            f"{label} chain CPU time")


def verify(receipt: Path = RECEIPT, root: Path = ROOT,
           inputs: dict[str, tuple[int, str, str]] | None = None) -> str:
    check_manifest(receipt)
    base_file = (receipt / "BASE.txt").read_text(encoding="utf-8").strip()
    equal(base_file, f"base={BASE}", "BASE.txt")
    marker = (receipt / "probe.sha256").read_text(encoding="utf-8").strip()
    require(re.fullmatch(r"[0-9a-f]{64}  \S+/mhgp9_tower_probe", marker) is not None,
            "probe.sha256 marker format")
    equal(marker.split("  ", 1)[0], PROBE_SHA256, "declared probe binary SHA256")
    pins_doc = load_json(receipt / "PINS_RAW.json")
    equal(pins_doc.get("schema"), "c_raw_pins_v1", "PINS_RAW schema")
    equal(pins_doc.get("base"), BASE, "PINS_RAW base")
    equal(pins_doc.get("probe"), "mhgp9_tower_probe v26 (Release CPU, W8, s8, --catalogue-digest)",
          "PINS_RAW probe")
    rows = pins_doc.get("pins")
    require(type(rows) is list and len(rows) == 6, "PINS_RAW must contain six rows")
    if inputs is None:
        inputs = read_inputs(root)
    expected_keys = {(scene, k) for scene in SCENES for k in ORDERS}
    seen = set()
    for row in rows:
        require(type(row) is dict, "PINS_RAW row is not an object")
        equal(set(row), {"scene", "frame", "input_sha256", "input_fnv", "sites", "K", "s", "balls",
                         "tower_digest", "catalogue_digest", "euler", "arms_equal", "batch_lanes_deferred",
                         "chain_cpu_s_engine"}, "PINS_RAW row fields")
        scene, k = row["scene"], row["K"]
        require((scene, k) in expected_keys and (scene, k) not in seen, f"unexpected/duplicate pin row {scene} K{k}")
        seen.add((scene, k))
        frame, _, sites, sha, fnv = SCENES[scene]
        for field, value in {"frame": frame, "input_sha256": sha, "input_fnv": fnv,
                             "sites": sites, "s": 8, "euler": "holds",
                             "arms_equal": ["engine", "batch_cpu(filter,certificates,q3,q4)"],
                             }.items():
            equal(row[field], value, f"{scene} K{k} pin {field}")
        equal(inputs[scene], (sites, sha, fnv), f"{scene} verified input")
        cases = {}
        for arm in ARMS:
            stem = f"{scene}_k{k}_{arm}"
            data = load_json(receipt / "probes" / f"{stem}.json")
            check_case(data, scene, k, arm, inputs[scene])
            check_time(receipt / "probes" / f"{stem}.time", scene, k, arm)
            equal(data["catalogue"]["balls"], row["balls"], f"{stem} pinned balls")
            equal(data["tower_digest"], row["tower_digest"], f"{stem} pinned tower digest")
            equal(data["catalogue_digest"], row["catalogue_digest"], f"{stem} pinned catalogue digest")
            cases[arm] = data
        engine, batch = cases["engine"], cases["batch"]
        for field in ("tower_digest", "catalogue_digest", "presentation_digest", "orders", "catalogue", "tower_work"):
            equal(batch[field], engine[field], f"{scene} K{k} engine/batch {field}")
        equal(batch["q34_batch"]["lanes_deferred"], row["batch_lanes_deferred"],
              f"{scene} K{k} pinned deferred lanes")
        equal(engine["chain_cpu_s"], row["chain_cpu_s_engine"], f"{scene} K{k} pinned engine CPU time")
    equal(seen, expected_keys, "PINS_RAW scene/K coverage")
    return "12/12 cases, 6/6 exact pairs, 3/3 versioned inputs, 29/29 receipt SHA256; JSON grid=unspecified (1mm input provenance)"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", type=Path, default=RECEIPT)
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    args = parser.parse_args()
    try:
        summary = verify(args.receipt, args.repo_root)
    except (ReaderError, OSError, ValueError, KeyError, TypeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print(f"PASS: {summary}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
