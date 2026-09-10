#!/usr/bin/env python3
"""Portable witness primitive evidence reader. Never execute ELF/CUDA code."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil


def need(value: bool, reason: str) -> None:
    if not value:
        raise RuntimeError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(root: Path) -> dict:
    manifest = json.loads((root / "manifest.json").read_text())
    files = set()
    for path in root.rglob("*"):
        need(not path.is_symlink(), "symlink refused")
        if path.is_file():
            files.add(str(path.relative_to(root)))
    need(files == set(manifest) | {"manifest.json"}, "manifest inventory")
    for relative, item in manifest.items():
        path = Path(relative)
        need(not path.is_absolute() and ".." not in path.parts, "manifest path")
        raw = (root / path).read_bytes()
        need(not raw.startswith(b"\x7fELF"), "ELF forbidden")
        need(len(raw) == item["bytes"] and hashlib.sha256(raw).hexdigest() == item["sha256"], "manifest digest")
    capture = json.loads((root / "capture.json").read_text())
    need(capture["device_executed"] is False and capture["GCP"] == "not_used_by_this_agent", "capture scope")
    summaries = {}
    for name, expected_status in (("run_r1", "failed"), ("run_r2", "completed")):
        directory = root / "runs" / name
        receipt_path = directory / "receipt.json"
        need(sha(receipt_path) == capture["runs"][name]["receipt_sha256"], "original receipt pin")
        receipt = json.loads(receipt_path.read_text())
        need(receipt["status"] == expected_status and receipt["backend"] == "stub-hote" and
             receipt["device_executed"] is False, "run scope")
        need(receipt["sources_stable"] and receipt["snapshot_stable"] and
             receipt["sources_before"] == receipt["sources_after"], "run source stability")
        view = json.loads((root / "source_views" / (name + ".json")).read_text())
        need(view == receipt["sources_before"], "source view binding")
        for relative, digest in view.items():
            path = Path(relative)
            need(not path.is_absolute() and ".." not in path.parts, "source view path")
            need(isinstance(digest, str) and re.fullmatch(r"[0-9a-f]{64}", digest) is not None, "object name")
            need(sha(root / "objects" / digest) == digest, "source object pin")
        commands = {}
        failed = []
        for row in receipt["commands"]:
            need(row["name"] not in commands, "duplicate command")
            commands[row["name"]] = row
            if row["returncode"] != row["expected"]:
                failed.append(row["name"])
            for stream in ("stdout", "stderr"):
                item = row[stream]
                relative = Path(item["path"])
                need(not relative.is_absolute() and ".." not in relative.parts, "stream path")
                path = directory / relative
                need(path.stat().st_size == item["bytes"] and sha(path) == item["sha256"], "stream pin")
        need(failed == (["compile_and_link_CUDA"] if name == "run_r1" else []), "failure ledger")
        if name == "run_r1":
            row = commands["compile_and_link_CUDA"]
            need(row["returncode"] == 1 and "maybe-uninitialized" in (directory / row["stderr"]["path"]).read_text(),
                 "original CUDA failure")
        else:
            need(commands["compile_and_link_CUDA"]["returncode"] == 0 and
                 commands["compile_and_link_CUDA"]["stderr"]["bytes"] == 0, "CUDA compile/link scope")
        for mode in ("O2", "SAN"):
            row = commands["selftest_" + mode]
            raw = (directory / row["stdout"]["path"]).read_text()
            match = re.search(r"checks=(\d+) failures=(\d+) rows=(\d+).*rejected=(\d+) semantic_differences=(\d+)", raw)
            need(match is not None and row["returncode"] == 0, "host gate")
            checks, failures, rows, rejects, differences = map(int, match.groups())
            need(checks >= 74000 and failures == 0 and rows >= 73000 and rejects >= 37 and differences == 0,
                 "host nonvacuity")
            for mutant in ("no-mask", "reverse-push"):
                row = commands["mutant_" + mode + "_" + mutant]
                match = re.search(r"semantic_differences=(\d+)", (directory / row["stdout"]["path"]).read_text())
                need(row["returncode"] == 1 and match is not None and int(match.group(1)) > 0, "causal mutant")
            need(commands["unknown_" + mode]["returncode"] == 2, "CLI rejection")
        for key, item in receipt["binaries"].items():
            need(item["sha256"] == item["sha256_after"] == capture["runs"][name]["binary_sha256_at_capture"][key],
                 "binary hash binding")
        summaries[name] = dict(status=expected_status, sources=len(view), commands=len(commands))
    print(json.dumps(dict(status="PASS", scope="stub_runtime_and_CUDA_compile_link_only", device_executed=False,
                          objects=len(list((root / "objects").iterdir())), runs=summaries), sort_keys=True))
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--extract", choices=("run_r1", "run_r2"))
    parser.add_argument("--destination", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    verify(root)
    if args.extract is not None:
        need(args.destination is not None, "extraction destination required")
        args.destination.mkdir(parents=True, exist_ok=False)
        view = json.loads((root / "source_views" / (args.extract + ".json")).read_text())
        for relative, digest in view.items():
            path = args.destination / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / "objects" / digest, path)
    else:
        need(args.destination is None, "destination without extraction")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("REFUS", str(error))
        raise SystemExit(1)
