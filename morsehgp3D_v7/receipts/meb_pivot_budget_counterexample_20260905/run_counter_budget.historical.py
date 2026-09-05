#!/usr/bin/env python3
"""Create-only bounded diagnostic runner; no product edits or cloud work."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time
from datetime import datetime, timezone


ROOT = Path(__file__).resolve().parents[2]
DESIGN = Path(__file__).resolve().parent
OUTPUT = DESIGN / "counter_budget_run_20260905"
PROTOTYPE = ROOT / "build/v7_meb_pivot_prototype/pivot.hpp"
SOURCE = DESIGN / "counter_budget.cpp"
EXPECTED_PROTOTYPE = "d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5"
EXPECTED_D = "5214a9a7f2b6f53b1c59c803d414e109c9a660f15ab9448d88aec90300160c71"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory() -> dict[str, str]:
    paths = sorted((ROOT / "morsehgp3D_v7/src").rglob("*.hpp"))
    paths += [PROTOTYPE, SOURCE, Path(__file__).resolve()]
    return {str(path.relative_to(ROOT)): sha(path) for path in paths}


def save(name: str, value: dict) -> None:
    with (OUTPUT / name).open("x", encoding="utf-8") as handle:
        json.dump(value, handle, sort_keys=True, indent=2)
        handle.write("\n")


def main() -> int:
    OUTPUT.mkdir(exist_ok=False)
    start = datetime.now(timezone.utc).isoformat()
    deadline = time.monotonic() + 30.0
    before = inventory()
    records: list[dict] = []
    error = None
    compiler = shutil.which("c++")
    binary = OUTPUT / "counter_budget"
    try:
        if sha(PROTOTYPE) != EXPECTED_PROTOTYPE:
            raise RuntimeError("prototype_pin_mismatch")
        if sha(ROOT / "morsehgp3D_v7/src/forest/silent_incidence.hpp") != EXPECTED_D:
            raise RuntimeError("reference_d_pin_mismatch")
        if not compiler:
            raise RuntimeError("compiler_missing")
        if any(os.environ.get(name) for name in ("LD_PRELOAD", "LD_AUDIT", "LD_LIBRARY_PATH")):
            raise RuntimeError("dynamic_loader_override_present")
        commands = [
            ("compiler", [compiler, "--version"]),
            ("compile", [compiler, "-std=c++20", "-O0", "-Wall", "-Wextra",
                         "-Wpedantic", "-Werror", "-pthread",
                         "-I", str(ROOT / "morsehgp3D_v7"),
                         "-MMD", "-MF", str(OUTPUT / "dependencies.d"),
                         str(SOURCE), "-o", str(binary)]),
            ("execute", [str(binary)]),
        ]
        for name, argv in commands:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise RuntimeError("combined_30s_deadline_exhausted")
            command_start = time.monotonic()
            timed_out = False
            with (OUTPUT / f"{name}.stdout").open("xb") as stdout:
                with (OUTPUT / f"{name}.stderr").open("xb") as stderr:
                    process = subprocess.Popen(argv, cwd=ROOT, stdout=stdout, stderr=stderr,
                                               start_new_session=True)
                    try:
                        returncode = process.wait(timeout=remaining)
                    except subprocess.TimeoutExpired:
                        timed_out = True
                        os.killpg(process.pid, signal.SIGKILL)
                        returncode = process.wait()
            records.append({"name": name, "argv": argv, "returncode": returncode,
                            "timed_out": timed_out,
                            "elapsed_seconds": time.monotonic() - command_start,
                            "stdout_sha256": sha(OUTPUT / f"{name}.stdout"),
                            "stderr_sha256": sha(OUTPUT / f"{name}.stderr")})
            if timed_out:
                raise RuntimeError(f"{name}_combined_deadline_exhausted")
            if returncode != 0:
                raise RuntimeError(f"{name}_nonzero_exit")
        expected = ("counter_budget=counterexample_confirmed cases=2 "
                    "proposition_candidates=5 reference_ordinal=4 public_status=not_claimed\n")
        if not (OUTPUT / "execute.stdout").read_text().endswith(expected):
            raise RuntimeError("missing_counterexample_marker")
    except (RuntimeError, OSError, subprocess.TimeoutExpired) as exception:
        error = type(exception).__name__ + ": " + str(exception)
    after = inventory()
    stable = before == after
    if not stable:
        error = "source_pins_changed_during_run"
    artifacts = {path.name: {"bytes": path.stat().st_size, "sha256": sha(path)}
                 for path in sorted(OUTPUT.iterdir()) if path.is_file()}
    receipt = {
        "status": "counterexample_confirmed" if error is None else "failed",
        "error": error,
        "started_at": start,
        "finished_at": datetime.now(timezone.utc).isoformat(),
        "combined_deadline_seconds": 30,
        "commands": records,
        "source_before": before,
        "source_after": after,
        "sources_stable": stable,
        "compiler_path": compiler,
        "compiler_sha256": sha(Path(compiler)) if compiler else None,
        "artifacts": artifacts,
        "scope": "bounded_counterfixture_not_benchmark_not_product_qualification",
        "testing_macro": False,
        "gcp_used": False,
        "public_status": "not_claimed",
    }
    save("receipt.json", receipt)
    print(json.dumps({"status": receipt["status"], "error": error,
                      "receipt": str(OUTPUT / "receipt.json"),
                      "receipt_sha256": sha(OUTPUT / "receipt.json")}, sort_keys=True))
    return 0 if error is None else 1


if __name__ == "__main__":
    raise SystemExit(main())
