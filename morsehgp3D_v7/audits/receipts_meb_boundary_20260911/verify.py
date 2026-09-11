#!/usr/bin/env python3
"""Verify frozen captures and the independent rational K7 certificate."""

import hashlib
import json
import tarfile
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SAN_ENV = {
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
}
EXPECTED_OUTPUT = (
    "PASS original_rejection reference_ok hybrid_canon_fail outside_power=176\n"
    "PASS fallback=1 all_fields_equal=1 proposal_candidates=167 "
    "proposal_powers=415 fallback_calls=1 fallback_powers=71\n"
    "PASS fallback=0 all_fields_equal=1 proposal_candidates=12 "
    "proposal_powers=71 fallback_calls=0 fallback_powers=0\n"
)


def need(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def validate_commands(rows, embedded=False):
    names = ["O2_compile", "O2_run", "SAN_compile", "SAN_run"]
    need([row["name"] for row in rows] == names, "four exact commands")
    for row in rows:
        name = row["name"]
        need(row["exit_code"] == 0, "exit: " + name)
        stderr = row["stderr"] if embedded else (ROOT / (name + ".stderr")).read_text()
        stdout = row["stdout"] if embedded else (ROOT / (name + ".stdout")).read_text()
        need(stderr == "", "stderr: " + name)
        need(row["sanitizer_environment"] == (SAN_ENV if name.startswith("SAN") else {}), "environment")
        if name.endswith("compile"):
            flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
            flags += ["-O2"] if name.startswith("O2") else [
                "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"
            ]
            need(all(flag in row["argv"] for flag in flags), "compile flags")
            need("-w" not in row["argv"], "no global warning suppression")
            need(any(arg.endswith("/guard.cpp") for arg in row["argv"]), "compiled guard")
            need(stdout == "", "compiler stdout")
        else:
            need(stdout == EXPECTED_OUTPUT, "exact boundary and positive control output")


def rational_certificate():
    q = Fraction
    points = [(2, 3, 2), (2, 0, 0), (0, 2, 2), (1, 0, 0),
              (2, 2, 0), (3, 0, 1), (0, 2, 3)]
    need(len(set(points)) == 7, "distinct sites")
    center = [q(29, 22), q(23, 22), q(37, 22)]
    radius_squared = q(193, 44)
    support = [0, 1, 5, 6]
    weights = [q(1, 22), q(3, 11), q(5, 22), q(5, 11)]
    need(all(weight > 0 for weight in weights) and sum(weights) == 1, "positive support")
    need(all(sum(weight * points[index][axis] for index, weight in zip(support, weights)) == center[axis]
             for axis in range(3)), "center in positive support hull")
    powers = [sum((q(point[axis]) - center[axis]) ** 2 for axis in range(3)) - radius_squared
              for point in points]
    need(powers == [0, 0, q(-18, 11), q(-4, 11), q(-2, 11), 0, 0], "true ball powers")
    need(all(power <= 0 for power in powers) and all(powers[index] == 0 for index in support), "MEB certificate")
    wrong_center = [q(29, 18), q(23, 18), q(23, 18)]
    wrong_powers = [sum((q(point[axis]) - wrong_center[axis]) ** 2 for axis in range(3)) - q(131, 36)
                    for point in points]
    need(wrong_powers == [0, q(-2, 9), 0, 0, q(-4, 3), 0, q(22, 9)], "proposal powers")
    need(wrong_powers[-1] * 72 == 176, "integer/rational diagnostic agreement")


def main():
    seal = {}
    for line in (ROOT / "SHA256SUMS").read_text().splitlines():
        expected, name = line.split("  ", 1)
        need(Path(name).name == name and name not in seal, "flat unique packet path")
        seal[name] = expected
        need(digest((ROOT / name).read_bytes()) == expected, "pin: " + name)
    need(set(seal) == {path.name for path in ROOT.iterdir() if path.name != "SHA256SUMS"}, "packet closure")
    source_manifest = json.loads((ROOT / "source_manifest.json").read_text())
    with tarfile.open(ROOT / "base.tar.gz") as archive:
        need(archive.pax_headers.get("comment") == source_manifest["base_commit"], "archive commit")
        files = {member.name: digest(archive.extractfile(member).read())
                 for member in archive.getmembers() if member.isfile()}
    need(files == source_manifest["files"] and len(files) == 57, "complete frozen source archive")
    frozen = json.loads((ROOT / "final_sources.json").read_text())
    need(set(frozen) == {"guard.cpp", "guarded_run.hpp", "meb_hybrid.cpp"}, "compiled local closure")
    for name, expected in frozen.items():
        need(digest((ROOT / name).read_bytes()) == expected, "compiled local pin")
    need(frozen["meb_hybrid.cpp"] == json.loads((ROOT / "pins.json").read_text())["prototype_sha256"], "original prototype")
    validate_commands(json.loads((ROOT / "commands.json").read_text()))
    exploration = json.loads((ROOT / "exploration.json").read_text())
    need([row["name"] for row in exploration["commands"]] == ["probe", "fixture"], "exploration closure")
    for row in exploration["commands"]:
        need(row["compile_exit_code"] == 0 and row["run_exit_code"] == 1, "original failures preserved")
        for stream in ["stdout", "stderr"]:
            need(digest((ROOT / (row["name"] + "." + stream)).read_bytes()) == row[stream + "_sha256"], "exploratory stream")
    need("hybrid=3 canon_fail" in (ROOT / "probe.stdout").read_text(), "first negative")
    need("FAIL n=7 mask=127" in (ROOT / "fixture.stdout").read_text(), "reduced negative")
    replay = json.loads((ROOT / "root_replay.json").read_text())
    need("morsehgp3D_v7/src/gpu/device_witness.cu" in files, "archived unrelated CUDA TU")
    expected_sources = {"source/" + name: expected for name, expected in files.items()
                        if name != "morsehgp3D_v7/src/gpu/device_witness.cu"}
    expected_sources.update(frozen)
    expected_sources["record.py"] = digest((ROOT / "record_original.source").read_bytes())
    need(replay["sources_before"] == replay["sources_after"] == expected_sources, "replay full source stability")
    need(replay["exit_code"] == 0 and replay["stderr"] == "" and replay["source_bytes_stable"] is True, "root replay success")
    need(replay["device_executed"] is False and replay["gcp_used"] is False, "scope")
    validate_commands(replay["commands"], embedded=True)
    rational_certificate()
    print(json.dumps({"status": "passed_meb_boundary_evidence", "archived_source_files": 57, "stable_replay_headers": 56,
                      "original_hybrid_rejects_valid_K7": True, "guarded_O2_SAN": True,
                      "independent_replay_O2_SAN": True, "rational_certificate": True,
                      "device_executed": False, "GCP_used": False}, sort_keys=True))


if __name__ == "__main__":
    main()
