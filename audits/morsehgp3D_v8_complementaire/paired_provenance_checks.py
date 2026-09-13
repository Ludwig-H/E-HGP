#!/usr/bin/env python3
"""Pin and rebuild small paired probes; preserve heterogeneous-build evidence.

Only temporary snapshots/builds are written. Current source changes are
refused with exit 2; --source-root may select a retained matching snapshot.
No closed receipt is overwritten. Synthetic proxies are clearly separated
from the two genuine builds and the genuine inactive-sheet counterexample.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
PINS = {
    "morsehgp3D_v8/CMakeLists.txt": "85f599fa3c7e4d94c82e9e0eb206304e3c1a9593ef115c571efb6c51f51ffb62",
    "morsehgp3D_v8/src/core/types.hpp": "f4c05da3de254aadf95993988a44a933bf235282bc6f33930984689eba967f2b",
    "morsehgp3D_v8/src/pipeline/axis_q2.hpp": "b19981a14339c6bce8cafec6ee6630aace5e9e743267c7655b5295c2d41653f8",
    "morsehgp3D_v8/src/pipeline/local_credits.hpp": "7d96b38571bacf504d79b4139aaead8042bf1dca26396abd08bd8811799496db",
    "morsehgp3D_v8/src/pipeline/tube_credits.hpp": "8485c7a5c59ae8784bb9085e9e06862435623747ce4df33d3d6059d3bd65c98c",
    "morsehgp3D_v8/src/spindle/predicates.hpp": "38cb2dbc1bf1ad5dd9219d2c3ba3ae68e90003fea92c2371c9a9e853eb973845",
    "morsehgp3D_v8/src/pipeline/axis_q2.cpp": "2cea8416e62886ff19168163975a34b678330eb904b8e291c6308841be975fa3",
    "morsehgp3D_v8/src/pipeline/local_credits.cpp": "24dcbc9839d862e7b9e78864bb41baf726f89c3d48321f0ef8ec1c382aa8d151",
    "morsehgp3D_v8/bench/axis_probe.cpp": "eda7da120fbd32bb99e3e3fb626eb90100e72bfe369089db8412797715846ad2",
    "morsehgp3D_v8/bench/batch_probe.cpp": "1feaab9c065f6b438ce7d7bd90cc29991e16505f181f0d5e5ba7dcd2dac16532",
    "morsehgp3D_v8/bench/check_p0_campaign.py": "6fdfd8fe3b59f025eb8d999480f436895d4c0783c5bf67a2ff7c9874eb4da935",
    "morsehgp3D_v8/bench/check_paired_campaign.py": "b633980545ffa4b29c5deaf38d062ee1ce68fcc5dbaa072f891fb40c8da06572",
    "morsehgp3D_v8/bench/p0_fixtures.hpp": "3c3a7bde624fde5c513ac4946fdceddaf6a392b61643d9f9a847c1b4f470520a",
    "morsehgp3D_v8/bench/p0_probe.cpp": "a95bb5cdce20ecec07c544fc596b684f39a9b573b3a4c6afdbb64c97ae3be552",
    "morsehgp3D_v8/bench/paired_receipts.py": "d6f1ce17cc33f8e0a685b7753964bf244e41c0a10550368eab7a6bcab8459b71",
    "morsehgp3D_v8/bench/probe_emit.hpp": "236331337c44bf25229899a798003e9ce7605ad5df98d90957c0d1ca2dc476d2",
    "morsehgp3D_v8/bench/run_p0_matrix.py": "751420bba712c30334709ee32bd51af7c796d6de5df01fadb9282e74f11ddbe9"
}


class InputChanged(RuntimeError):
    """The pinned draft source is unavailable or has changed."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def invoke(command: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=cwd, text=True, capture_output=True,
                          timeout=60, env=dict(os.environ, GIT_OPTIONAL_LOCKS="0"))


def checked(command: list[str], cwd: Path) -> None:
    result = invoke(command, cwd)
    require(result.returncode == 0, f"command failed: {command}: {result.stderr}")


def reader(root: Path, bundle: Path, kind: str) -> list[dict[str, Any]]:
    script = root / "morsehgp3D_v8/bench" / (
        "check_p0_campaign.py" if kind == "single" else "check_paired_campaign.py")
    results = []
    for optimized in (False, True):
        flags = ["-B", "-O"] if optimized else ["-B"]
        command = [sys.executable, *flags, str(script), str(bundle), "--summary"]
        result = invoke(command, root)
        results.append({
            "python_optimized": optimized, "exit_code": result.returncode,
            "result": json.loads(result.stdout) if result.returncode == 0 else None,
            "last_stderr_line": (result.stderr.splitlines()[-1].replace(
                str(root), "<snapshot>") if result.stderr else ""),
        })
    return results


