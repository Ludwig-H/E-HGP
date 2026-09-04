#!/usr/bin/env python3
"""Build and pin candidate C before the paired mono diagnostic, create-only."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import time

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]
BUILD = ROOT / "build/v7"
B = ROOT / "build/v7_mono_baseline/mhgp7_B"
C = BUILD / "mhgp7"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name: str, value: object) -> None:
    with (OUT / name).open("x") as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")


def main() -> int:
    spec = importlib.util.spec_from_file_location("paired", ROOT / "morsehgp3D_v7/bench/compare_v6_v7.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    before = module.source_snapshot(ROOT)
    baseline = digest(B)
    save("before.json", {"source_sha256": before, "baseline_binary": baseline,
         "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
         "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=ROOT, text=True),
         "compiler": subprocess.check_output(["c++", "--version"], text=True),
         "cmake": subprocess.check_output(["cmake", "--version"], text=True),
         "runner_sha256": digest(Path(__file__)), "public_status": "not_claimed"})
    results = []
    failure = None
    try:
        for name, command, limit in (
            ("configure", ["cmake", "-S", "morsehgp3D_v7", "-B", str(BUILD),
                           "-DCMAKE_BUILD_TYPE=Release", "-DMHGP7_ENABLE_CUDA=OFF"], 120),
            ("build", ["cmake", "--build", str(BUILD), "--parallel", "1", "--target", "mhgp7"], 600),
        ):
            started = time.monotonic()
            record = {"name": name, "argv": command, "timeout_seconds": limit,
                      "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}
            print("START " + name, flush=True)
            with (OUT / (name + ".stdout")).open("xb") as stdout, (OUT / (name + ".stderr")).open("xb") as stderr:
                process = subprocess.Popen(command, cwd=ROOT, stdout=stdout, stderr=stderr,
                                           start_new_session=True, env=dict(os.environ, LC_ALL="C"))
                try:
                    record["exit_code"] = process.wait(timeout=limit)
                finally:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    process.wait()
            record["elapsed_seconds"] = time.monotonic() - started
            results.append(record)
            save(name + ".json", record)
            print(f"END {name} rc={record['exit_code']}", flush=True)
            if record["exit_code"] != 0:
                raise RuntimeError(name + " failed")
        save("build_configuration.json", {str(path.relative_to(ROOT)): {
            "sha256": digest(path), "text": path.read_text()}
            for path in (BUILD / "CMakeFiles/mhgp7.dir/flags.make", BUILD / "CMakeFiles/mhgp7.dir/link.txt")})
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    after = module.source_snapshot(ROOT)
    candidate = digest(C) if C.is_file() else None
    summary = {"status": "built_not_release_qualified" if not failure and len(results) == 2
               and before == after and baseline == digest(B) and candidate != baseline else "invalid_or_failed",
               "failure": failure, "source_stable": before == after, "baseline_unchanged": baseline == digest(B),
               "baseline_binary_sha256": baseline, "candidate_binary_sha256": candidate,
               "source_after_sha256": after, "public_status": "not_claimed", "results": results}
    save("summary.json", summary)
    save("manifest.json", {path.name: digest(path) for path in sorted(OUT.iterdir()) if path.is_file()})
    print(json.dumps({k: v for k, v in summary.items() if k != "source_after_sha256"}), flush=True)
    return 0 if summary["status"] == "built_not_release_qualified" else 1


if __name__ == "__main__":
    def interrupted(signum: int, _frame: object) -> None:
        raise InterruptedError(f"signal {signum}")

    for handled in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(handled, interrupted)
    raise SystemExit(main())
