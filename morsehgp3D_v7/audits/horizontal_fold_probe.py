"""Receipt the bounded set-oracle audit of the normalized C++ horizontal fold."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import time


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CPP = HERE / "horizontal_fold_probe.cpp"
WORK = HERE / ".work_horizontal_fold"
OUTPUT = HERE / "receipts_horizontal_20260905/fold"
REFERENCE = "61f72a6805e27f1bc216b5d7444164b31fc970b6"
MUTANTS = {
    "binary-ties": "batch_count",
    "repr-ties": "batch_count",
    "reduced-latent-parent": "delta.parents",
    "reduced-drop-materialization": "delta_count",
    "drop-nonmerge": "delta_count",
    "csr-keep-continuation": "delta_count",
    "csr-stale-output": "delta.output",
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def quoted_closure(path: Path, found: set[Path]) -> None:
    """Seal local includes before compilation; verify actual .d dependencies later."""
    path = path.resolve()
    if path in found:
        return
    found.add(path)
    for target in re.findall(r'^\s*#\s*include\s+"([^"]+)"', path.read_text(), re.MULTILINE):
        included = (path.parent / target).resolve()
        if not included.is_relative_to(ROOT):
            raise RuntimeError(f"include outside repository: {included}")
        quoted_closure(included, found)


def git(*arguments: str) -> str:
    return subprocess.check_output(["git", *arguments], cwd=ROOT, text=True).strip()


def main() -> int:
    WORK.mkdir(exist_ok=True)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    paths: set[Path] = {Path(__file__).resolve()}
    quoted_closure(CPP, paths)

    def sources() -> dict[str, str]:
        return {str(path.relative_to(ROOT)): digest(path) for path in sorted(paths)}

    commands: list[dict[str, object]] = []
    report: dict[str, object] = {
        "status": "running",
        "scope": "bounded_normalized_fold_set_hypergraph_and_token_reader",
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
        "normal_counts": {},
        "limits": [
            "Synthetic structurally valid event streams; no Euclidean realization claimed.",
            "Exact fixture rationals fit u64; reference comparison uses independent u128 cross products.",
            "K1 roots cover vertices appearing in events; isolated input vertices are caller responsibility.",
            "Deltas within a batch compared as semantic groups, not raw UF-root storage order.",
            "Sequential fold with threads=1; this receipt is not a concurrent qualification.",
            "No product benchmark, universal geometry proof or global exactness promotion.",
        ],
    }

    def run(name: str, argv: list[str], expected: int = 0) -> str:
        start = time.monotonic()
        result = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, check=False)
        out, err = OUTPUT / f"{name}.stdout", OUTPUT / f"{name}.stderr"
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
        return result.stdout

    try:
        reference_hashes = {}
        for path in paths:
            relative = str(path.relative_to(ROOT))
            if relative.startswith("morsehgp3D_v7/src/"):
                raw = subprocess.check_output(["git", "show", f"{REFERENCE}:{relative}"], cwd=ROOT)
                reference_hashes[relative] = hashlib.sha256(raw).hexdigest()
        report["product_reference"] = {"commit": REFERENCE, "sha256": reference_hashes}
        if not reference_hashes or any(report["sources_before"][name] != value
                                       for name, value in reference_hashes.items()):
            raise RuntimeError("a consumed product source differs from reference E")
        compiler = os.environ.get("CXX", "g++")
        run("compiler", [compiler, "--version"])
        for name, options in [
            ("optimized", ["-O2"]),
            ("ubsan", ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                       "-D_GLIBCXX_ASSERTIONS"]),
        ]:
            binary, dependencies = WORK / f"fold_{name}", WORK / f"fold_{name}.d"
            run(f"build_{name}", [compiler, "-std=c++20", *options, "-Wall", "-Wextra",
                                  "-Wpedantic", "-Werror", "-DMHGP7_TESTING", "-pthread",
                                  "-MMD", "-MF", str(dependencies), str(CPP), "-o", str(binary)])
            dep_text = dependencies.read_text(encoding="utf-8")
            actual = {Path(path).resolve() for path in dep_text.replace("\\\n", " ").split(":", 1)[1].split()}
            if actual != paths - {Path(__file__).resolve()}:
                raise RuntimeError(f"compilation dependency mismatch: {actual.symmetric_difference(paths - {Path(__file__).resolve()})}")
            dep_output = OUTPUT / f"{name}.dependencies"
            dep_output.write_text(dep_text, encoding="utf-8")
            report["binaries"][name] = {
                "path": str(binary.relative_to(HERE)), "sha256": digest(binary),
                "dependencies": dep_output.name, "dependencies_sha256": digest(dep_output),
            }
            run(f"ldd_{name}", ["ldd", str(binary)])
            stdout = run(name, [str(binary)])
            counts = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", stdout)}
            required = {"streams": 40, "strict_closed_cuts": 250, "deltas": 100, "births": 20,
                        "growths": 20, "merges": 20, "omitted_continuations": 20,
                        "no_new_point_growths": 15, "latent_contacts": 100,
                        "k1_initial_roots": 32, "reindexed_cuts": 60, "invalid_inputs": 8}
            if set(counts) != set(required) or any(counts[key] < floor for key, floor in required.items()):
                raise RuntimeError(f"{name}: nonvacuity or count schema failed: {counts}")
            report["normal_counts"][name] = counts
            for mutant, reason in MUTANTS.items():
                stdout = run(f"{name}_{mutant}", [str(binary), "--mutant", mutant], expected=4)
                if not re.fullmatch(rf"killed mutant={re.escape(mutant)} reason={re.escape(reason)} completed_streams=\d+\n", stdout):
                    raise RuntimeError(f"{name}: mutation was not rejected by its targeted gate: {stdout}")
            run(f"{name}_invalid_cli", [str(binary), "--mutant", "unregistered-audit-fault"], expected=2)
        if report["normal_counts"]["optimized"] != report["normal_counts"]["ubsan"]:
            raise RuntimeError("builds disagree on observed nonvacuity counts")
        report["sources_after"] = sources()
        if report["sources_before"] != report["sources_after"]:
            raise RuntimeError("consumed sources changed during the audit")
        report["status"] = "completed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
        report["sources_after"] = sources()
    report["head_after"] = git("rev-parse", "HEAD")
    report["worktree_after"] = git("status", "--short")
    report["utc_end"] = datetime.now(timezone.utc).isoformat()
    (OUTPUT / "summary.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"horizontal fold status={report['status']} commands={len(commands)}")
    if "error" in report:
        print(report["error"])
    return 0 if report["status"] == "completed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
