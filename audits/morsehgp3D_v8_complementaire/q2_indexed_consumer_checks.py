#!/usr/bin/env python3
"""Bounded all-site q2 consumer audit on a published v8 source snapshot."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
BASE = "8e406f9b"
SOURCES = (
    "src/core/types.hpp", "src/spindle/predicates.hpp",
    "src/pipeline/local_credits.hpp", "src/pipeline/local_credits.cpp",
    "src/pipeline/tube_credits.hpp", "src/pipeline/axis_q2.hpp",
    "src/pipeline/axis_q2.cpp",
)


def checked(command: list[str], timeout: int = 60) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, check=True,
                          cwd=ROOT, timeout=timeout)


def semantic(rows: list[dict]) -> list[dict]:
    return [{k: v for k, v in row.items() if not k.endswith("_ms")} for row in rows]


def run(large: bool) -> dict[str, object]:
    here = Path(__file__).resolve().parent
    manifest = {}
    with tempfile.TemporaryDirectory(prefix="mhgp8_q2_consumer_") as name:
        directory = Path(name)
        for relative in SOURCES:
            path = f"morsehgp3D_v8/{relative}"
            data = checked(["git", "show", f"{BASE}:{path}"]).stdout.encode()
            manifest[path] = hashlib.sha256(data).hexdigest()
            target = directory / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        data = (here / "q2_indexed_consumer_probe.cpp").read_bytes()
        probe = directory / "probe.cpp"
        probe.write_bytes(data)
        manifest[str((here / "q2_indexed_consumer_probe.cpp").relative_to(ROOT))] = (
            hashlib.sha256(data).hexdigest())
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("g++ unavailable")
        command = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic",
                   "-Werror", f"-I{directory / 'src'}", str(probe),
                   str(directory / "src/pipeline/local_credits.cpp"),
                   str(directory / "src/pipeline/axis_q2.cpp")]
        plain = directory / "plain"
        sanitized = directory / "sanitized"
        checked([*command, "-o", str(plain)])
        sanitized_command = [*command, "-fsanitize=undefined", "-fno-sanitize-recover=all",
                             "-o", str(sanitized)]
        checked(sanitized_command)
        positive = checked([str(plain), "--selftest"])
        sanitized_positive = checked([str(sanitized), "--selftest"])
        rows = [json.loads(line) for line in positive.stdout.splitlines()]
        sanitized_rows = [json.loads(line) for line in sanitized_positive.stdout.splitlines()]
        if len(rows) != 16 or semantic(rows) != semantic(sanitized_rows) or positive.stderr or sanitized_positive.stderr:
            raise RuntimeError("selftest nonvacuity or sanitizer mismatch")
        mutations = (
            ("closed_ball", b"maximum < radius4", b"maximum <= radius4"),
            ("preload_core", b"owner->points()[b], h, 0)",
             b"owner->points()[b], h, owner->core_credit(mhgp8::Lane::Q2))"),
            ("omit_external_sites", b"std::iota(ids_.begin(), ids_.end(), 0);",
             b"std::iota(ids_.begin(), ids_.end(), 0);\n"
             b"    ids_.erase(std::remove_if(ids_.begin(), ids_.end(), [&](auto id) {\n"
             b"      const auto a = owner.a_range(), b = owner.b_range();\n"
             b"      return !(a.first <= id && id < a.last) && !(b.first <= id && id < b.last);\n"
             b"    }), ids_.end());"),
        )
        mutant_results = []
        for label, before, after in mutations:
            if data.count(before) != 1:
                raise RuntimeError(f"mutation target changed: {label}")
            probe.write_bytes(data.replace(before, after))
            checked(sanitized_command)
            result = subprocess.run([str(sanitized), "--selftest"], capture_output=True,
                                    text=True, timeout=60)
            if result.returncode != 1 or "indexed depth differs from independent census" not in result.stderr:
                raise RuntimeError(f"mutant not rejected causally: {label}: {result.stderr}")
            mutant_results.append({"name": label, "exit_code": result.returncode,
                                   "diagnostic": result.stderr.strip()})
        probe.write_bytes(data)
        large_result: dict[str, object] = {"status": "not_requested"}
        if large:
            # plain was built before source mutation; no mutant binary is timed.
            try:
                result = checked([str(plain), "--large"], timeout=60)
                large_rows = [json.loads(line) for line in result.stdout.splitlines()]
                if len(large_rows) != 6 or result.stderr:
                    raise RuntimeError("large construction nonvacuity")
                large_result = {"status": "completed", "exit_code": result.returncode,
                                "cases": large_rows, "timings_not_qualified": True,
                                "binary_sha256": hashlib.sha256(plain.read_bytes()).hexdigest()}
            except subprocess.TimeoutExpired as error:
                raw = error.stdout or b""
                text = raw.decode(errors="replace") if isinstance(raw, bytes) else raw
                large_result = {"status": "interrupted_at_audit_timeout", "timeout_seconds": 60,
                                "stdout": text, "timings_not_qualified": True}
        return {"status": "passed", "scope": "audit_q2_all_site_index_consumer",
                "base_commit": checked(["git", "rev-parse", BASE]).stdout.strip(),
                "source_sha256": manifest,
                "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "compiler": checked([compiler, "--version"]).stdout.splitlines()[0],
                "compile_command": command,
                "positive_exit_codes": [positive.returncode, sanitized_positive.returncode],
                "selftest_semantics_equal": True, "small_cases": rows,
                "mutants": mutant_results, "large": large_result,
                "public_status": "not_claimed", "not_full_tower": True,
                "not_production_consumer": True, "gcp_used": False}


def main() -> int:
    if sys.argv[1:] not in (["--selftest"], ["--selftest", "--large"]):
        print("usage: q2_indexed_consumer_checks.py --selftest [--large]", file=sys.stderr)
        return 2
    try:
        result = run("--large" in sys.argv)
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError) as error:
        print(f"indexed q2 checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
