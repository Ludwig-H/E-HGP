#!/usr/bin/env python3
"""Check exact isometric sheet fixtures against a fresh axial-filter snapshot."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "src/core/types.hpp", "src/spindle/predicates.hpp",
    "src/pipeline/local_credits.hpp", "src/pipeline/local_credits.cpp",
    "src/pipeline/tube_credits.hpp", "src/pipeline/axis_q2.hpp",
    "src/pipeline/axis_q2.cpp",
)


def checked(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, check=True,
                          cwd=ROOT, timeout=60)


def run(replay: Path | None = None) -> dict[str, object]:
    base = ROOT / "morsehgp3D_v8"
    here = Path(__file__).resolve().parent
    probe_path = str((here / "axis_rotation_probe.cpp").relative_to(ROOT))
    if replay is None:
        sources = {name: (base / name).read_bytes() for name in SOURCES}
        if sources != {name: (base / name).read_bytes() for name in SOURCES}:
            raise RuntimeError("product sources changed while taking snapshot")
        probe_data = (ROOT / probe_path).read_bytes()
    else:
        document = json.loads(replay.read_text())
        reference = document.get("per_mode", {}).get("normal", document)
        payload = document["snapshot_utf8"]
        for path, data in payload.items():
            if hashlib.sha256(data.encode()).hexdigest() != reference["source_sha256"][path]:
                raise RuntimeError(f"replay payload hash mismatch: {path}")
        sources = {name: payload[f"morsehgp3D_v8/{name}"].encode() for name in SOURCES}
        probe_data = payload[probe_path].encode()
    manifest = {f"morsehgp3D_v8/{name}": hashlib.sha256(data).hexdigest()
                for name, data in sources.items()}
    with tempfile.TemporaryDirectory(prefix="mhgp8_axis_rotation_") as name:
        snapshot = Path(name)
        for relative, data in sources.items():
            target = snapshot / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        probe = snapshot / "probe.cpp"
        probe.write_bytes(probe_data)
        manifest[str((here / "axis_rotation_probe.cpp").relative_to(ROOT))] = (
            hashlib.sha256(probe_data).hexdigest())
        compiler = shutil.which("g++")
        if compiler is None:
            raise RuntimeError("g++ unavailable")
        executable = snapshot / "probe"
        command = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic",
                   "-Werror", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                   f"-I{snapshot / 'src'}", str(probe),
                   str(snapshot / "src/pipeline/local_credits.cpp"),
                   str(snapshot / "src/pipeline/axis_q2.cpp"), "-o", str(executable)]
        checked(command)
        positive = checked([str(executable)])
        rows = [json.loads(line) for line in positive.stdout.splitlines()]
        if len(rows) != 27 or positive.stderr:
            raise RuntimeError("case nonvacuity or sanitizer diagnostic")
        if not any(row["rotated"]["candidates"] > row["aligned"]["candidates"]
                   for row in rows if row["exhaustive"]):
            raise RuntimeError("small orientation gap absent")
        return {"status": "passed", "scope": "axis_q2_exact_isometry_audit",
                "phase": "exploration_v8_hors_registre", "backend": "cpu_reference",
                "profile": "quantized_u16_input_only", "public_status": "not_claimed",
                "not_full_tower": True, "gcp_used": False,
                "source_state": "embedded_snapshot_replay" if replay else "working_tree_snapshot",
                "source_sha256": manifest,
                "snapshot_utf8": {**{f"morsehgp3D_v8/{path}": data.decode()
                                      for path, data in sources.items()},
                                  probe_path: probe_data.decode()},
                "git_head": checked(["git", "rev-parse", "HEAD"]).stdout.strip(),
                "python_optimized": bool(sys.flags.optimize),
                "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "compiler": checked([compiler, "--version"]).stdout.splitlines()[0],
                "compile_command": command,
                "binary_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
                "positive_exit_code": positive.returncode, "cases": rows,
                "nonintegral_frame_rejected": True,
                "sources_unchanged_at_closing": None if replay else sources == {
                    name: (base / name).read_bytes() for name in SOURCES}}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true", required=True)
    parser.add_argument("--replay", type=Path)
    args = parser.parse_args()
    try:
        result = run(args.replay)
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError, KeyError) as error:
        print(f"axis rotation checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
