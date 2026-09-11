#!/usr/bin/env python3
"""Read-only local receipt judge. No compile, CUDA, GCP, or ELF requirement."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
EXPECTED = {
    "status": "passed", "backend": "HOST_STUB", "sm": "0.0", "checks": 22245,
    "compared": 605, "q1": 82, "q2": 393, "q3": 110, "q4": 20,
    "extra_shells": 197, "rejections": 47, "causal_flags": 4, "failures": 0,
    "device_name": "host_stub", "device_executed": False,
    "scope": "local_meb_device_key_only", "gcp_used": False, "abi_version": 2,
    "request_bytes": 72, "selection_bytes": 208, "positions": 350,
    "resident_h2d_bytes": 8400, "batch_h2d_bytes": 169400,
    "batch_d2h_bytes": 125840, "batch_launches": 1,
    "host_validation_powers": 0, "host_materializations": 0,
    "reported_selection_powers": 9697, "backend_key_materializations": 605,
    "key_words_compared": 6050, "key_high_words_nonzero": 1384,
    "key_mutations": 17, "legacy_host_materialize_deleted": True,
    "device_level_available": False,
}


def need(value, reason):
    if not value:
        raise RuntimeError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def safe(base, relative):
    path = Path(relative)
    need(not path.is_absolute() and ".." not in path.parts, "unsafe path")
    candidate = base / path
    need(candidate.is_file() and not candidate.is_symlink(), "missing or linked file: " + str(candidate))
    return candidate


def main():
    captures = sorted(ROOT.glob("*/receipt.json"))
    need(bool(captures), "no captures")
    passed = {}
    failures = []
    for capture in captures:
        folder = capture.parent
        receipt = json.loads(capture.read_text())
        need(receipt["status"] in ("passed", "failed"), "capture still running")
        need(receipt["device_executed"] is False and receipt["GCP_used"] is False, "unexpected external execution")
        before = receipt["sources_before"]
        need(receipt["sources_stable"] is True and before == receipt["sources_after"], "unstable capture")
        for relative, digest in before.items():
            need(sha(safe(folder / "source_snapshot", relative)) == digest, "snapshot pin")
        commands = receipt["commands"]
        for command in commands:
            for stream in ("stdout", "stderr"):
                need(sha(safe(folder, command["name"] + "." + stream)) == command[stream + "_sha256"], "log pin")
        if receipt["status"] == "failed":
            failures.append(folder.name)
            continue
        mode = receipt["mode"]
        expected_codes = [0, 0] if mode == "nvcc" else [0, 0, 0, 2, 2]
        need([row["exit_code"] for row in commands] == expected_codes, "unexpected exits")
        need([row["expected_exit_code"] for row in commands] == expected_codes, "unexpected contract")
        need(all((folder / (row["name"] + ".stderr")).stat().st_size == 0 for row in commands), "successful capture has stderr")
        compile_command = commands[1]["argv"]
        expected_gate = "/" + folder.name + "/source_snapshot/source/morsehgp3D_v7/tests/anchor_meb_key_route_gate.cu"
        need(sum(arg.endswith(expected_gate) for arg in compile_command) == 1, "not compiling captured gate")
        if mode == "nvcc":
            need("--gpu-architecture=sm_120" in compile_command and "-fmad=false" in compile_command,
                 "CUDA architecture/rounding flags")
            need("-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror" in compile_command, "CUDA strict flags")
            need(sum(arg.endswith("/" + folder.name + "/source_snapshot/nvcc_strict_host.py")
                     for arg in compile_command) == 1, "uncaptured adapter")
        else:
            for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"):
                need(flag in compile_command, "missing strict host flag")
            raw = (folder / "selftest.stdout").read_text()
            need(len(raw.splitlines()) == 1 and json.loads(raw) == EXPECTED, "selftest JSON contract")
            need((folder / "unknown.stdout").stat().st_size == 0 and
                 (folder / "missing_arg.stdout").stat().st_size == 0, "invalid CLI wrote output")
            if mode == "san":
                need("-fsanitize=address,undefined" in compile_command, "missing SAN flags")
                need(receipt["sanitizer_environment"] == {
                    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}, "SAN disabled")
        passed.setdefault(mode, []).append((folder.name, before))
    need(set(passed) == {"stub", "san", "nvcc"}, "O2/SAN/NVCC qualification incomplete")
    final_sources = passed["stub"][-1][1]
    for mode in passed:
        need(passed[mode][-1][1] == final_sources, "final modes did not consume identical sources")
    for relative, digest in final_sources.items():
        need(sha(safe(ROOT, relative)) == digest, "current qualified source drift")
    print(json.dumps({"status": "passed", "modes": {mode: rows[-1][0] for mode, rows in passed.items()},
                      "preserved_failed_captures": failures, "compared": 605,
                      "device_executed": False, "gcp_used": False, "elf_required": False}, sort_keys=True))


if __name__ == "__main__":
    main()
