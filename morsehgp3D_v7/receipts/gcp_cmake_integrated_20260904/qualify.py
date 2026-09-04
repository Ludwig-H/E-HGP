#!/usr/bin/env python3
"""Record only local integrated-controller tests; no network or cloud calls."""
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
    names = ("v7_g4_session.py", "private_cmake_v7.py", "selftest_session_v7.py",
             "selftest_private_cmake_v7.py", "selftest_private_cmake_controller_v7.py",
             "create_v7_g4.py", "selftest_create_v7.py", "README_v7.md")
    paths = [ROOT / "gcp-migration" / name for name in names]
    paths += [ROOT / "build/v7/mhgp7", ROOT / "build/v7_mono_baseline/mhgp7_B"]
    pins = lambda: {str(path.relative_to(ROOT)): H.digest(path) for path in paths}
    before = pins()
    H.save("pins_before.json", before)
    results, failure = {}, None
    try:
        for script in ("selftest_session_v7.py", "selftest_private_cmake_v7.py",
                       "selftest_private_cmake_controller_v7.py", "selftest_create_v7.py"):
            for optimized in (False, True):
                name = script.removesuffix(".py") + ("_optimized" if optimized else "")
                results[name] = H.run(name, [sys.executable, "-B", *(["-O"] if optimized else []),
                                            str(ROOT / "gcp-migration" / script)], 45)
                if results[name]["exit_code"]:
                    raise RuntimeError(name + " failed")
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    after = pins()
    H.save("pins_after.json", after)
    passed = failure is None and before == after and len(results) == 8
    summary = {"status": "passed" if passed else "failed_or_invalid", "failure": failure,
               "pins_stable": before == after, "public_status": "not_claimed", "gcp": "not_used",
               "scope": "integrated_tooling_controller_mock_tests_not_gpu_engine", "results": results,
               "official_wheel_smoke": {"receipt": "../gcp_cmake_overlay_20260904/receipt_manifest.json",
                                         "sha256": H.digest(OUT.with_name("gcp_cmake_overlay_20260904") / "receipt_manifest.json")}}
    H.save("summary.json", summary)
    H.save("receipt_manifest.json", [{"path": path.name, "sha256": H.digest(path), "size": path.stat().st_size}
                                    for path in sorted(OUT.iterdir()) if path.is_file() and path.name != "receipt_manifest.json"])
    print(json.dumps({key: value for key, value in summary.items() if key != "results"}), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
