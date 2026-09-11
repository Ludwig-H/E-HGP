#!/usr/bin/env python3
"""Read-only evidence reader, effective under normal Python and -O."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    manifest = read_json(ROOT / "manifest.json")
    need(manifest["device_executed"] is False and manifest["gcp_used"] is False,
         "invalid scope")
    for name, entry in manifest["files"].items():
        path = ROOT / name
        need(not Path(name).is_absolute() and ".." not in Path(name).parts,
             "unsafe manifest path")
        need(not path.is_symlink() and path.is_file(), "missing file " + name)
        content = path.read_bytes()
        need(len(content) == entry["size"] and hashlib.sha256(content).hexdigest() == entry["sha256"],
             "file hash mismatch " + name)
    expected = {
        "o2_r1": ("failed", [("compile_oracle", 0), ("run_oracle", 0),
                              ("args_oracle", 2), ("compile_transport", 1)]),
        "o2_r2": ("passed", [("compile_oracle", 0), ("run_oracle", 0),
                              ("args_oracle", 2), ("compile_transport", 0),
                              ("run_transport", 0), ("args_transport", 2)]),
        "nvcc_r1": ("passed", [("nvcc_version", 0), ("strict_kernel_compile_link", 0)]),
        "san_r1": ("failed", [("compile_oracle", 0), ("run_oracle", 1)]),
    }
    receipts = {}
    for name, (status, commands) in expected.items():
        directory = ROOT / name
        receipt = read_json(directory / "receipt.json")
        receipts[name] = receipt
        need(receipt["status"] == status and receipt["sources_stable"] is True,
             "capture status " + name)
        need(receipt["device_executed"] is False and receipt["gcp_used"] is False,
             "capture scope " + name)
        need(receipt["sources_before"] == receipt["sources_after"], "unstable source " + name)
        need([(c["name"], c["exit_code"]) for c in receipt["commands"]] == commands,
             "capture commands " + name)
        for source, digest in receipt["sources_before"].items():
            relative = name + "/source_snapshot/" + source
            need(relative in manifest["files"], "unsealed source " + relative)
            need(sha(ROOT / relative) == digest, "captured source " + relative)
        for command in receipt["commands"]:
            for stream in ("stdout", "stderr"):
                path = directory / (command["name"] + "." + stream)
                need(sha(path) == command[stream + "_sha256"], "captured log " + str(path))
    for name in ("nvcc_r1", "san_r1"):
        need(receipts[name]["sources_before"] == receipts["o2_r2"]["sources_before"],
             "qualified sources differ " + name)
    for source, digest in receipts["o2_r2"]["sources_before"].items():
        need(sha(ROOT / source) == digest, "current source differs " + source)
    for name in ("o2_r1", "o2_r2"):
        result = read_json(ROOT / name / "run_oracle.stdout")
        need(result["status"] == "pass" and result["checks"] == 16592 and
             result["comparisons"] == 605 and result["permutations"] == 300 and
             result["extra_shells"] == 197 and result["accepted_supports"] == [82, 393, 110, 20],
             "oracle nonvacuity " + name)
    result = read_json(ROOT / "o2_r2/run_transport.stdout")
    need(result == {"status": "passed", "checks": 253, "rejections": 33,
                    "causal_flags": 4, "device_executed": False}, "transport nonvacuity")
    need("expected" in (ROOT / "o2_r1/compile_transport.stderr").read_text() and
         "namespace" in (ROOT / "o2_r1/compile_transport.stderr").read_text(), "lost compile failure")
    need("LeakSanitizer does not work under ptrace" in (ROOT / "san_r1/run_oracle.stderr").read_text(),
         "lost SAN environment failure")
    need((ROOT / "san_r1/run_oracle.stdout").read_bytes() == b"", "unexpected SAN claim")
    need((ROOT / "nvcc_r1/strict_kernel_compile_link.stderr").read_bytes() == b"", "NVCC diagnostics")
    nvcc = receipts["nvcc_r1"]["commands"][1]["argv"]
    for flag in ("--gpu-architecture=sm_120", "-fmad=false", "--expt-relaxed-constexpr",
                 "-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror"):
        need(flag in nvcc, "missing strict CUDA flag " + flag)
    print(json.dumps({"status": "passed", "files": len(manifest["files"]),
                      "captures": 4, "host_o2_passed": True, "nvcc_link_passed": True,
                      "san_passed": False, "device_executed": False, "gcp_used": False,
                      "manifest_sha256": sha(ROOT / "manifest.json")}))


if __name__ == "__main__":
    main()