def campaign(root: Path, probe: Path, output: Path, kind: str,
             extra: list[str]) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    command = [sys.executable, "-B", str(root / "morsehgp3D_v8/bench/run_p0_matrix.py"),
               "--probe", str(probe), "--probe-kind", kind, "--output", str(output),
               "--sizes", "8", "--families", "grid", "--strategies", "tubes",
               "--kmax", "10", "--s", "8", "--repeats", "1", *extra]
    result = invoke(command, root)
    manifest = json.loads((output / "MANIFEST.json").read_text())
    closing = json.loads((output / "COMPLETION.json").read_text())
    rows = [json.loads(line) for line in (output / "MEASURES.jsonl").read_text().splitlines()]
    require(closing["source_hashes_unchanged"] is True and
            closing["probe_hash_unchanged"] is True and
            closing["source_sha256_closing"] == manifest["source_sha256"],
            "unstable campaign source or binary")
    require(closing["attempts"] == len(rows) and
            closing["runs"] == sum(row["status"] == "completed" for row in rows),
            "lost invocation or wrong success accounting")
    descriptions = []
    for row in rows:
        raw = base64.b64decode(row["stdout_base64"], validate=True)
        require(raw.decode("utf-8", errors="replace") == row["stdout"], "stdout lost")
        require(row["probe_sha256_before"] == row["probe_sha256_after"] ==
                manifest["probe_sha256"], "wrong invocation binary pins")
        descriptions.append({
            "arguments": row["command"][1:], "status": row["status"],
            "exit_code": row["exit_code"], "error": row.get("error", ""),
            "raw_stdout_length": len(raw), "raw_stdout_sha256": hashlib.sha256(raw).hexdigest(),
            "checksum_kind": row.get("result", {}).get("checksum_kind"),
        })
    return {
        "runner_exit_code": result.returncode, "completion": closing,
        "probe_sha256": manifest["probe_sha256"],
        "receipt_sha256": {name: digest(output / name) for name in
                           ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json")},
        "rows": descriptions,
    }, rows


def run(source_root: Path) -> dict[str, Any]:
    snapshot_data = {}
    for name, pin in PINS.items():
        path = source_root / name
        if not path.is_file() or digest(path) != pin:
            raise InputChanged(f"changed or unavailable input: {name}")
        snapshot_data[name] = path.read_bytes()
        require(hashlib.sha256(snapshot_data[name]).hexdigest() == pin, "copy race")
    with tempfile.TemporaryDirectory(prefix="paired_provenance_audit_", dir=ROOT / "build") as name:
        temporary = Path(name)
        for relative, data in snapshot_data.items():
            path = temporary / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        build_commands = []
        for build, build_type, flags in (
                ("release", "Release", ["-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG"]),
                ("debug", "Debug", [])):
            configure = ["cmake", "-S", "morsehgp3D_v8", "-B", build,
                         "-DBUILD_TESTING=OFF", f"-DCMAKE_BUILD_TYPE={build_type}", *flags]
            compile_command = ["cmake", "--build", build, "--parallel", "2"]
            checked(configure, temporary)
            checked(compile_command, temporary)
            build_commands.extend((configure, compile_command))
        axis_compile = ["/usr/bin/c++", "-std=c++20", "-O2", "-DNDEBUG", "-Wall",
                        "-Wextra", "-Wpedantic", "-Werror", "-I", "morsehgp3D_v8/src",
                        "morsehgp3D_v8/bench/axis_probe.cpp", "release/libmhgp8_p0.a",
                        "-o", "release/mhgp8_axis_probe"]
        # The captured draft CMake does not register the axis CLI yet; compile
        # this audit executable explicitly without changing its CMake source.
        checked(axis_compile, temporary)
        build_commands.append(axis_compile)
        mixed = {}
        raw_mixed = []
        for build in ("release", "debug"):
            output = temporary / "mixed" / build
            record, rows = campaign(temporary, temporary / build / "mhgp8_batch_probe",
                                    output, "batch", ["--sizes", "128"])
            require(record["runner_exit_code"] == 0 and len(rows) == 2,
                    "genuine mixed-build campaign failed")
            cache = (temporary / build / "CMakeCache.txt").read_text()
            record["build_cache_sha256"] = digest(temporary / build / "CMakeCache.txt")
            record["relevant_cache_lines"] = [line for line in cache.splitlines() if
                line.startswith(("CMAKE_BUILD_TYPE:", "CMAKE_CXX_FLAGS_DEBUG:",
                                 "CMAKE_CXX_FLAGS_RELEASE:"))]
            mixed[build] = record
            raw_mixed.extend(row["result"] for row in rows)
        require(mixed["release"]["probe_sha256"] != mixed["debug"]["probe_sha256"],
                "the heterogeneous-build fixture must use distinct executables")
        mixed_readers = reader(temporary, temporary / "mixed", "batch")
        for result in mixed_readers:
            require(result["exit_code"] == 0, "historical mixed-build acceptance changed")
            summary = result["result"]
            require(summary["campaigns"] == 2 and summary["measurements"] == 4 and
                    summary["configurations_including_order"] == 2, "mixed grouping changed")
            for group in summary["summary"]:
                require(group["repeats"] == 2, "mixed repeats not pooled")
                observations = [row for row in raw_mixed if row["order"] == group["order"]]
                for field in ("baseline_total_ms", "batch_total_ms"):
                    require(group[f"median_{field}"] == statistics.median(
                        row[field] for row in observations), "reported median is not the mixed one")
        results = {"mixed_real_builds": {"campaigns": mixed, "readers": mixed_readers}}
        axis = temporary / "release/mhgp8_axis_probe"
        prelude = (f"#!{sys.executable}\nimport json, subprocess, sys\n"
                   f"real = {str(axis)!r}\n"
                   "result = subprocess.run([real, *sys.argv[1:]], capture_output=True, check=True)\n"
                   "row = json.loads(result.stdout)\n")
        for label, body in (
                ("axis_bad_checksum_kind", "row['checksum_kind'] = 'fnv1a64_be_u64_axis_plan_v0'\nprint(json.dumps(row))\n"),
                ("axis_bad_tuple", "row['n'] += 1\nprint(json.dumps(row))\n"),
                ("axis_truncated", "print('{\"schema\":', end='')\n")):
            proxy = temporary / "release" / label
            proxy.write_text(prelude + body)
            proxy.chmod(0o755)
        cases = (
            ("axis_genuine", axis, "axis", [], 0, 0),
            ("axis_bad_checksum_kind", temporary / "release/axis_bad_checksum_kind", "axis", [], 0, 0),
            ("axis_bad_tuple", temporary / "release/axis_bad_tuple", "axis", [], 1, 1),
            ("axis_truncated", temporary / "release/axis_truncated", "axis", [], 1, 1),
            ("inactive_sheet", temporary / "release/mhgp8_p0_probe", "single",
             ["--families", "sheet", "--strategies", "dual", "--lanes", "3", "--kmax", "1"], 0, 1),
        )
        for label, probe, kind, extra, runner_exit, reader_exit in cases:
            bundle = temporary / label
            result, rows = campaign(temporary, probe, bundle / "case", kind, extra)
            require(result["runner_exit_code"] == runner_exit,
                    f"historical runner behavior changed: {label}")
            result["readers"] = reader(temporary, bundle, kind)
            require(all(item["exit_code"] == reader_exit for item in result["readers"]),
                    f"historical reader behavior changed: {label}")
            if label == "axis_bad_tuple":
                require(result["completion"]["status"] == "invalid" and
                        "paired command/result mismatch: n" in rows[0]["error"],
                        "tuple refusal was not causal")
            elif label == "axis_truncated":
                require(result["completion"]["status"] == "invalid" and
                        "Expecting value:" in rows[0]["error"] and bool(rows[0]["stdout"]),
                        "truncated stdout was lost or wrongly refused")
            elif label == "inactive_sheet":
                row = rows[0]["result"]
                require(row["threshold"] == 0 and row["candidate_pairs"] == 0 and
                        all(item["last_stderr_line"] == "RuntimeError: sheet counter-fixture changed"
                            for item in result["readers"]), "inactive-sheet cause changed")
            elif label == "axis_bad_checksum_kind":
                require(all(row["result"]["checksum_kind"] == "fnv1a64_be_u64_axis_plan_v0"
                            for row in rows), "checksum-kind proxy did not mutate the marker")
            results[label] = result
        require(len(results) == 6, "non-vacuity floor")
        require(all(digest(temporary / relative) == pin for relative, pin in PINS.items()),
                "snapshot source changed")
        return {
            "status": "paired_provenance_gaps_reproduced_with_positive_controls",
            "scope": "tiny_receipt_protocol_checks_not_a_performance_or_geometry_claim",
            "phase": "exploration_v8_hors_registre", "backend": "cpu_reference",
            "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture",
            "public_status": "not_claimed", "gcp_used": False,
            "source_sha256": PINS, "audit_script_sha256": digest(Path(__file__)),
            "build_commands": build_commands, "results": results,
            "verdicts": {"P1_mixed_build_medians": "open", "P2_axis_checksum_kind": "open",
                         "P2_inactive_sheet_reader": "still_open",
                         "wrong_tuple_and_truncated_stdout": "correctly_rejected_and_recorded"},
        }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", required=True, action="store_true")
    parser.add_argument("--source-root", type=Path, default=ROOT)
    args = parser.parse_args()
    try:
        result = run(args.source_root.resolve())
    except InputChanged as error:
        print(f"source pin changed; closed paired receipt preserved: {error}", file=sys.stderr)
        return 2
    except (OSError, RuntimeError, ValueError, KeyError, TypeError,
            subprocess.SubprocessError) as error:
        print(f"paired provenance audit failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
