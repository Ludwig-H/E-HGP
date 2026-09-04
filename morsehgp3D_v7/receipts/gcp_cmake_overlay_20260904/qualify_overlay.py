#!/usr/bin/env python3
"""Qualify the overlay, including the real pinned wheel in private build storage."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import signal
import sys
import tempfile
import time


OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None: raise RuntimeError("module unavailable")
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


H = module("overlay_receipt", OUT.with_name("release_20260904") / "run_release.py")
H.OUT = OUT
M = module("private_cmake_real_smoke", OUT / "private_cmake_v7.py")


def main() -> int:
    paths = [OUT / name for name in ("private_cmake_v7.py", "controller.overlay.py", "controller.patch",
                                     "selftest.py", "selftest_controller.py")]
    paths.append(ROOT / "gcp-migration/v7_g4_session.py")
    before = {str(path.relative_to(ROOT)): H.digest(path) for path in paths}
    H.save("pins_before.json", before)
    H.save("scope.json", {"public_status": "not_claimed", "gcp": "not_used", "os_installation": False,
           "scope": "tooling_overlay_and_pinned_real_wheel_smoke_not_gpu_engine",
           "source_json": M.SOURCE_JSON, "wheel_sha256": M.WHEEL_SHA256, "wheel_bytes": M.WHEEL_SIZE})
    results = {}
    smoke_root = None
    failure = None
    try:
        for script in ("selftest.py", "selftest_controller.py"):
            for optimized in (False, True):
                name = script.removesuffix(".py") + ("_optimized" if optimized else "")
                results[name] = H.run(name, [sys.executable, "-B", *(["-O"] if optimized else []), str(OUT / script)], 30)
                if results[name]["exit_code"]: raise RuntimeError(name + " failed")
        smoke_root = Path(tempfile.mkdtemp(prefix="v7-cmake-wheel-smoke-", dir=ROOT / "build"))
        smoke_root.chmod(0o700)
        deadline = time.time() + M.INSTALL_BUDGET
        H.save("private_smoke_location.json", {"path": str(smoke_root), "mode": "0700", "retained_under_build": True})
        command = [sys.executable, "-B", str(OUT / "private_cmake_v7.py"), "--install", "--root", str(smoke_root),
                   "--seconds", str(M.INSTALL_BUDGET), "--deadline-epoch", str(deadline)]
        results["real_wheel_smoke"] = H.run("real_wheel_smoke", command, M.INSTALL_BUDGET)
        if results["real_wheel_smoke"]["exit_code"]: raise RuntimeError("real pinned wheel smoke failed")
        receipt = json.loads((OUT / "real_wheel_smoke.stdout").read_text())
        M.validate_receipt(receipt)
        for name, relative in M.BINARIES.items():
            path = smoke_root / relative
            if H.digest(path) != receipt["probes"][name]["binary_sha256"] or path.stat().st_mode & 0o777 != 0o555:
                raise RuntimeError("native smoke binary hash/mode mismatch")
        H.save("verified_real_tooling.json", receipt)
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    finally:
        after = {str(path.relative_to(ROOT)): H.digest(path) for path in paths}
        H.save("pins_after.json", after)
        passed = failure is None and before == after and len(results) == 5
        summary = {"status": "passed" if passed else "failed_or_invalid", "failure": failure,
                   "pins_stable": before == after, "public_status": "not_claimed", "gcp": "not_used",
                   "private_smoke_directory": str(smoke_root) if smoke_root else None, "results": results}
        H.save("summary.json", summary)
        H.save("receipt_manifest.json", [{"path": p.name, "sha256": H.digest(p), "size": p.stat().st_size}
               for p in sorted(OUT.iterdir()) if p.is_file() and p.name != "receipt_manifest.json"])
        print(json.dumps({key: value for key, value in summary.items() if key != "results"}), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(main())
