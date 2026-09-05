"""Receipt a bounded compiled spindle audit; all writes stay within audits/."""

from __future__ import annotations

from datetime import datetime, timezone
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
WORK = HERE / ".work_spindle_compiled"
OUTPUT = HERE / "receipts_front_compiled_20260905/spindle"
CPP = HERE / "spindle_compiled_probe.cpp"
STATIC_LEDGER = HERE / "receipts_front_20260905/spindle_bounds.json"
SOURCE_PATHS = [
    CPP, Path(__file__).resolve(),
    *[ROOT / f"morsehgp3D_v7/src/{name}" for name in [
        "spindle/spindle.hpp", "spindle/witness_count.hpp", "core/intmath.hpp",
        "core/types.hpp", "core/device.hpp", "core/mutants.hpp", "core/morton.hpp",
        "tree/cloud_index.hpp",
    ]],
]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sources() -> dict[str, str]:
    return {str(path.relative_to(ROOT)): digest(path) for path in SOURCE_PATHS}


def git(*arguments: str) -> str:
    return subprocess.check_output(["git", *arguments], cwd=ROOT, text=True).strip()


def main() -> int:
    WORK.mkdir(exist_ok=True)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    commands: list[dict[str, object]] = []
    report: dict[str, object] = {
        "status": "running",
        "scope": "bounded_actual_cpp_spindle_helpers",
        "phase": "exploration_v7_hors_registre",
        "backend": "cpu_reference",
        "profile": "quantized_u16_input_only",
        "mode": "audit_independant_math_and_architecture",
        "public_status": "not_claimed",
        "gcp": "not_used",
        "utc_start": datetime.now(timezone.utc).isoformat(),
        "head_before": git("rev-parse", "HEAD"),
        "worktree_before": git("status", "--short"),
        "sources_before": sources(),
        "commands": commands,
        "platform": platform.platform(),
        "python": sys.version,
        "binaries": {},
    }

    def run(name: str, argv: list[str], expected: int = 0) -> None:
        start = time.monotonic()
        result = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, check=False)
        out = OUTPUT / f"{name}.stdout"
        err = OUTPUT / f"{name}.stderr"
        out.write_text(result.stdout, encoding="utf-8")
        err.write_text(result.stderr, encoding="utf-8")
        commands.append({
            "name": name, "argv": argv, "cwd": str(ROOT),
            "exit_code": result.returncode, "expected_exit_code": expected,
            "seconds": time.monotonic() - start,
            "stdout": out.name, "stdout_sha256": digest(out),
            "stderr": err.name, "stderr_sha256": digest(err),
        })
        if result.returncode != expected:
            raise RuntimeError(f"{name}: got {result.returncode}, expected {expected}: {result.stderr}")

    try:
        previous = json.loads(STATIC_LEDGER.read_text(encoding="utf-8"))["sources_after"]
        bindings = {path: value == previous[path] for path, value in report["sources_before"].items()
                    if path in previous}
        report["static_ledger"] = {
            "path": str(STATIC_LEDGER.relative_to(HERE)),
            "sha256": digest(STATIC_LEDGER),
            "same_shared_sources": bindings,
        }
        if not bindings or not all(bindings.values()):
            raise RuntimeError("a source shared with the static spindle ledger changed")
        compiler = os.environ.get("CXX", "g++")
        run("compiler", [compiler, "--version"])
        for name, options in [
            ("optimized", ["-O2"]),
            ("ubsan", ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all", "-D_GLIBCXX_ASSERTIONS"]),
        ]:
            binary = WORK / f"spindle_{name}"
            dependencies = WORK / f"spindle_{name}.d"
            run(f"build_{name}", [compiler, "-std=c++20", *options, "-Wall", "-Wextra",
                                  "-Wpedantic", "-Werror", "-DMHGP7_TESTING", "-MMD", "-MF",
                                  str(dependencies), str(CPP), "-o", str(binary)])
            dep_text = dependencies.read_text(encoding="utf-8")
            actual_headers = dep_text.replace("\\\n", " ").split(":", 1)[1].split()
            expected_paths = {path.resolve() for path in SOURCE_PATHS}
            unknown = [path for path in actual_headers if Path(path).resolve() not in expected_paths]
            if unknown:
                raise RuntimeError(f"unsealed compilation dependency: {unknown}")
            (OUTPUT / f"{name}.dependencies").write_text(dep_text, encoding="utf-8")
            report["binaries"][name] = {"path": str(binary.relative_to(HERE)), "sha256": digest(binary)}
            run(f"ldd_{name}", ["ldd", str(binary)])
            run(name, [str(binary)])
            for mutant in ["core-ball-ceil-distance", "witness-no-lane-mask"]:
                run(f"{name}_{mutant}", [str(binary), "--mutant", mutant], expected=4)
        report["sources_after"] = sources()
        if report["sources_before"] != report["sources_after"]:
            raise RuntimeError("compiled sources changed during the audit")
        report["status"] = "completed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
        report["sources_after"] = sources()
    report["head_after"] = git("rev-parse", "HEAD")
    report["worktree_after"] = git("status", "--short")
    report["utc_end"] = datetime.now(timezone.utc).isoformat()
    (OUTPUT / "summary.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"spindle compiled status={report['status']} commands={len(commands)}")
    if "error" in report:
        print(report["error"])
    return 0 if report["status"] == "completed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
