#!/usr/bin/env python3
"""Read-only private receipt verifier. No assertion depends on Python mode."""
import argparse
import copy
import hashlib
import json
from pathlib import Path

FINAL_VECTORS = "692d2105b551d464df537085afd5e610579555447df37ceb6991cfa541cf0145"
HELPER = "165fd5964c9ea2394ca29747b236713d74528db4fd4149429f9080dd423cc75a"
OWNER = "ce94d8ee9a93fad16319487979ff31daf31236544e6539b5942a754591f13fda"
GATE = "2a51d67a5a3edc56c81ed29441d3dd6414a916c93fe699b1b3cd314472bcc3b9"
CAPTURES = ("o2_r1", "san_root_r1", "o2_r2", "san_root_r2", "host_r1", "nvcc_r1", "mutants_r1", "host_san_root_r1")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def validate(manifest, get_object):
    need(manifest["schema"] == "mhgp7_intruder_private_v1", "schema")
    need(manifest["captures"] == list(CAPTURES), "captures.complete")
    need(manifest["claims"] == dict(device_executed=False, GCP_used=False, public_status="not_claimed",
         full_terminal=False, performance_contract=False, profile="quantized_u16_input_only"), "claims.bounded")
    files = manifest["files"]
    for name, row in files.items():
        need(not name.startswith("/") and ".." not in Path(name).parts, "path")
        data = get_object(row["sha256"])
        need(len(data) == row["bytes"] and digest(data) == row["sha256"], "object.hash")
        need(not data.startswith(b"\x7fELF"), "object.no_ELF")

    def data(name):
        need(name in files, "file.required:" + name)
        return get_object(files[name]["sha256"])

    def document(name):
        return json.loads(data(name))

    need(files["current/intruder.cuh"]["sha256"] == HELPER, "helper.pin")
    need(files["current/owner.hpp"]["sha256"] == OWNER, "owner.pin")
    need(files["current/gate.cpp"]["sha256"] == GATE, "gate.pin")
    need(files["captures/o2_r2/export_vectors.stdout"]["sha256"] == FINAL_VECTORS, "vectors.pin")
    receipts = {}
    for capture in CAPTURES:
        prefix = "captures/" + capture + "/"
        receipt = document(prefix + "receipt.json")
        receipts[capture] = receipt
        need(receipt["status"] == ("failed" if capture == "san_root_r1" else "passed"), capture + ".status")
        need(receipt["sources_stable"] and receipt["sources_before"] == receipt["sources_after"], capture + ".stable")
        need(receipt["device_executed"] is False and receipt["GCP_used"] is False and
             receipt["public_status"] == "not_claimed", capture + ".scope")
        for name, pin in receipt["sources_before"].items():
            need(files[prefix + "source_snapshot/" + name]["sha256"] == pin, capture + ".source_pin")
        if capture not in ("o2_r1", "san_root_r1"):
            need(receipt["sources_before"]["intruder.cuh"] == HELPER and
                 receipt["sources_before"]["owner.hpp"] == OWNER and
                 receipt["sources_before"]["gate.cpp"] == GATE, capture + ".final_sources")
        for command in receipt["commands"]:
            for stream in ("stdout", "stderr"):
                need(files[prefix + command["name"] + "." + stream]["sha256"] == command[stream + "_sha256"],
                     capture + ".log_pin")

    def commands(capture, expected):
        rows = receipts[capture]["commands"]
        need([(row["name"], row["exit_code"]) for row in rows] == expected, capture + ".command_exits")
        return rows

    for capture in ("o2_r1", "o2_r2"):
        commands(capture, [("compile_gate", 0), ("run_gate", 0), ("invalid_args", 2), ("export_vectors", 0)])
    commands("san_root_r1", [("compile_gate", 1)])
    need(b"range-loop-construct" in data("captures/san_root_r1/compile_gate.stderr"), "failed_fixture.cause")
    commands("san_root_r2", [("compile_gate", 0), ("run_gate", 0), ("invalid_args", 2)])
    for capture in ("o2_r2", "san_root_r2"):
        row = document("captures/" + capture + "/run_gate.stderr")
        need(row == dict(status="passed", checks=6861, queries=596, found=372, missing=224, shell_points=34,
            range_hits=394, excluded_interior=1083, high_coefficients=3, nontrivial_rounding=94,
            axis_cases=175, box_cases=700, rejects=19, owner_rejects=13, maximum_stack=49, exported_cases=612),
            capture + ".nonvacuity_exact")
    legacy = document("captures/o2_r1/run_gate.stderr")
    need(legacy["checks"] == 6852 and legacy["exported_cases"] == 609 and legacy["rejects"] == 16,
         "legacy.separate")
    for capture in ("host_r1", "nvcc_r1", "host_san_root_r1"):
        compile_name = "compile_link" if capture == "nvcc_r1" else "compile"
        rows = commands(capture, [(compile_name, 0), ("nominal", 0), ("skip_write", 1),
                                  ("corrupt_word", 1), ("wrong_pin", 1), ("invalid_args", 2)])
        need(receipts[capture]["vectors_sha256"] == FINAL_VECTORS and
             files["captures/" + capture + "/vectors.txt"]["sha256"] == FINAL_VECTORS, capture + ".vectors")
        row = document("captures/" + capture + "/nominal.stdout")
        need(row["status"] == "passed" and row["backend"] == "host_stub" and row["cases"] == 612 and
             row["checked_words"] == 14688 and row["found"] == 372 and row["missing"] == 224 and row["rejected"] == 16 and
             row["peak49_cases"] > 0 and row["device_arch"] == 0 and row["device_executed"] is False and
             row["vectors_sha256"] == FINAL_VECTORS, capture + ".portable_nonvacuity")
        for name in ("skip_write", "corrupt_word"):
            need(data("captures/" + capture + "/" + name + ".stderr").startswith(b"FAIL backend.expected_word\n"),
                 capture + ".causal_transport")
        need(data("captures/" + capture + "/wrong_pin.stderr").startswith(b"FAIL vectors.sha256\n"), capture + ".causal_pin")
        if capture == "nvcc_r1":
            argv = rows[0]["argv"]
            need("--gpu-architecture=sm_120" in argv and "-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror" in argv and
                 "--host" in rows[1]["argv"], "nvcc.strict_compile_host_only")
            log = data("captures/nvcc_r1/compile_link.stderr")
            need(b"intruder_kernel" in log and b"336 bytes stack frame, 0 bytes spill stores, 0 bytes spill loads" in log and
                 b"Used 96 registers" in log, "nvcc.real_kernel_resources")
    for capture in ("san_root_r2", "host_san_root_r1"):
        need("-fsanitize=address,undefined" in receipts[capture]["commands"][0]["argv"], capture + ".sanitizers")
        recorder = data("captures/" + capture + "/source_snapshot/record.py")
        need(b'detect_leaks=1:halt_on_error=1' in recorder, capture + ".leaks_enabled_recorder")
    expected = []
    causal = {1: "axis.brute_integer_minimum", 2: "query.first_geometry_index_boost", 3: "query.first_geometry_index_boost",
              4: "query.first_geometry_index_boost", 5: "query.first_geometry_index_boost", 6: "query.admission"}
    for mutant, reason in causal.items():
        expected += [(f"compile_mutant{mutant}", 0), (f"run_mutant{mutant}", 1)]
        need(data(f"captures/mutants_r1/run_mutant{mutant}.stderr").startswith(("FAIL " + reason + " ").encode()),
             "mutant.causal:" + str(mutant))
    commands("mutants_r1", expected)
    binary_pins = document("post_capture_binary_pins.json")
    need(binary_pins["authority"] == "post_capture_read_only_hashes_not_new_execution", "binary_pin.authority")
    for capture, receipt in receipts.items():
        if "binary_sha256" in receipt:
            binary = "gate" if capture.startswith(("o2", "san")) else "device_gate"
            need(binary_pins["files"][capture + "/" + binary] == receipt["binary_sha256"], "binary_pin.closed_capture")
    return dict(status="passed", captures=len(CAPTURES), logical_files=len(files), queries=596,
                exported_cases=612, checked_portable_words=14688, cpp_mutants=6, device_executed=False)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("packet", nargs="?", default=str(Path(__file__).resolve().parent))
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    packet = Path(args.packet)
    raw = (packet / "manifest.json").read_bytes()
    manifest = json.loads(raw)
    get_object = lambda pin: (packet / "objects" / pin).read_bytes()
    result = validate(manifest, get_object)
    if args.selftest:
        rejected = 0
        for fault in range(6):
            altered = copy.deepcopy(manifest)
            overlay = {}
            if fault == 0:
                altered["claims"]["device_executed"] = True
            elif fault == 1:
                del altered["files"]["captures/mutants_r1/run_mutant6.stderr"]
            elif fault == 5:
                path = "captures/o2_r2/export_vectors.stdout"
                value = get_object(altered["files"][path]["sha256"]) + b" \n"
                pin = digest(value); overlay[pin] = value; altered["files"][path] = dict(sha256=pin, bytes=len(value))
            else:
                path = {2: "captures/mutants_r1/receipt.json", 3: "captures/o2_r2/run_gate.stderr",
                        4: "captures/san_root_r2/receipt.json"}[fault]
                value = json.loads(get_object(altered["files"][path]["sha256"]))
                if fault == 2:
                    value["commands"][-1]["exit_code"] = 0
                elif fault == 3:
                    value["queries"] = 0
                else:
                    value["sources_stable"] = False
                encoded = (json.dumps(value, sort_keys=True) + "\n").encode()
                pin = digest(encoded); overlay[pin] = encoded; altered["files"][path] = dict(sha256=pin, bytes=len(encoded))
                if fault == 3:
                    # Re-pin the log in its receipt too: the semantic floor, not just a hash, must reject.
                    receipt_path = "captures/o2_r2/receipt.json"
                    receipt = json.loads(get_object(altered["files"][receipt_path]["sha256"]))
                    receipt["commands"][1]["stderr_sha256"] = pin
                    encoded = (json.dumps(receipt, sort_keys=True) + "\n").encode()
                    pin = digest(encoded); overlay[pin] = encoded
                    altered["files"][receipt_path] = dict(sha256=pin, bytes=len(encoded))
            try:
                validate(altered, lambda pin: overlay[pin] if pin in overlay else get_object(pin))
            except (KeyError, ValueError):
                rejected += 1
        need(rejected == 6, "reader.selftests")
        result["reader_only_faults_rejected"] = rejected
    result["manifest_sha256"] = digest(raw)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
