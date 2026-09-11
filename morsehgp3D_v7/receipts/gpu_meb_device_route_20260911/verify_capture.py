#!/usr/bin/env python3
"""Read-only local route receipt gate; normal Python and -O equivalent."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURES = ("fixture_r1", "stub_r1", "stub_r2", "stub_r3", "nvcc_r1", "san_root_r1")


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    manifest = read_json(ROOT / "manifest.json")
    need(manifest["device_executed"] is False and manifest["gcp_used"] is False, "local scope")
    for name, metadata in manifest["files"].items():
        need(not Path(name).is_absolute() and ".." not in Path(name).parts, "unsafe path")
        path = ROOT / name
        need(path.is_file() and not path.is_symlink(), "missing or symlink " + name)
        data = path.read_bytes()
        need(len(data) == metadata["size"] and hashlib.sha256(data).hexdigest() == metadata["sha256"],
             "file hash " + name)
    receipts = {}
    for name in CAPTURES:
        directory = ROOT / name
        receipt = read_json(directory / "receipt.json")
        receipts[name] = receipt
        need(receipt["status"] == ("failed" if name == "stub_r1" else "passed"), "capture status " + name)
        need(receipt["sources_stable"] is True and receipt["sources_before"] == receipt["sources_after"],
             "source stability " + name)
        need(receipt["device_executed"] is False and receipt["gcp_used"] is False, "capture scope " + name)
        for source, digest in receipt["sources_before"].items():
            path = directory / "source_snapshot" / source
            need(path.relative_to(ROOT).as_posix() in manifest["files"] and sha(path) == digest,
                 "captured source " + name + "/" + source)
        for command in receipt["commands"]:
            for stream in ("stdout", "stderr"):
                need(sha(directory / (command["name"] + "." + stream)) == command[stream + "_sha256"],
                     "captured log " + name)
            expected = 1 if name == "stub_r1" else command["expected_exit_code"]
            need(command["exit_code"] == expected, "exit code " + name)
    need([command["name"] for command in receipts["stub_r1"]["commands"]] == ["compile"], "failed attempt shape")
    need("ambiguous" in (ROOT / "stub_r1/compile.stderr").read_text(), "initial compile failure preserved")
    final = receipts["stub_r3"]["sources_before"]
    need(final == receipts["nvcc_r1"]["sources_before"] == receipts["san_root_r1"]["sources_before"],
         "O2/SAN/NVCC source mismatch")
    for source, digest in final.items():
        need(sha(ROOT / source) == digest, "source changed after qualification " + source)
    # The generator consumes the frozen CPU geometry and Gram oracle, not the
    # subsequently added private GPU selection flags or route headers.
    for source, digest in receipts["fixture_r1"]["sources_before"].items():
        if source.startswith("source/") and not source.endswith("/src/gpu/anchor_meb_selection_private.cuh"):
            need(sha(ROOT / source) == digest, "generator CPU dependency changed " + source)
    fixture = ROOT / "source/morsehgp3D_v7/tests/anchor_meb_fixtures.inc"
    need(sha(fixture) == "1ea6d0f74a256baf88e4c31cc001af13548f1bc9978c5a9daa9e70359767995d" and
         fixture.read_bytes() == (ROOT / "fixture_r1/generate.stdout").read_bytes(), "fixture provenance")
    generated = read_json(ROOT / "fixture_r1/generate.stderr")
    need(generated["checks"] == 11752 and generated["comparisons"] == 605 and generated["extra_shells"] == 197 and
         generated["accepted_supports"] == [82, 393, 110, 20], "Gram generator nonvacuity")
    for name in ("stub_r2", "stub_r3", "san_root_r1"):
        row = read_json(ROOT / name / "selftest.stdout")
        need(row["status"] == "passed" and row["backend"] == "HOST_STUB" and row["device_executed"] is False,
             "host gate scope " + name)
        need(row["checks"] == 21432 and row["compared"] == 605 and row["rejections"] == 44 and
             row["causal_flags"] == 4 and row["extra_shells"] == 197 and
             [row["q" + str(q)] for q in range(1, 5)] == [82, 393, 110, 20], "host gate nonvacuity " + name)
        need(row["batch_h2d_bytes"] == 111320 and row["batch_d2h_bytes"] == 67760 and
             row["resident_h2d_bytes"] == 8400 and row["host_validation_powers"] == 1997 and
             row["reported_selection_powers"] == 9697, "phase counters " + name)
        need((ROOT / name / "selftest.stderr").read_bytes() == b"", "unexpected gate stderr " + name)
    need(read_json(ROOT / "stub_r3/selftest.stdout") == read_json(ROOT / "san_root_r1/selftest.stdout"), "SAN/O2 physical report")
    need(receipts["san_root_r1"]["sanitizer_environment"] == {
        "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}, "sanitizers enabled")
    nvcc = receipts["nvcc_r1"]["commands"]
    need([command["name"] for command in nvcc] == ["version", "compile_link"], "no CUDA execution in local capture")
    for flag in ("--gpu-architecture=sm_120", "-fmad=false", "--expt-relaxed-constexpr", "-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror"):
        need(flag in nvcc[1]["argv"], "strict CUDA flag " + flag)
    need((ROOT / "nvcc_r1/compile_link.stderr").read_bytes() == b"", "CUDA compile diagnostics")
    print(json.dumps({"status": "passed", "captures": len(CAPTURES), "logical_files": len(manifest["files"]),
                      "host_o2_san_passed": True, "nvcc_compile_link_passed": True,
                      "device_executed": False, "gcp_used": False,
                      "manifest_sha256": sha(ROOT / "manifest.json")}))


if __name__ == "__main__":
    main()
