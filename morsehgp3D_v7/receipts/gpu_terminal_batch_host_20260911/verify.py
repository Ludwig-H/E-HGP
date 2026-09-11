#!/usr/bin/env python3
"""Portable source-backed reader. No compiler, geometry, CUDA or infrastructure."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURE_PIN = "410e8e7fb5c59d9a9afdf418443360e1713a2d8b16248ff292e7330a8661926d"
CAPTURES = {
    "root/o2_r1": ("o2", "failed", [0, 1]),
    "root/o2_r2": ("o2", "passed", [0, 0, 0, 2, 2]),
    "root/o2_r3": ("o2", "failed", [0, 0, -6]),
    "root/o2_r4": ("o2", "passed", [0, 0, 0, 2, 2]),
    "root/o2_r5": ("o2", "passed", [0, 0, 0, 2, 2]),
    "root/san_root_r1": ("san", "passed", [0, 0, 0, 2, 2]),
    "traffic/o2_r1": ("o2", "passed", [0, 0, 0, 2, 2]),
}
CHANGING = {"prototype/batch_route.cuh", "prototype/batch_adapter.hpp", "prototype/batch_gate.cpp",
            "prototype/source/morsehgp3D_v7/src/forest/full_ball_tower.hpp"}
FINAL_PINS = {
    "prototype/batch_route.cuh": "a62bd1d5ab4bf80eaac5eff34db115ad7b6682222774d68c843cb0ccc20fc45f",
    "prototype/batch_adapter.hpp": "993786f352421eab8f183eb0675fe210e408dc5b3045b55af66a245a40f278ff",
    "prototype/batch_gate.cpp": "49b35f002d692a8ccc71fcff9ef37eba8335211ecaa337445f2fcb5bdc44b7e9",
    "prototype/source/morsehgp3D_v7/src/forest/full_ball_tower.hpp": "83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366",
    "prototype/terminal.cuh": "d73de05f81354b9c658d22ee040ac490f52625b8c8bdb523939aa9b5fd8e0860",
    "prototype/seed_owner.hpp": "f2ac463bce6eeaae59046d5a4198c6f4455a48b5ab5e6fad20ccbc456bb10485",
}


def need(good, reason):
    if not good:
        raise RuntimeError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    path = Path(name)
    need(not path.is_absolute() and ".." not in path.parts and str(path) not in ("", "."), "unsafe path")
    return path


def qualify(read, captured):
    need(captured["scope"] == "private_seeded_terminal_batch_HOST_STUB" and
         captured["device_executed"] is False and captured["gcp_used"] is False and
         captured["cuda_compiled"] is False and captured["public_status"] == "not_claimed", "scope")
    need(set(captured["captures"]) == set(CAPTURES), "complete capture list")
    receipts, summaries, command_count = {}, {}, 0
    for name, (mode, status, codes) in CAPTURES.items():
        prefix = "captures/" + name + "/"
        receipt = json.loads(read(prefix + "receipt.json"))
        need(sha(read(prefix + "receipt.json")) == captured["captures"][name], "receipt authority")
        need(receipt["mode"] == mode and receipt["status"] == status and receipt["sources_stable"] is True and
             receipt["snapshot_stable"] is True and receipt["sources_before"] == receipt["sources_after"] and
             receipt["device_executed"] is False and receipt["GCP_used"] is False, "closed capture")
        for source, pin in receipt["sources_before"].items():
            need(sha(read(prefix + "source_snapshot/" + source)) == pin, "compiled snapshot")
        commands = receipt["commands"]
        need([c["name"] for c in commands] == ["compiler", "compile", "selftest", "unknown", "missing"][:len(codes)],
             "command sequence")
        need([c["exit_code"] for c in commands] == codes, "exact exits")
        for command in commands:
            expected = 2 if command["name"] in ("unknown", "missing") else 0
            need(command["expected_exit_code"] == expected, "declared exits")
            need(command["started_ns"] <= command["ended_ns"], "command interval")
            for stream in ("stdout", "stderr"):
                need(sha(read(prefix + command["name"] + "." + stream)) == command[stream + "_sha256"], "stream pin")
            command_count += 1
        compile_args = commands[1]["argv"]
        need(all(flag in compile_args for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-pthread", "-DMHGP7_FAKE_DEVICE")), "strict explicit host compilation")
        need(any(arg.endswith("source_snapshot/prototype/batch_gate.cpp") for arg in compile_args), "compiled TU")
        if mode == "san":
            need(all(flag in compile_args for flag in ("-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                 "-fno-pie", "-no-pie")), "SAN compiler flags")
            recorder = read(prefix + "source_snapshot/record.py").decode()
            need("env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1')" in recorder,
                 "pinned recorder enforces LSan and UBSan")
        else:
            need("-O2" in compile_args, "O2 compiler flags")
        if name == "root/o2_r1":
            need("binary_sha256" not in receipt and receipt["failure"] == "unexpected exit: compile", "compile failure kept")
            error = read(prefix + "compile.stderr")
            need(b"-Werror=sign-compare" in error and b"FullBallStatus" in error and b"kOk" in error,
                 "r1 exact diagnostic classes")
        else:
            need(captured["binary_pins_no_ELF"][prefix + "gate"]["sha256"] == receipt["binary_sha256"], "ELF pin only")
        if name == "root/o2_r3":
            need(receipt["failure"] == "unexpected exit: selftest" and
                 b"mhgp7::full_ball_detail::Failure" in read(prefix + "selftest.stderr") and
                 b"catch (const std::overflow_error&)" in read(prefix + "source_snapshot/prototype/batch_gate.cpp"),
                 "r3 fixture catch failure kept")
        if status == "passed":
            data = json.loads(read(prefix + "selftest.stdout"))
            need(data["status"] == "passed" and data["device_executed"] is False and
                 data["physical_pairs"] == 64 and data["batches"] == 84 and data["requests"] == 176 and
                 data["seed_hits"] == 8, "forest and seed nonvacuity")
            summaries[name] = data
        receipts[name] = receipt
    final = receipts["root/o2_r5"]["sources_before"]
    need(final == receipts["root/san_root_r1"]["sources_before"], "complete O2 SAN final source closure")
    for source, pin in FINAL_PINS.items():
        need(final[source] == pin, "final semantic source pin")
    for source, pin in final.items():
        need(sha(read("current/" + source)) == pin, "visible current source")
    for name, receipt in receipts.items():
        before = receipt["sources_before"]
        if name.startswith("root/"):
            need(set(before) == set(final), "historical full source coverage")
            need(all(before[source] == pin for source, pin in final.items() if source not in CHANGING),
                 "historical unchanged source closure")
        else:
            for source, pin in final.items():
                if source.startswith("prototype/") and source not in CHANGING:
                    need(before[source] == pin, "isolated unchanged source closure")
            need(before["prototype/batch_route.cuh"] == final["prototype/batch_route.cuh"], "same optimized route")
            need(receipt["binary_stable"] is True and receipt["binary_after_sha256"] == receipt["binary_sha256"],
                 "isolated binary after pin")
    imported = json.loads(read("captures/traffic/o2_r1/source_snapshot/import.json"))
    old_raw = read("captures/root/o2_r2/receipt.json")
    need(imported["source_receipt_sha256"] == sha(old_raw) and imported["inherited_results"] is False and
         imported["source_pins"] == receipts["root/o2_r2"]["sources_before"] and
         read("captures/traffic/o2_r1/source_snapshot/source_receipt.json") == old_raw, "isolated explicit r2 import")
    need(summaries["root/o2_r2"]["checks"] == 27650 and summaries["root/o2_r2"]["transaction_rejections"] == 28 and
         "direct_terminals" not in summaries["root/o2_r2"], "r2 limited result")
    need(summaries["root/o2_r4"]["checks"] == 28003 and summaries["root/o2_r4"]["transaction_rejections"] == 34 and
         summaries["root/o2_r4"]["direct_terminals"] == 176 and
         summaries["root/o2_r4"]["synthetic_aggregate_rejections"] == 1, "r4 atomic direct-target result")
    need(summaries["root/o2_r5"] == summaries["root/san_root_r1"], "identical cumulative O2 SAN observations")
    for name in ("root/o2_r5", "root/san_root_r1", "traffic/o2_r1"):
        data = summaries[name]
        isolated = name.startswith("traffic/")
        need(data["checks"] == (27691 if isolated else 28044) and
             data["transaction_rejections"] == (35 if isolated else 41) and
             data["omissions_after_reuse"] == 3 and data["omissions_after_growth"] == 3 and
             data["initialization_rollbacks"] == 1 and data["reuse_H2D_bytes"] == 2496 and
             data["reuse_initialization_bytes"] == 2552 and data["request_size"] == 104 and
             data["wire_result_size"] == 232 and data["compact_target_size"] == 16 and
             data["former_full_result_size"] == 208, "exact traffic and rejection accounting")
        if isolated:
            need("direct_terminals" not in data and "synthetic_aggregate_rejections" not in data,
                 "isolated proof not promoted")
        else:
            need(data["direct_terminals"] == 176 and data["synthetic_aggregate_rejections"] == 1,
                 "cumulative direct target and overflow checks")
    need(captured["summaries"] == summaries, "summary authority")
    return {"status": "passed", "captures": 7, "historical_failures": 2, "commands": command_count,
            "cumulative_checks_each": 28044, "cumulative_forests_each": 64, "direct_terminals_each": 176,
            "transaction_rejections_each": 41, "synthetic_aggregate_rejections_each": 1,
            "isolated_traffic_checks": 27691, "geometry_executed_by_reader": False,
            "device_executed": False, "gcp_used": False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--extract", type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT / "manifest.json").read_text())
    actual = {p.relative_to(ROOT).as_posix() for p in ROOT.rglob("*") if p.is_file()}
    need(actual == set(manifest["files"]) | {"manifest.json"}, "physical coverage")
    need(all(not p.is_symlink() for p in ROOT.rglob("*")), "no symlink")
    for name, entry in manifest["files"].items():
        data = (ROOT / safe(name)).read_bytes()
        need(sha(data) == entry["sha256"] and len(data) == entry["size"], "physical pin")
        need(data[:4] != b"\x7fELF", "ELF forbidden")
        data.decode("utf-8")
    raw = (ROOT / "capture_manifest.json").read_bytes()
    need(sha(raw) == CAPTURE_PIN, "capture authority")
    captured = json.loads(raw)
    mapping = json.loads((ROOT / "storage_map.json").read_text())
    need(set(mapping) == set(captured["files"]), "logical coverage")
    for name, entry in mapping.items():
        safe(name)
        data = (ROOT / safe(entry["storage"])).read_bytes()
        need({key: entry[key] for key in ("size", "sha256")} == captured["files"][name] and
             sha(data) == entry["sha256"] and len(data) == entry["size"], "logical pin")

    def read(name):
        need(name in mapping, "missing logical file: " + name)
        return (ROOT / safe(mapping[name]["storage"])).read_bytes()

    report = qualify(read, captured)
    if args.extract:
        args.extract.mkdir(exist_ok=False)
        for name in mapping:
            path = args.extract / safe(name)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("xb") as output:
                output.write(read(name))
    report.update(physical_files=len(actual), logical_files=len(mapping),
                  ELF_pins_only=len(captured["binary_pins_no_ELF"]))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
