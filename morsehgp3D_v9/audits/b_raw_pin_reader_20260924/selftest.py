#!/usr/bin/env python3
"""Small causal rejection tests for read_pins.py; touches temporary copies only."""

import hashlib
import json
import re
import shutil
import sys
import tempfile
from pathlib import Path

from read_pins import ROOT, RECEIPT, ReaderError, read_inputs, verify


def update_manifest(receipt: Path, relative: str) -> None:
    path = receipt / relative
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    manifest = receipt / "SHA256SUMS"
    lines = manifest.read_text(encoding="utf-8").splitlines()
    old = [line for line in lines if line.endswith(f"  ./{relative}")]
    if len(old) != 1:
        raise RuntimeError(f"selftest manifest entry missing: {relative}")
    lines[lines.index(old[0])] = f"{digest}  ./{relative}"
    manifest.write_text("\n".join(lines) + "\n", encoding="utf-8")


def changed_digest(receipt: Path) -> None:
    relative = "probes/b00_k5_batch.json"
    path = receipt / relative
    data = json.loads(path.read_text(encoding="utf-8"))
    old = data["tower_digest"]
    data["tower_digest"] = ("0" if old[0] != "0" else "1") + old[1:]
    path.write_text(json.dumps(data, separators=(",", ":")) + "\n", encoding="utf-8")
    update_manifest(receipt, relative)


def nonzero_exit(receipt: Path) -> None:
    relative = "probes/b01_k10_engine.time"
    path = receipt / relative
    body = path.read_text(encoding="utf-8")
    body, count = re.subn(r"(?m)^\s*Exit status: 0\s*$", "\tExit status: 1", body)
    if count != 1:
        raise RuntimeError("selftest GNU time exit line missing")
    path.write_text(body, encoding="utf-8")
    update_manifest(receipt, relative)


def missing_case(receipt: Path) -> None:
    (receipt / "probes/b02_k5_engine.json").unlink()


def main() -> int:
    try:
        inputs = read_inputs(ROOT)
        verify(RECEIPT, ROOT, inputs)
        cases = (
            ("missing_case", missing_case, "missing or linked receipt file"),
            ("changed_digest", changed_digest, "pinned tower digest"),
            ("nonzero_exit", nonzero_exit, "GNU time exit"),
        )
        for name, mutation, cause in cases:
            with tempfile.TemporaryDirectory(prefix="mhgp9_b_raw_reader_") as temp:
                copy = Path(temp) / "receipt"
                shutil.copytree(RECEIPT, copy)
                mutation(copy)
                try:
                    verify(copy, ROOT, inputs)
                except ReaderError as error:
                    if cause not in str(error):
                        raise RuntimeError(f"{name}: wrong rejection: {error}") from error
                else:
                    raise RuntimeError(f"{name}: mutant accepted")
        print("PASS: baseline and 3 causal mutants (missing case, changed digest, nonzero GNU time exit)")
        return 0
    except (ReaderError, OSError, RuntimeError, ValueError, KeyError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
