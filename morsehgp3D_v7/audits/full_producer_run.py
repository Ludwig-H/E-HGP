"""Capture and exercise private copies of the FULL producer (audit only)."""
from __future__ import annotations

import argparse
import datetime as dt
import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parents[1]
BRIDGE = AUDIT / "full_producer_bridge.cpp"
HEADER = Path("morsehgp3D_v7/src/forest/full_gabriel.hpp")
MUTANTS = {
    "stale_terminal": (
        "if (!normalize(terminal->token, root)) return false;",
        "root = terminal->token;  // Private mutant: consume stale terminal.",
    ),
    "reset_support_budget": (
        "return geometry.miniball(sites, n, &ball) || geometry_failed();",
        "geometry_result.stats.meb_supports = 0;  // Private mutant.\n"
        "    return geometry.miniball(sites, n, &ball) || geometry_failed();",
    ),
    "omit_portal": (
        "if (!locate(f, level, prior_count, root)) return false;",
        "if (aliases.find(f) == aliases.end()) continue;  // Private mutant.\n"
        "        if (!locate(f, level, prior_count, root)) return false;",
    ),
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def write(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, indent=2) + "\n")


def command(argv: list[str], receipt: Path, prefix: str,
            stdin: Path | None = None, overrides: dict[str, str] | None = None) -> dict:
    start = now()
    result = subprocess.run(argv, cwd=ROOT, input=stdin.read_bytes() if stdin else None,
                            capture_output=True, env={**os.environ, **(overrides or {})})
    output = receipt / (prefix + ("_output.json" if stdin else "_stdout.txt"))
    error = receipt / (prefix + "_stderr.txt")
    output.write_bytes(result.stdout)
    error.write_bytes(result.stderr)
    return {"command": argv, "exit_code": result.returncode, "started_utc": start,
            "finished_utc": now(), "environment_override": overrides or {},
            "stdout": output.name, "stdout_sha256": sha(output),
            "stderr": error.name, "stderr_sha256": sha(error)}


def dependencies(path: Path) -> list[Path]:
    return [Path(p).resolve() for p in path.read_text().replace("\\\n", " ").split(":", 1)[1].split()]


def prepare(receipt: Path, work: Path) -> None:
    if (receipt / "source_pins.json").exists():
        raise RuntimeError("capture already exists; use a new receipt directory")
    argv = ["g++", "-std=c++20", "-MM", "-I", "morsehgp3D_v7", str(BRIDGE)]
    result = command(argv, receipt, "discovery")
    if result["exit_code"] != 0:
        raise RuntimeError("dependency discovery failed")
    capture = {}
    for path in dependencies(receipt / result["stdout"]):
        if path == BRIDGE:
            continue
        rel = path.relative_to(ROOT)
        if not str(rel).startswith("morsehgp3D_v7/src/"):
            raise RuntimeError(f"unexpected dependency: {rel}")
        raw = path.read_bytes()
        target = receipt / "source" / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
        if path.read_bytes() != raw:
            raise RuntimeError(f"source changed during capture: {rel}")
        capture[str(rel)] = sha(target)
    write(receipt / "source_pins.json", {
        "schema": "mhgp7-full-producer-source-capture-v1", "captured_utc": now(),
        "source_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=ROOT, text=True),
        "pins": capture, "discovery": result,
        "compiler": subprocess.check_output(["g++", "--version"], text=True),
        "system": subprocess.check_output(["uname", "-a"], text=True),
    })
    original = (receipt / "source" / HEADER).read_text()
    for name, (before, after) in MUTANTS.items():
        if original.count(before) != 1:
            raise RuntimeError(f"mutant target not unique: {name}")
        target_root = work / name / "source"
        shutil.copytree(receipt / "source", target_root)
        changed = original.replace(before, after)
        (target_root / HEADER).write_text(changed)
        (receipt / (name + ".patch.txt")).write_text("".join(difflib.unified_diff(
            original.splitlines(True), changed.splitlines(True),
            fromfile=str(HEADER), tofile=f"private/{name}/{HEADER}")))


