#!/usr/bin/env python3
"""Exact Fraction preflight and complete-stream C++ oracle, audit-only."""

from __future__ import annotations

import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


SUPPORT = ((30, 30, 30), (30, 10, 10), (10, 30, 10), (10, 10, 30))
CORE = ((20, 20, 20), (21, 20, 20), (20, 21, 20), (20, 20, 21),
        (19, 20, 20), (20, 19, 20), (20, 20, 19))
ORIGIN = (20, 20, 20)


def fraction_gate() -> dict[str, object]:
    witnesses = tuple(
        tuple(20 - radius * ((v[j] - 20) // 10) for j in range(3))
        for v in SUPPORT for radius in (11, 12)
    )

    def power(point: tuple[int, int, int], centre: tuple[Fraction, ...], radius2: Fraction) -> Fraction:
        return sum((Fraction(point[j]) - centre[j]) ** 2 for j in range(3)) - radius2

    result: dict[str, object] = {}
    for p in (1, 2, 7):
        points = SUPPORT + witnesses + CORE[:p]
        if len(points) != len(set(points)) or not all(0 <= x < 1 << 18 for point in points for x in point):
            raise RuntimeError("fixture sites are duplicate or outside u18")
        centre4 = tuple(Fraction(x) for x in ORIGIN)
        powers4 = [power(point, centre4, Fraction(300)) for point in points]
        if powers4[:4] != [0] * 4 or any(x <= 0 for x in powers4[4:12]) or any(x >= 0 for x in powers4[12:]):
            raise RuntimeError("target q4 support/shell/interior changed")
        face_depths = []
        for omitted, vertex in enumerate(SUPPORT):
            centre3 = tuple(Fraction(ORIGIN[j]) - Fraction(vertex[j] - ORIGIN[j], 3) for j in range(3))
            powers3 = [power(point, centre3, Fraction(800, 3)) for point in points]
            if any(powers3[j] != 0 for j in range(4) if j != omitted):
                raise RuntimeError("q3 face contact changed")
            if powers3[omitted] <= 0:
                raise RuntimeError("omitted q4 vertex is not outside q3 face")
            face_depths.append(sum(value < 0 for value in powers3))
        k = p + 3
        if face_depths != [k - 1] * 4 or not (p < k - 2) or not (p >= (k - 1) - 2):
            raise RuntimeError("q3/q4 acceptance thresholds changed")
        result[str(k)] = {"sites": len(points), "q4_depth": p, "q3_face_depths": face_depths}
    return result


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cache_value(cache: Path, name: str) -> str:
    matches = [line.split("=", 1)[1] for line in cache.read_text().splitlines()
               if line.startswith(name + ":")]
    if len(matches) != 1:
        raise RuntimeError(f"CMakeCache has no unique {name}")
    return matches[0]


def git_output(source: Path, *arguments: str) -> str:
    return subprocess.check_output(["git", "-C", str(source), *arguments], text=True).strip()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    source = args.source_root.resolve()
    build = args.build.resolve()
    gate = source / "tests/gen/wspd_q34_gate.cpp"
    library = build / "libmhgp9_gen.a"
    sidecar = Path(__file__).with_name("check.cpp")
    cache = build / "CMakeCache.txt"
    marker = "\nint main(int argc,char** argv) {"
    original = gate.read_text()
    if original.count(marker) != 1:
        raise RuntimeError("product gate main signature changed")
    renamed = original.replace(marker, "\nint mhgp9_original_global_gate_main(int argc,char** argv) {", 1)
    build_source = Path(cache_value(cache, "CMAKE_HOME_DIRECTORY")).resolve()
    if build_source != source:
        raise RuntimeError("CMake build was configured against another source tree")
    boost = Path(cache_value(cache, "MHGP9_BOOST_INCLUDE_DIR"))
    if not boost.joinpath("boost/multiprecision/cpp_int.hpp").is_file():
        raise RuntimeError("Boost headers are absent")
    source_commit_before = git_output(source, "rev-parse", "HEAD")
    gen_status_before = git_output(source, "status", "--porcelain", "--", "src/gen")
    test_status_before = git_output(source, "status", "--porcelain", "--", "tests/gen")
    if gen_status_before or test_status_before:
        raise RuntimeError("generator or oracle source worktree is dirty")
    build_command = ["cmake", "--build", str(build), "--target", "mhgp9_gen", "-j4"]
    build_result = subprocess.run(build_command, capture_output=True, text=True, check=True)
    library_hash_before = sha256(library)
    fraction = fraction_gate()
    with tempfile.TemporaryDirectory(prefix="mhgp9-q4-depth-ladder-") as temporary:
        directory = Path(temporary)
        (directory / "wspd_q34_gate_renamed.cpp").write_text(renamed)
        executable = directory / "check"
        command = ["c++", "-std=c++20", "-O2", "-DNDEBUG", "-Wall", "-Wextra",
                   "-Wpedantic", "-Werror", "-I", str(directory),
                   "-I", str(source / "tests/gen"), "-I", str(source / "src/gen"),
                   "-isystem", str(boost), str(sidecar), str(library), "-pthread",
                   "-o", str(executable)]
        subprocess.run(command, check=True)
        completed = subprocess.run([str(executable)], capture_output=True, text=True, timeout=1800)
        source_commit_after = git_output(source, "rev-parse", "HEAD")
        gen_status_after = git_output(source, "status", "--porcelain", "--", "src/gen")
        test_status_after = git_output(source, "status", "--porcelain", "--", "tests/gen")
        library_hash_after = sha256(library)
        if (source_commit_before != source_commit_after or gen_status_after or test_status_after or
                library_hash_before != library_hash_after):
            raise RuntimeError("generator source, test source or linked library changed during gate")
        receipt = {
            "status": "PASS" if completed.returncode == 0 else "FAIL",
            "exit_code": completed.returncode,
            "fraction_gate": fraction,
            "source_commit": source_commit_before,
            "cmake_home_directory": str(build_source),
            "cmake_build_type": cache_value(cache, "CMAKE_BUILD_TYPE"),
            "cmake_sanitize": cache_value(cache, "MHGP9_SANITIZE"),
            "cmake_cache_sha256": sha256(cache),
            "build_command": build_command,
            "build_stdout": build_result.stdout.strip(),
            "build_stderr": build_result.stderr.strip(),
            "product_gate_sha256": sha256(gate),
            "sidecar_sha256": sha256(sidecar),
            "runner_sha256": sha256(Path(__file__)),
            "library_sha256_before": library_hash_before,
            "library_sha256_after": library_hash_after,
            "binary_sha256": sha256(executable),
            "compiler": subprocess.check_output(["c++", "--version"], text=True).splitlines()[0],
            "gen_worktree_status_before": gen_status_before,
            "gen_worktree_status_after": gen_status_after,
            "test_worktree_status_before": test_status_before,
            "test_worktree_status_after": test_status_after,
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
