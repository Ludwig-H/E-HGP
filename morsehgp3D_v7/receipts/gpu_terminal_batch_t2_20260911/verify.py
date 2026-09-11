#!/usr/bin/env python3
"""Portable reader: real-census seeded batch and separate high-K diagnostics."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURE_PIN = "1799fe59b841022ceccaf42b910b1e63798341d1eae7ac4f78c8a8f41c1d8393"
BATCH_PIN = "2c7df2e04132e306cd0a063efa93007548a0bc01736f818b5f447b8af1e0a4dd"
CENSUS_PIN = "f26e11a64ac44c6cc8f617b7e3b670b86fe75427a3aad5370705e8a98114f953"
FINAL_GATE = "6f85cb7a44b0acbcf4b0436e8ea3a2bd218937f5cd07baf570a476d3c54e3678"
FIXTURES = ("line12", "shell14", "spatial12")
REAL_COUNTS = ((525085,390,720,352752,2392,22024,0,0,0),
    (6326828,1242,2080,4658780,91638,851740,6,702,36),
    (5463812,1512,10200,3092416,7540,69020,0,738,378))
BATCH_COUNTS = (
    [(0,0,0)] * 9,
    [(0,0,0),(66,0,0),(126,12,0),(132,0,0),(72,0,0),(0,0,0),(24,0,0),(12,0,0),(12,0,0)],
    [(72,0,0),(192,36,36),(276,48,36),(336,36,0),(180,60,24),(228,36,24),(120,0,0),(12,0,0),(12,0,0)],
)
HIGH_COUNTS = (
    [(220,312,0,0,0),(66,45,0,0,0)],
    [(2002,2916,2,959,0),(1001,1680,0,571,0)],
    [(220,220,34,214,226),(66,36,5,83,19)],
)


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
    need(captured["scope"] == "HOST_STUB_real_census_seeded_batch_T2_and_separate_highK" and
         captured["device_executed"] is False and captured["cuda_compiled"] is False and
         captured["gcp_used"] is False and captured["public_status"] == "not_claimed", "CPU-only scope")
    names = ("o2_r1", "o2_r2", "o2_r3", "san_root_r1")
    need(set(captured["captures"]) == set(names), "all captures accounted")
    receipts, observations = {}, {}
    for name in names:
        prefix = "captures/" + name + "/"
        receipt = json.loads(read(prefix + "receipt.json"))
        need(sha(read(prefix + "receipt.json")) == captured["captures"][name], "receipt pin")
        need(receipt["status"] == ("failed" if name == "o2_r1" else "passed") and
             receipt["mode"] == ("san" if name.startswith("san") else "o2") and
             receipt["sources_stable"] is True and receipt["snapshot_stable"] is True and
             receipt["sources_before"] == receipt["sources_after"] and
             receipt["device_executed"] is False and receipt["GCP_used"] is False, "closed capture")
        for source, pin in receipt["sources_before"].items():
            need(sha(read(prefix + "source_snapshot/" + source)) == pin, "actual compiled snapshot")
        commands = receipt["commands"]
        expected_names = ["compiler", "compile"] if name == "o2_r1" else ["compiler", "compile", *FIXTURES, "unknown", "missing"]
        need([command["name"] for command in commands] == expected_names, "complete command sequence")
        for command in commands:
            expected = 2 if command["name"] in ("unknown", "missing") else 0
            actual = 1 if name == "o2_r1" and command["name"] == "compile" else expected
            need(command["exit_code"] == actual and command["expected_exit_code"] == expected, "exact exit status")
            for stream in ("stdout", "stderr"):
                need(sha(read(prefix + command["name"] + "." + stream)) == command[stream + "_sha256"], "log pin")
        argv = commands[1]["argv"]
        need(all(flag in argv for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-pthread", "-DMHGP7_FAKE_DEVICE")) and
             any(arg.endswith("source_snapshot/prototype/batch_t2_gate.cpp") for arg in argv), "strict HOST_STUB compilation")
        if name.startswith("san"):
            need(all(flag in argv for flag in ("-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie")), "SAN flags")
            need(b"env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1')" in
                 read(prefix + "source_snapshot/record.py"), "LSan enforced by pinned recorder")
        else:
            need("-O2" in argv, "O2 flags")
        if name == "o2_r1":
            need("binary_sha256" not in receipt and receipt["failure"] == "unexpected exit: compile" and
                 b"result type must be constructible from input type" in read(prefix + "compile.stderr") and
                 b"const auto scalar = build_full_ball_tower" in read(prefix + "source_snapshot/prototype/batch_t2_gate.cpp"),
                 "original const-result compile failure preserved")
        else:
            need(captured["binary_pins_no_ELF"][prefix + "gate"]["sha256"] == receipt["binary_sha256"], "ELF pin only")
            output = {}
            for index, fixture in enumerate(FIXTURES):
                rows = [json.loads(line) for line in read(prefix + fixture + ".stdout").decode().splitlines() if line.startswith('{"status":')]
                need(len(rows) == (2 if name == "o2_r2" else 3), "distinct physical JSON scopes")
                real, batch = rows[:2]
                keys = ("checks", "catalogue_rows", "cuts", "vertical_checks", "high_order_facets", "high_order_verticals", "shell12_rows", "q3_rows", "q4_rows")
                need(all(real.get(key) == value for key, value in zip(keys, REAL_COUNTS[index])) and
                     real["status"] == "passed" and real["scope"] == "bounded_real_census_FULL_K1_K10" and
                     real["clouds"] == 2 and real["orders"] == 180 and real["census_runs"] == 6 and
                     real["physical_tower_pairs"] == 16, "real census-to-tower observations")
                expected = [{"K": k, "direct_terminals": values[0], "Q": values[1], "H": values[2]}
                            for k, values in zip(range(2,11), BATCH_COUNTS[index])]
                need(batch["status"] == "passed" and batch["scope"] == "real_census_seeded_batch_T2" and
                     batch["device_executed"] is False and batch["batch_towers"] == 12 and
                     batch["batches"] == (0,84,108)[index] and batch["per_k"] == expected, "chronological batch per-K evidence")
                if name != "o2_r2":
                    high = rows[2]
                    expected = [{"K": k, "direct_terminals": values[0], "Q": values[1], "H": values[2],
                                 "reference_q3": values[3], "reference_q4": values[4]}
                                for k, values in zip((9,10), HIGH_COUNTS[index])]
                    need(high["status"] == "passed" and high["scope"] == "separate_all_highK_subsets_upper_cut" and
                         high["device_executed"] is False and high["failures_filtered"] == 0 and high["per_k"] == expected,
                         "separate predeclared high-K evidence")
                output[fixture] = rows
            observations[name] = output
        receipts[name] = receipt
    final = receipts["o2_r3"]["sources_before"]
    need(final == receipts["san_root_r1"]["sources_before"] and final["prototype/batch_t2_gate.cpp"] == FINAL_GATE,
         "complete final O2 SAN source closure")
    for name, receipt in receipts.items():
        need(set(receipt["sources_before"]) == set(final) and
             all(receipt["sources_before"][source] == pin for source, pin in final.items()
                 if source != "prototype/batch_t2_gate.cpp"), "historical source changes limited to judge")
    for source, pin in final.items():
        need(sha(read("current/" + source)) == pin, "current visible source")
    need(observations["o2_r3"] == observations["san_root_r1"] and captured["observations"] == observations, "O2 SAN exact observations")
    batch_raw = read("qualification/batch_manifest.json")
    census_raw = read("qualification/census_manifest.json")
    need(sha(batch_raw) == BATCH_PIN and sha(census_raw) == CENSUS_PIN, "published authority pins")
    batch_manifest, census_manifest = json.loads(batch_raw), json.loads(census_raw)
    base_raw = read("qualification/batch_r5_receipt.json")
    need(sha(base_raw) == batch_manifest["files"]["captures/root/o2_r5/receipt.json"]["sha256"], "published r5 receipt")
    imported = json.loads(read("current/import.json"))
    base = json.loads(base_raw)
    need(imported["base_receipt_sha256"] == sha(base_raw) and imported["base_sources"] == base["sources_before"] and
         imported["inherited_results"] is False and base["status"] == "passed", "explicit r5 import without inherited results")
    for source, pin in imported["base_sources"].items():
        if source.startswith("prototype/"):
            need(final[source] == pin == batch_manifest["files"]["sources/current/" + source]["sha256"], "complete same batch product closure")
    for name, pin in imported["relocated_test_pins"].items():
        original = read("qualification/" + name)
        need(sha(original) == pin == census_manifest["files"]["sources/current/morsehgp3D_v7/tests/" + name]["sha256"], "same published T2 judge/oracle")
        current = read("current/prototype/source/morsehgp3D_v7/tests/" + name)
        if name == "census_tower_gate.cpp":
            old = b"int main(int argc, char** argv) {"
            need(original.count(old) == 1 and current == original.replace(old, b"int mhgp7_private_census_main(int argc, char** argv) {"),
                 "T2 incorporation changes only entrypoint")
        else:
            need(current == original, "unchanged independent T2 oracle")
    return {"status": "passed", "captures": 4, "historical_failures": 1, "commands": 23,
            "retained_towers_each": 54, "batch_towers_each": 36, "additional_scalar_towers_each": 36,
            "paid_tower_constructions_each": 90, "chronological_direct_terminals_each": 1872,
            "chronological_Q_each": 228, "chronological_H_each": 120, "separate_highK_each": 3575,
            "separate_Q9_each": 3448, "separate_Q10_each": 1761, "separate_H9_each": 36, "separate_H10_each": 5,
            "geometry_executed_by_reader": False, "device_executed": False, "cuda_compiled": False, "gcp_used": False}


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
        need(sha(data) == entry["sha256"] and len(data) == entry["size"] and data[:4] != b"\x7fELF", "physical pin and no ELF")
        data.decode("utf-8")
    raw = (ROOT / "capture_manifest.json").read_bytes()
    need(sha(raw) == CAPTURE_PIN, "capture authority")
    captured = json.loads(raw)
    mapping = json.loads((ROOT / "storage_map.json").read_text())
    need(set(mapping) == set(captured["files"]), "logical coverage")
    for name, entry in mapping.items():
        safe(name)
        data = (ROOT / safe(entry["storage"])).read_bytes()
        need({key: entry[key] for key in ("sha256", "size")} == captured["files"][name] and
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
    report.update(logical_files=len(mapping), physical_files=len(actual), ELF_pins_only=len(captured["binary_pins_no_ELF"]))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