def build(receipt: Path, work: Path, name: str) -> None:
    source = receipt / "source" if name in ("O2", "sanitized") else work / name / "source"
    dep = receipt / (name + "_dependencies.d.txt")
    binary = work / (name + ".bin")
    options = ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if name == "sanitized" else ["-O2"]
    argv = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            *options, "-MMD", "-MF", str(dep), "-I", str(source / "morsehgp3D_v7"),
            str(BRIDGE), "-o", str(binary)]
    result = command(argv, receipt, name + "_build")
    if result["exit_code"] == 0:
        result["binary_sha256"] = sha(binary)
        result["dependencies"] = [{"path": str(p.relative_to(ROOT)), "sha256": sha(p)}
                                  for p in dependencies(dep)]
    write(receipt / (name + "_build.json"), result)
    print(json.dumps({"name": name, "exit_code": result["exit_code"]}))
    if result["exit_code"] != 0:
        raise RuntimeError(f"build failed: {name}; inspect captured stderr")


def run(receipt: Path, work: Path, name: str) -> None:
    binary, fixture = work / (name + ".bin"), receipt / "fixtures.txt"
    overrides = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if name == "sanitized" else {}
    result = command([str(binary)], receipt, name, fixture, overrides)
    result.update(binary_sha256=sha(binary), input_sha256=sha(fixture))
    write(receipt / (name + "_run.json"), result)
    print(json.dumps({"name": name, "exit_code": result["exit_code"]}))


def judge_all(receipt: Path) -> None:
    import full_producer_audit as judge

    judge.OUT = receipt
    fixture = judge.make_fixtures()
    judge.require(json.loads((receipt / "fixtures.json").read_text()) == fixture,
                  "fixture.changed")
    judge.require((receipt / "fixtures.txt").read_text() == judge.fixture_text(fixture),
                  "fixture.text_changed")
    mode = "normal" if __debug__ else "optimized"
    outcomes = []
    for name in ("O2", "sanitized", *MUTANTS):
        path = receipt / (name + "_output.json")
        start = now()
        try:
            result = judge.check_output(path, fixture)
            code = 0
        except (ValueError, KeyError, TypeError) as error:
            result = {"status": "rejected", "reason": str(error),
                      "python_optimized": not __debug__, "output_sha256": sha(path),
                      "checker_sha256": sha(Path(judge.__file__))}
            code = 1
        write(receipt / (name + "_" + mode + ".json"), result)
        expected = 0 if name in ("O2", "sanitized") else 1
        outcomes.append({"name": name, "exit_code": code, "expected_exit_code": expected,
                         "started_utc": start, "finished_utc": now(),
                         "result": name + "_" + mode + ".json"})
        print(json.dumps({"name": name, "exit_code": code,
                          "reason": result.get("reason"), "counts": result.get("counts")}))
    write(receipt / ("judgments_" + mode + ".json"), {
        "schema": "mhgp7-full-producer-judgments-v1", "python_optimized": not __debug__,
        "outcomes": outcomes, "checker_sha256": sha(Path(judge.__file__)),
        "runner_sha256": sha(Path(__file__)),
        "command": ["python3", "-B", *([] if __debug__ else ["-O"]),
                    str(Path(__file__).relative_to(ROOT)), "judge", "--receipt", str(receipt)]})
    if any(row["exit_code"] != row["expected_exit_code"] for row in outcomes):
        raise RuntimeError("unexpected judgment; inspect all retained results")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "build", "run", "judge"))
    parser.add_argument("--name", choices=("O2", "sanitized", *MUTANTS), default="O2")
    parser.add_argument("--receipt", type=Path, default=AUDIT / "receipts_full_producer_20260905")
    parser.add_argument("--work", type=Path, default=AUDIT / ".work_full_producer_20260905")
    args = parser.parse_args()
    receipt, work = args.receipt.resolve(), args.work.resolve()
    for directory in (receipt, work):
        directory.relative_to(AUDIT)
        directory.mkdir(parents=True, exist_ok=True)
    if args.action == "prepare":
        prepare(receipt, work)
    elif args.action == "build":
        build(receipt, work, args.name)
    elif args.action == "run":
        run(receipt, work, args.name)
    else:
        judge_all(receipt)


if __name__ == "__main__":
    main()
