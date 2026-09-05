"""Build and receipt the bounded index audit; writes only below audits/."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORK = HERE / ".work_20260905_index"
RECEIPTS = HERE / "receipts_20260905"
SOURCE = HERE / "index_probe_20260905.cpp"
SOURCES = [
    SOURCE,
    Path(__file__).resolve(),
    ROOT / "morsehgp3D_v7/src/core/types.hpp",
    ROOT / "morsehgp3D_v7/src/core/morton.hpp",
    ROOT / "morsehgp3D_v7/src/core/caps.hpp",
    ROOT / "morsehgp3D_v7/src/tree/cloud_index.hpp",
    ROOT / "morsehgp3D_v7/src/pipeline/run.hpp",
]
MUTANTS = [
    "alias-child",
    "parent-link",
    "omit-leaf",
    "tight-box",
    "morton-key",
    "identity-swap",
    "csr-boundary",
]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sources() -> dict[str, str]:
    return {str(path.relative_to(ROOT)): digest(path) for path in SOURCES}


def git(*arguments: str) -> str:
    return subprocess.check_output(["git", *arguments], cwd=ROOT, text=True).strip()


def main() -> int:
    WORK.mkdir(exist_ok=True)
    RECEIPTS.mkdir(exist_ok=True)
    report: dict[str, object] = {
        "scope": "bounded_independent_index_audit",
        "phase": "exploration_v7_hors_registre",
        "backend": "cpu_reference",
        "profile": "quantized_u16_input_only",
        "mode": "audit_independant_math_and_architecture",
        "public_status": "not_claimed",
        "gcp": "not_used",
        "head_before": git("rev-parse", "HEAD"),
        "worktree_before": git("status", "--short"),
        "sources_before": sources(),
        "python": sys.version,
        "platform": platform.platform(),
        "commands": [],
        "binaries": {},
    }
    commands: list[dict[str, object]] = []
    binaries: dict[str, object] = {}
    report["commands"] = commands
    report["binaries"] = binaries

    def run(name: str, command: list[str], expected: int = 0) -> None:
        start = time.monotonic()
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
        stdout = RECEIPTS / f"index_{name}.stdout"
        stderr = RECEIPTS / f"index_{name}.stderr"
        stdout.write_text(result.stdout, encoding="utf-8")
        stderr.write_text(result.stderr, encoding="utf-8")
        commands.append({
            "name": name,
            "argv": command,
            "cwd": str(ROOT),
            "exit_code": result.returncode,
            "expected_exit_code": expected,
            "seconds": time.monotonic() - start,
            "stdout": str(stdout.relative_to(HERE)),
            "stdout_sha256": digest(stdout),
            "stderr": str(stderr.relative_to(HERE)),
            "stderr_sha256": digest(stderr),
        })
        if result.returncode != expected:
            raise RuntimeError(f"{name}: got {result.returncode}, expected {expected}: {result.stderr}")

    try:
        compiler = os.environ.get("CXX", "g++")
        run("compiler", [compiler, "--version"])
        for name, options in [
            ("optimized", ["-O2"]),
            ("ubsan", ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all", "-D_GLIBCXX_ASSERTIONS"]),
        ]:
            binary = WORK / f"index_{name}"
            run(f"build_{name}", [compiler, "-std=c++20", *options, "-Wall", "-Wextra",
                                   "-Wpedantic", "-Werror", str(SOURCE), "-o", str(binary)])
            binaries[name] = {"path": str(binary.relative_to(HERE)), "sha256": digest(binary)}
            run(name, [str(binary)])
            for mutant in MUTANTS:
                run(f"{name}_{mutant}", [str(binary), "--mutant", mutant], expected=3)
        report["sources_after"] = sources()
        if report["sources_before"] != report["sources_after"]:
            raise RuntimeError("audited sources changed during execution")
        report["status"] = "completed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
    report["head_after"] = git("rev-parse", "HEAD")
    report["worktree_after"] = git("status", "--short")
    (RECEIPTS / "index_summary.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(f"index audit status={report['status']} commands={len(commands)}")
    return 0 if report["status"] == "completed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
