#!/usr/bin/env python3
"""Retain runner selftests and two tiny real-wire smokes; no SLO measurement."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import signal
import sys


OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]


def module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError("module unavailable")
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


H = module("receipt_helpers", OUT.with_name("release_20260904") / "run_release.py")
H.OUT = OUT
SCRIPT = ROOT / "morsehgp3D_v7/bench/compare_v6_v7.py"
GATE = ROOT / "morsehgp3D_v7/tests/compare_campaign_gate.py"
BINARY = ROOT / "build/v7_mono_baseline/mhgp7_B"


def main() -> int:
    before = {str(p.relative_to(ROOT)): H.digest(p) for p in (SCRIPT, GATE, BINARY)}
    expected = json.loads((OUT.with_name("release_delta2_20260904") / "summary.json").read_text())["product_b_sha256"]
    if before[str(BINARY.relative_to(ROOT))] != expected:
        raise RuntimeError("baseline B differs from sealed B2 product")
    H.save("pins_before.json", before)
    H.save("scope.json", {"public_status": "not_claimed", "gcp": "not_used",
           "scope": "runner_negative_tests_and_two_real_wire_smokes_not_performance",
           "binary_source_authority": "release_delta2_20260904, not the current changing C sources",
           "release_b2_manifest_sha256": H.digest(OUT.with_name("release_delta2_20260904") / "receipt_manifest.json")})
    results, parsed = {}, {}
    failure = None
    try:
        for optimized in (False, True):
            name = "gate_optimized" if optimized else "gate"
            results[name] = H.run(name, [sys.executable, "-B", *(["-O"] if optimized else []), str(GATE)], 120)
            if results[name]["exit_code"]:
                raise RuntimeError(name + " failed")
        runner = module("paired_runner_smoke", SCRIPT)
        for kmax, separation in ((5, 10), (10, 12)):
            name = f"wire_b_k{kmax}_s{separation}"
            usage = OUT / (name + ".usage")
            command = [str(BINARY), "--family=uniform", "--n=200", "--seed=3", "--threads=1",
                       f"--s={separation}", f"--smax={kmax + 1}", "--layout=csr",
                       "--fold-inflight=1", "--fold-join=1", "--digest"]
            results[name] = H.run(name, ["/usr/bin/time", "-v", "-o", str(usage), "--", *command], 30)
            if results[name]["exit_code"]:
                raise RuntimeError(name + " failed")
            parsed[name] = runner.parse_success((OUT / (name + ".stdout")).read_text(),
                                                (OUT / (name + ".stderr")).read_text(), usage.read_text(),
                                                label="v7", family="uniform", n=200, seed=3, threads=1,
                                                wall_seconds=results[name]["elapsed_seconds"], kmax=kmax,
                                                s=separation, serial_stages_requested=True)
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    finally:
        after = {str(p.relative_to(ROOT)): H.digest(p) for p in (SCRIPT, GATE, BINARY)}
        H.save("pins_after.json", after)
        H.save("parsed_smokes.json", parsed)
        passed = failure is None and before == after and len(results) == 4 and len(parsed) == 2
        summary = {"status": "passed" if passed else "failed_or_invalid", "failure": failure,
                   "pins_stable": before == after, "public_status": "not_claimed", "gcp": "not_used",
                   "results": results}
        H.save("summary.json", summary)
        H.save("receipt_manifest.json", [{"path": p.name, "sha256": H.digest(p), "size": p.stat().st_size}
               for p in sorted(OUT.iterdir()) if p.is_file() and p.name != "receipt_manifest.json"])
        print(json.dumps({k: v for k, v in summary.items() if k != "results"}), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(main())
