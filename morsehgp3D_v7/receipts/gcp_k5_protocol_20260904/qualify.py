#!/usr/bin/env python3
"""Local bounded protocol qualification, synthetic workloads only, no cloud."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import sys

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]
spec = importlib.util.spec_from_file_location("receipt_tools", OUT.with_name("release_20260904") / "run_release.py")
if spec is None or spec.loader is None:
    raise RuntimeError("receipt helper unavailable")
H = importlib.util.module_from_spec(spec)
spec.loader.exec_module(H)
H.OUT = OUT


def main() -> int:
    if (OUT / "summary.json").exists():
        raise RuntimeError("receipt already sealed")
    scripts = ("selftest_session_v7.py", "selftest_private_cmake_v7.py",
               "selftest_private_cmake_controller_v7.py", "selftest_create_v7.py", "selftest_cpu_towers_v7.py")
    paths = [ROOT / "gcp-migration" / name for name in (*scripts, "v7_g4_session.py", "private_cmake_v7.py",
              "create_v7_g4.py", "README_v7.md", "session_campagne_v7_g4.sh", "start_and_verify.sh", "stop_and_verify.sh")]
    paths += [ROOT / path for path in ("build/v7/mhgp7", "build/v7_mono_baseline/mhgp7_B",
              "morsehgp3D_v7/bench/compare_v6_v7.py", "morsehgp3D_v7/bench/incidence_campaign.py",
              "morsehgp3D_v7/tests/compare_campaign_gate.py", "morsehgp3D_v7/tests/incidence_campaign_gate.py")]
    pins = lambda: {str(path.relative_to(ROOT)): H.digest(path) for path in paths}
    before = pins()
    H.save("pins_before.json", before)
    results, failure = {}, None
    try:
        for script in scripts:
            for optimized in (False, True):
                name = script.removesuffix(".py") + ("_optimized" if optimized else "")
                results[name] = H.run(name, [sys.executable, "-B", *(["-O"] if optimized else []),
                                            str(ROOT / "gcp-migration" / script)], 45)
                if results[name]["exit_code"]:
                    raise RuntimeError(name + " failed")
        for name, command in (("wrapper_syntax", ["bash", "-n", "gcp-migration/session_campagne_v7_g4.sh"]),
                              ("diff_check", ["git", "diff", "--check", "--", "gcp-migration"]),
                              ("review_diff", ["git", "diff", "--", "gcp-migration/v7_g4_session.py",
                                                "gcp-migration/selftest_session_v7.py", "gcp-migration/README_v7.md"])):
            results[name] = H.run(name, command, 30)
            if results[name]["exit_code"]:
                raise RuntimeError(name + " failed")
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    after = pins()
    H.save("pins_after.json", after)
    passed = failure is None and before == after and len(results) == 13
    summary = {"status": "passed" if passed else "failed_or_invalid", "failure": failure,
               "pins_stable": before == after, "public_status": "not_claimed", "gcp": "not_used",
               "scope": "bounded_k5_protocol_real_parsers_synthetic_workloads_not_50k_measurement", "results": results,
               "scientific_semantics": "verified_events_only_not_exact_hgp",
               "previous_tooling_receipt": {"path": "../gcp_cmake_integrated_20260904/receipt_manifest.json",
                   "sha256": H.digest(OUT.with_name("gcp_cmake_integrated_20260904") / "receipt_manifest.json")}}
    H.save("summary.json", summary)
    H.save("receipt_manifest.json", [{"path": path.name, "sha256": H.digest(path), "size": path.stat().st_size}
                                    for path in sorted(OUT.iterdir()) if path.is_file() and path.name != "receipt_manifest.json"])
    print(json.dumps({key: value for key, value in summary.items() if key != "results"}), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
