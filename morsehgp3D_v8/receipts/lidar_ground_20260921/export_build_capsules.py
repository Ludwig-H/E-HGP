#!/usr/bin/env python3
"""Export small, non-autonomous build capsules; never rerun a native command."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
from datetime import datetime, timezone

HERE = Path(__file__).resolve().parent
V8 = HERE.parents[1]
ROOT = V8.parent
sys.path.insert(0, str(V8 / "bench"))
import build_patchwork_ground as builder

BUILDS = {
    "release": ROOT / "build/v8_ground_patchwork_20260921",
    "sanitize": ROOT / "build/v8_ground_patchwork_sanitize_20260921",
}
PREFLIGHT = Path("/tmp/mhgp8-ground-preflight.4mfm_a74")
GENERATOR = Path("/tmp/mhgp8-patchwork-inspect.kMXFCspn/preflight.py")
BUILD_FILES = [
    "MANIFEST.json", "COMPLETION.json",
    *(f"command_{i:03d}.json" for i in range(9)),
    *(f"{prefix}{unit}.d" for prefix in ("pre_", "")
      for unit in ("patchworkpp", "plane_fit", "probe")),
    *(f"local_sources/{name}" for name in (
        "build_patchwork_ground.py", "patchwork_ground_probe.cpp", "run_p0_matrix.py")),
    *(f"third_party/{name}" for name in (
        "LICENSE", "cpp/patchworkpp/src/patchworkpp.cpp",
        "cpp/patchworkpp/include/patchwork/patchworkpp.h",
        "cpp/common/src/plane_fit.cpp", "cpp/common/include/patchwork/plane_fit.h",
        "cpp/common/include/patchwork/types.h")),
]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def now() -> str:
    return datetime.now(timezone.utc).isoformat()


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2, sort_keys=True, ensure_ascii=False)
        stream.write("\n")


def main() -> int:
    report_path = HERE / "BUILD_CAPSULES_EXPORT.json"
    if report_path.exists():
        raise FileExistsError(report_path)
    # Refuse reuse before creating any partial destination.
    destinations = [HERE / "builds" / name for name in BUILDS]
    destinations.append(HERE / "preflight/initial_12")
    for path in destinations:
        if path.exists():
            raise FileExistsError(path)
    preflight_files = sorted(path for path in PREFLIGHT.iterdir() if path.is_file())
    if len(preflight_files) != 28:
        raise ValueError("unexpected preflight population")
    pairs = [
        (build / name, HERE / "builds" / label / name)
        for label, build in BUILDS.items() for name in BUILD_FILES
    ]
    pairs.extend((path, HERE / "preflight/initial_12" / path.name)
                 for path in preflight_files)
    pairs.append((GENERATOR, HERE / "preflight/initial_12/preflight.py"))
    inputs = [Path(__file__).resolve(), *(source for source, _ in pairs)]
    report = {
        "schema": "mhgp8_lidar_ground_build_capsules_v1",
        "scope": "export_integrity_only_not_autonomous_qualification",
        "status": "failed",
        "started_utc": now(),
        "source_before": {str(path): sha(path) for path in inputs},
        "authorities_before": {},
        "authorities_after": {},
        "copies": [],
        "artifacts": {},
        "omitted": ["native binaries", "object files", "system header contents"],
        "closing_errors": [],
    }
    try:
        for label, build in BUILDS.items():
            report["authorities_before"][label] = builder.read(build)
        for source, destination in pairs:
            payload = source.read_bytes()
            digest = hashlib.sha256(payload).hexdigest()
            if digest != report["source_before"][str(source)]:
                raise ValueError(f"input changed before export: {source}")
            destination.parent.mkdir(parents=True, exist_ok=True)
            with destination.open("xb") as stream:
                stream.write(payload)
            if sha(destination) != digest:
                raise ValueError(f"export changed bytes: {destination}")
            report["copies"].append({
                "source": str(source),
                "destination": str(destination.relative_to(HERE)),
                "sha256": digest,
            })
        for label in BUILDS:
            write_json(HERE / "builds" / label / "BUILD_AUTHORITY.json",
                       report["authorities_before"][label])
        for label, build in BUILDS.items():
            report["authorities_after"][label] = builder.read(build)
        if report["authorities_before"] != report["authorities_after"]:
            raise ValueError("original LIVE build authorities changed")
        report["source_after"] = {str(path): sha(path) for path in inputs}
        if report["source_before"] != report["source_after"]:
            raise ValueError("export sources changed")
        for item in report["copies"]:
            path = HERE / item["destination"]
            if sha(path) != item["sha256"]:
                raise ValueError(f"export changed during closure: {path}")
            report["artifacts"][item["destination"]] = sha(path)
        for label in BUILDS:
            path = HERE / "builds" / label / "BUILD_AUTHORITY.json"
            report["artifacts"][str(path.relative_to(HERE))] = sha(path)
        report["status"] = "passed"
    except BaseException as error:
        report["closing_errors"].append(f"{type(error).__name__}: {error}")
    finally:
        report["finished_utc"] = now()
        write_json(report_path, report)
    print(json.dumps({
        "status": report["status"], "report": str(report_path),
        "copies": len(report["copies"]), "artifacts": len(report["artifacts"]),
        "closing_errors": report["closing_errors"],
    }, sort_keys=True))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
