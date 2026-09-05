"""Build and audit the current CPU gates, keeping all outputs in audits/."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time
import xml.etree.ElementTree as ET


AUDITS = Path(__file__).resolve().parent
LINEAGE = AUDITS.parent
ROOT = LINEAGE.parent
RUN_NAME = os.environ.get("MHGP7_AUDIT_RUN_NAME", "20260905")
if re.fullmatch(r"[a-zA-Z0-9_-]+", RUN_NAME) is None:
    raise SystemExit("invalid MHGP7_AUDIT_RUN_NAME")
WORK = AUDITS / f".work_{RUN_NAME}_release"
RECEIPT = AUDITS / f"receipts_{RUN_NAME}/release"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sources() -> dict[str, str]:
    paths = [LINEAGE / "CMakeLists.txt"]
    for name in ("src", "tests", "oracle", "cmake", "cli", "bench"):
        paths.extend(
            p for p in (LINEAGE / name).rglob("*")
            if p.is_file() and "__pycache__" not in p.parts
        )
    return {str(p.relative_to(LINEAGE)): digest(p) for p in sorted(paths)}


def write(name: str, value: object) -> None:
    (RECEIPT / name).write_text(
        json.dumps(value, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    # A new directory is required: no accidental reuse or erased failed run.
    WORK.mkdir()
    RECEIPT.mkdir(parents=True)
    (WORK / "tmp").mkdir()
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TMPDIR=str(WORK / "tmp"))
    summary: dict[str, object] = {
        "status": "running", "public_status": "not_claimed",
        "backend": "cpu_reference", "gcp": "not_used", "steps": [],
        "source_head": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
        ).strip(),
        "worktree_before": subprocess.check_output(
            ["git", "status", "--short"], cwd=ROOT, text=True
        ),
    }
    before = sources()
    write("sources_before.json", before)
    build = WORK / "build"
    commands = [
        ("configure", ["cmake", "-S", str(LINEAGE), "-B", str(build),
                       "-DCMAKE_BUILD_TYPE=Release", "-DMHGP7_ENABLE_CUDA=OFF",
                       "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"]),
        ("build", ["cmake", "--build", str(build), "--parallel", "2"]),
        ("inventory", ["ctest", "--test-dir", str(build), "--show-only=json-v1",
                       "--no-tests=error", "-L", "^gate$"]),
        ("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure",
                   "--no-tests=error", "-L", "^gate$", "--parallel", "2",
                   "--output-junit", str(RECEIPT / "ctest.junit.xml")]),
    ]
    try:
        for name, command in commands:
            if name == "ctest":
                write("binaries_before.json", {
                    p.name: digest(p) for p in sorted(build.glob("mhgp7*"))
                    if p.is_file() and os.access(p, os.X_OK)
                })
            start = time.monotonic()
            with (RECEIPT / f"{name}.stdout").open("w") as out:
                run = subprocess.run(command, cwd=ROOT, env=env, stdout=out,
                                     stderr=subprocess.STDOUT, check=False)
            summary["steps"].append({
                "name": name, "command": command, "returncode": run.returncode,
                "seconds": time.monotonic() - start,
            })
            write("summary.json", summary)
            print(f"{name}: returncode={run.returncode}", flush=True)
            if run.returncode:
                raise RuntimeError(f"{name} failed")
        cases = ET.parse(RECEIPT / "ctest.junit.xml").getroot().findall(".//testcase")
        inventory = json.loads((RECEIPT / "inventory.stdout").read_text())["tests"]
        names = [case.attrib["name"] for case in cases]
        expected_names = {case["name"] for case in inventory}
        if len(names) < 323 or len(names) != len(set(names)) or set(names) != expected_names:
            raise RuntimeError("gate inventory mismatch or non-vacuity failure")
        if any(case.find(tag) is not None for case in cases
               for tag in ("failure", "error", "skipped")):
            raise RuntimeError("JUnit contains failures, errors or skips")
        binaries_after = {
            p.name: digest(p) for p in sorted(build.glob("mhgp7*"))
            if p.is_file() and os.access(p, os.X_OK)
        }
        write("binaries_after.json", binaries_after)
        if binaries_after != json.loads((RECEIPT / "binaries_before.json").read_text()):
            raise RuntimeError("binary changed during CTest")
        summary.update(status="completed", tests=len(names), failures=0, skipped=0)
    except (OSError, ValueError, RuntimeError, ET.ParseError) as error:
        summary.update(status="failed", error=str(error))
    finally:
        after = sources()
        write("sources_after.json", after)
        summary["sources_stable"] = before == after
        if before != after:
            summary.update(status="invalid", error="source changed during audit")
        summary["environment"] = {
            "PYTHONDONTWRITEBYTECODE": "1", "TMPDIR": str(WORK / "tmp"),
            "timings": "shared host, qualification durations only",
        }
        for name in ("CMakeCache.txt", "compile_commands.json"):
            path = build / name
            if path.is_file():
                (RECEIPT / name.replace(".json", ".txt")).write_bytes(path.read_bytes())
        write("summary.json", summary)
        write("receipt_manifest.json", {
            p.name: digest(p) for p in sorted(RECEIPT.iterdir())
            if p.is_file() and p.name != "receipt_manifest.json"
        })
    return 0 if summary["status"] == "completed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
