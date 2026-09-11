"""Create-only, bounded qualification of the frozen private assembler."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import time


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
HEADER = "morsehgp3D_v7/src/forest/full_coverage_incremental.hpp"
JOURNAL = "morsehgp3D_v7/src/forest/full_coverage_certificate.hpp"
MUTANTS = {
    "future_root": (
        "if (!full_coverage_detail::admitted(forest.nodes()[next].level, cut, closed)) break;",
        "// MUTANT: follow every future successor.",
    ),
    "backdated_contribution": (
        "out_.contributions_.push_back({level, segment, ref});",
        "out_.contributions_.push_back({out_.nodes_[segment].level, segment, ref});",
    ),
    "reused_continuation": (
        "live_[parent] = 0;",
        "live_[parent] = (action.parents.size() == 1);",
    ),
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unpack(target: Path) -> dict[str, str]:
    pins = json.loads((HERE / "source_pins.json").read_text())
    archive = HERE / "source_snapshot.tar.gz"
    if sha(archive) != pins["archive_sha256"]:
        raise ValueError("source archive changed")
    with tarfile.open(archive) as source:
        members = source.getmembers()
        if sorted(m.name for m in members) != sorted(pins["files"]):
            raise ValueError("source archive membership changed")
        for member in members:
            path = target / member.name
            if not member.isfile() or not path.resolve().is_relative_to(target.resolve()):
                raise ValueError("invalid source member")
            raw = source.extractfile(member).read()
            if hashlib.sha256(raw).hexdigest() != pins["files"][member.name]:
                raise ValueError("source member changed")
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
    return pins["files"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True)
    parser.add_argument("--san", action="store_true")
    parser.add_argument("--mutants", action="store_true")
    args = parser.parse_args()
    if not args.name.isidentifier() or (args.san and args.mutants):
        parser.error("use a fresh identifier and one mode")
    out = HERE / (args.name + ".json")
    work = HERE / (".work_" + args.name)
    if out.exists() or work.exists():
        parser.error("capture already exists")
    work.mkdir()
    source = work / "source"
    source_pins = unpack(source)
    inputs = {name: sha(HERE / name) for name in (
        "record.py", "prefix_gate.cpp", "source_pins.json", "source_snapshot.tar.gz")}
    fixture = REPO / json.loads((HERE / "source_pins.json").read_text())["fixture_origin"]["path"]
    expected_fixture = json.loads((HERE / "source_pins.json").read_text())["fixture_origin"]["sha256"]
    if sha(fixture) != expected_fixture:
        raise ValueError("historical fixture changed")
    compiler = shutil.which("g++")
    boost = REPO / "build/v7_boost_gate/extracted/usr/include"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-isystem", str(boost)]
    flags += (["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
              if args.san else ["-O2"])
    env = os.environ.copy()
    if args.san:
        env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
        env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt = {
        "schema": "mhgp7-independent-prefix-capture-v1", "status": "running",
        "authority": "private_structural_prototype_only", "public_status": "not_claimed",
        "gcp_used": False, "sanitizers": args.san, "mutants": args.mutants,
        "inputs_before": inputs, "commands": [], "cwd": str(REPO),
        "compiler_sha256": sha(Path(compiler)),
        "boost_version_sha256": sha(boost / "boost/version.hpp"),
        "sanitizer_options": {key: env[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS") if key in env},
    }

    def save() -> None:
        out.write_text(json.dumps(receipt, indent=2) + "\n")

    def command(name: str, argv: list[str], expected: int = 0) -> None:
        start = time.time()
        run = subprocess.run(argv, cwd=REPO, env=env, capture_output=True, timeout=180)
        stdout, stderr = run.stdout.decode(), run.stderr.decode()
        receipt["commands"].append(dict(name=name, argv=argv, expected_exit=expected,
            exit_code=run.returncode, started_unix=start, seconds=time.time() - start,
            stdout=stdout, stderr=stderr))
        save()
        print(name, run.returncode, flush=True)
        if run.returncode != expected:
            raise ValueError(name + ": unexpected exit")

    save()
    try:
        command("compiler_version", [compiler, "--version"])
        original = (source / JOURNAL).read_text()
        for flavor in (["nominal", *MUTANTS] if args.mutants else ["nominal"]):
            changed = original
            if flavor != "nominal":
                before, after = MUTANTS[flavor]
                if original.count(before) != 1:
                    raise ValueError("mutation precondition: " + flavor)
                changed = original.replace(before, after)
            (source / JOURNAL).write_text(changed)
            binary = work / flavor
            command("compile_" + flavor, [compiler, *flags,
                '-DMHGP7_AUDIT_HEADER="' + str(source / HEADER) + '"',
                str(HERE / "prefix_gate.cpp"), "-o", str(binary)])
            command("run_" + flavor, [str(binary)], 0 if flavor == "nominal" else 1)
            if flavor == "nominal":
                command("reject_arguments", [str(binary), "unexpected"], 2)
        (source / JOURNAL).write_text(original)
        receipt["status"] = "passed"
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
    finally:
        receipt["inputs_after"] = {name: sha(HERE / name) for name in inputs}
        receipt["inputs_stable"] = inputs == receipt["inputs_after"]
        # Restore no failed source: the mutation and first refusal remain visible.
        receipt["compiled_source_final_pins"] = {name: sha(source / name) for name in source_pins}
        if not receipt["inputs_stable"]:
            receipt["status"] = "failed"
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
