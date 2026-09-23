#!/usr/bin/env python3
"""Build and run the audit-only 12-site global q4 stream gate."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, required=True,
                        help="v9 source tree containing src/gen and tests/gen")
    parser.add_argument("--build", type=Path, required=True,
                        help="matching CMake build containing libmhgp9_gen.a")
    parser.add_argument("--receipt", type=Path,
                        help="write the JSON receipt after execution")
    args = parser.parse_args()
    source = args.source_root.resolve()
    build = args.build.resolve()
    sidecar = Path(__file__).with_name("check.cpp")
    gate = source / "tests/gen/wspd_q34_gate.cpp"
    library = build / "libmhgp9_gen.a"
    cache = build / "CMakeCache.txt"
    marker = "\nint main(int argc,char** argv) {"
    original = gate.read_text()
    if original.count(marker) != 1:
        raise RuntimeError("product gate main signature changed; rename must be reviewed")
    renamed = original.replace(marker, "\nint mhgp9_original_global_gate_main(int argc,char** argv) {", 1)
    boost_line = next((line for line in cache.read_text().splitlines()
                       if line.startswith("MHGP9_BOOST_INCLUDE_DIR:PATH=")), None)
    if boost_line is None:
        raise RuntimeError("matching CMake build has no Boost include path")
    boost = Path(boost_line.split("=", 1)[1])
    if not boost.joinpath("boost/multiprecision/cpp_int.hpp").is_file():
        raise RuntimeError("Boost headers of matching build are absent")
    with tempfile.TemporaryDirectory(prefix="mhgp9-q4-global-12-") as temporary:
        directory = Path(temporary)
        (directory / "wspd_q34_gate_renamed.cpp").write_text(renamed)
        executable = directory / "check"
        command = ["c++", "-std=c++20", "-O2", "-DNDEBUG", "-Wall", "-Wextra",
                   "-Wpedantic", "-Werror", "-I", str(directory),
                   "-I", str(source / "tests/gen"), "-I", str(source / "src/gen"),
                   "-isystem", str(boost), str(sidecar), str(library), "-pthread",
                   "-o", str(executable)]
        subprocess.run(command, check=True)
        completed = subprocess.run([str(executable)], check=False, capture_output=True,
                                   text=True, timeout=600)
        receipt = {
            "status": "PASS" if completed.returncode == 0 else "FAIL",
            "exit_code": completed.returncode,
            "source_commit": subprocess.check_output(["git", "-C", str(source),
                                                      "rev-parse", "HEAD"], text=True).strip(),
            "product_gate_sha256": sha256(gate),
            "sidecar_sha256": sha256(sidecar),
            "runner_sha256": sha256(Path(__file__)),
            "library_sha256": sha256(library),
            "binary_sha256": sha256(executable),
            "compiler": subprocess.check_output(["c++", "--version"], text=True).splitlines()[0],
            "gen_worktree_status": subprocess.check_output(
                ["git", "-C", str(source), "status", "--porcelain", "--", "src/gen"],
                text=True).strip(),
            "test_worktree_status": subprocess.check_output(
                ["git", "-C", str(source), "status", "--porcelain", "--", "tests/gen"],
                text=True).strip(),
            "stdout": completed.stdout.strip(),
            "stderr": completed.stderr.strip(),
        }
        if args.receipt is not None:
            args.receipt.write_text(json.dumps(receipt, sort_keys=True, indent=2) + "\n")
        print(json.dumps(receipt, sort_keys=True))
        if completed.returncode != 0:
            raise SystemExit(completed.returncode)


if __name__ == "__main__":
    main()
