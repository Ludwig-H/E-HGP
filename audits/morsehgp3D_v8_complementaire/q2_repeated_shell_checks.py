#!/usr/bin/env python3
"""Pin a live census snapshot and test repeated shell incidences independently."""
from __future__ import annotations

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
    "src/pipeline/tube_credits.hpp", "src/pipeline/axis_q2.hpp", "src/pipeline/axis_q2.cpp",
    "src/pipeline/q2_census.hpp", "src/pipeline/q2_census.cpp",
)


def command(args: list[str]) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise RuntimeError(f"command failed {result.returncode}: {args}: {result.stderr}")
    return result


def run(replay: Path | None) -> dict:
    if replay:
        previous = json.loads(replay.read_text())
        sources = previous["sources_utf8"]
        source_hashes = previous["source_sha256"]
    else:
        sources = {path: (ROOT / "morsehgp3D_v8" / path).read_text() for path in SOURCES}
        sources["probe.cpp"] = Path(__file__).with_name("q2_repeated_shell_probe.cpp").read_text()
        source_hashes = {path: hashlib.sha256(text.encode()).hexdigest() for path, text in sources.items()}
        if any((ROOT / "morsehgp3D_v8" / path).read_text() != sources[path] for path in SOURCES):
            raise RuntimeError("product files changed during snapshot")
    if any(hashlib.sha256(text.encode()).hexdigest() != source_hashes[path] for path, text in sources.items()):
        raise RuntimeError("snapshot hash mismatch")
    with tempfile.TemporaryDirectory(prefix="mhgp8_repeated_shell_") as name:
        folder = Path(name)
        for path, text in sources.items():
            target = folder / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text)
        compiler = shutil.which("g++")
        if not compiler:
            raise RuntimeError("g++ required")
        build = [compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                 "-fsanitize=undefined", "-fno-sanitize-recover=all", f"-I{folder / 'src'}",
                 str(folder / "probe.cpp"), str(folder / "src/pipeline/local_credits.cpp"),
                 str(folder / "src/pipeline/axis_q2.cpp"), str(folder / "src/pipeline/q2_census.cpp"),
                 "-o", str(folder / "probe")]
        command(build)
        positive = command([str(folder / "probe")])
        result = json.loads(positive.stdout)
        if result["calls"] != 36 or result["oracle_point_tests"] != 179100 or positive.stderr:
            raise RuntimeError("positive nonvacuity failure")
        probe = folder / "probe.cpp"
        before = "            incidences.emplace_back(pair,key);"
        after = "            if(inserted) incidences.emplace_back(pair,key);"
        if sources["probe.cpp"].count(before) != 1:
            raise RuntimeError("canonical mutation anchor changed")
        probe.write_text(sources["probe.cpp"].replace(before, after))
        command(build)
        mutant = subprocess.run([str(folder / "probe")], capture_output=True, text=True, timeout=60)
        if mutant.returncode != 1 or "canonicalization lost incidences" not in mutant.stderr:
            raise RuntimeError("incidence-loss mutant survived")
    return {
        "status": "passed", "scope": "bounded_actual_census_and_lossless_catalog_normalization",
        "public_status": "not_claimed", "gcp_used": False,
        "context_commit": command(["git", "rev-parse", "HEAD"]).stdout.strip(),
        "compiler": command([compiler, "--version"]).stdout.splitlines()[0],
        "compile_command": build, "source_sha256": source_hashes, "sources_utf8": sources,
        "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "positive_exit_code": positive.returncode, "result": result,
        "mutant": {"name": "deduplicate_incidence_along_with_geometry", "exit_code": mutant.returncode,
                   "diagnostic": mutant.stderr.strip(), "scope": "audit_normalizer_not_product"},
        "product_sources_still_equal_at_close": all(
            (ROOT / "morsehgp3D_v8" / path).read_text() == sources[path] for path in SOURCES),
        "replay": str(replay) if replay else None,
        "timings_measured": False, "production_catalog_implemented": False,
    }


def main() -> int:
    args = sys.argv[1:]
    replay = None
    if len(args) == 2 and args[0] == "--replay":
        replay = Path(args[1])
    elif args != ["--selftest"]:
        print("usage: q2_repeated_shell_checks.py --selftest | --replay receipt.json", file=sys.stderr)
        return 2
    try:
        result = run(replay)
    except (OSError, RuntimeError, subprocess.SubprocessError, ValueError) as error:
        print(f"repeated shell checks failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
