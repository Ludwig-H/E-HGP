#!/usr/bin/env python3
"""Bounded audit-only s=8/10/12 sweep on one pinned K5 LiDAR quarter."""

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


HERE = Path(__file__).resolve().parent
AUDITS = HERE.parent
INPUT = AUDITS / "s4a_cpu_scene02_physical_panel_20260924/inputs/quarter_quarter_x_nonneg_y_nonneg.u32le"
SOURCE_RECEIPT = AUDITS / "b_s2_trace_k5_20260924/RESULT.json"
EXPECTED_INPUT_SHA = "33630aea9492d3b059001e10c74ba30bec143dd7a1a0be6b25f8701b2f2a5e8f"
EXPECTED_BINARY_SHA = "1afdf9948624949f6b593db055ca95e3b1454a5ca168af6dacfd6dcf0b0aa3b1"
EXPECTED_DIGESTS = ("e0c1c44b3d006ca5", "41a027f010f53600")
EXPECTED = {8: (37459, 46218, 39274), 10: (42686, 40728, 35804), 12: (47158, 37843, 33782)}
LEVERS = (
    "atlas_saturate_deep=1", "q3_leaf_census=1", "q34_dead_lanes=1",
    "q34_witness_cache=1", "q34_dead_core=1", "tower_meb_proposal=1",
    "q34_jobs_by_mass=1", "q34_fine_jobs=1", "tower_overlap_static=1",
    "q2_jobs_by_mass=1", "q34_batch_filter=1", "q34_batch_certificates=1",
    "q34_gpu_filter=0", "q34_gpu_certificates=0", "q34_batch_q3=0",
    "q34_gpu_q3=0", "q34_batch_q4=0",
)


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def check(condition, message):
    if not condition:
        raise ValueError(message)


def validate(s, result):
    check(result["schema"] == "mhgp9_tower_probe_v26" and
          result["status"] == "complete_relative" and
          result["reason"] == "complete_relative_to_cross_checked_catalogue",
          "status/schema")
    check(result["input"] == {"format": "u32le", "grid": "1mm", "sites": 1288,
                              "hash": "9e91c2c06b43a6ff"}, "input")
    options = result["options"]
    check((options["K"], options["s"], options["workers"],
           options["tower_static_threads"], options["run_tower"]) ==
          (5, s, 8, 8, True), "options")
    check(all(options["levers"][name] == (value == "1")
              for name, value in (lever.split("=") for lever in LEVERS)), "levers")
    rectangles, pairs, q2_pairs = EXPECTED[s]
    batch, ledger, gen = result["q34_batch"], result["ledger"], result["generator"]
    check((batch["rectangles"], ledger["expanded_pairs"], gen["q2_candidate_pairs"]) ==
          (rectangles, pairs, q2_pairs), "s-dependent work")
    check((batch["backend"], batch["certificate_backend"], batch["survivors"],
           batch["deferred"], ledger["core_sites"], ledger["core_builds"],
           ledger["dead_core_loads"], ledger["core_closed_edges"],
           gen["q2_accepted_pairs"], gen["q34_cover_builds"],
           gen["q3_emitted"], gen["q4_emitted"]) ==
          ("cpu", "cpu", 27099, 0, 298205, 27099, 27099, 7020,
           12020, 20079, 14386, 2348), "common work")
    check((result["catalogue_digest"], result["tower_digest"]) == EXPECTED_DIGESTS,
          "exact output digests")
    return {"s": s, "rectangles": rectangles, "expanded_pairs": pairs,
            "q2_candidate_pairs": q2_pairs, "survivors": batch["survivors"],
            "core_sites": ledger["core_sites"], "chain_total_ms": result["times_ms"]["chain_total"],
            "q34_ms": result["times_ms"]["q34"], "tower_ms": result["times_ms"]["tower"]}


def command(binary, s):
    return [str(binary), str(INPUT), "5", "8", f"--s={s}", "--static=8",
            "--grid=1mm", "--catalogue-digest", *(f"--lever={x}" for x in LEVERS)]


def run(binary):
    receipt = json.loads(SOURCE_RECEIPT.read_text())
    check(sha(INPUT) == EXPECTED_INPUT_SHA, "pinned input SHA")
    check(receipt["local_product_binary_sha256"] == EXPECTED_BINARY_SHA and
          sha(binary) == EXPECTED_BINARY_SHA, "pinned binary SHA")
    paths = [HERE / f"s{s}.stdout.json" for s in EXPECTED]
    check(not any(path.exists() for path in paths) and not (HERE / "MANIFEST.json").exists(),
          "outputs already exist")
    manifest = {"schema": "mhgp9_b_s_sweep_k5_quarter_v1", "scope": "audit_only_one_1288_site_quarter_cpu",
                "input_sha256": sha(INPUT), "binary_sha256": sha(binary),
                "script_sha256": sha(HERE / "sweep.py"), "runs": []}
    for s, path in zip(EXPECTED, paths):
        argv = command(binary, s)
        completed = subprocess.run(argv, cwd=HERE.parents[2], capture_output=True, check=False)
        check(completed.returncode == 0 and not completed.stderr,
              f"product failed s={s}: {completed.returncode} {completed.stderr[:200]!r}")
        result = json.loads(completed.stdout)
        observed = validate(s, result)
        path.write_bytes(completed.stdout)
        manifest["runs"].append({"s": s, "argv": argv, "stdout_file": path.name,
                                 "stdout_sha256": sha(path), "observed": observed})
    (HERE / "MANIFEST.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "captured", "runs": manifest["runs"]}, sort_keys=True))


def verify(check_binary):
    manifest = json.loads((HERE / "MANIFEST.json").read_text())
    check(manifest["schema"] == "mhgp9_b_s_sweep_k5_quarter_v1" and
          manifest["scope"] == "audit_only_one_1288_site_quarter_cpu" and
          manifest["input_sha256"] == EXPECTED_INPUT_SHA and
          manifest["binary_sha256"] == EXPECTED_BINARY_SHA and
          manifest["script_sha256"] == sha(HERE / "sweep.py") and
          sha(INPUT) == EXPECTED_INPUT_SHA, "manifest identity")
    check(len(manifest["runs"]) == 3 and [entry["s"] for entry in manifest["runs"]] ==
          [8, 10, 12], "run list")
    for entry in manifest["runs"]:
        s = entry["s"]
        path = HERE / f"s{s}.stdout.json"
        check(entry["stdout_file"] == path.name and entry["stdout_sha256"] == sha(path),
              "stdout SHA")
        check(entry["observed"] == validate(s, json.loads(path.read_text())), "observed record")
        check(entry["argv"] == command(Path(entry["argv"][0]), s), "command argv")
        if check_binary:
            check(sha(Path(entry["argv"][0])) == EXPECTED_BINARY_SHA, "live binary SHA")
    print(json.dumps({"status": "verified", "s": [8, 10, 12],
                      "binary": "live" if check_binary else "archived_sha_only",
                      "catalogue_digest": EXPECTED_DIGESTS[0],
                      "tower_digest": EXPECTED_DIGESTS[1]}, sort_keys=True))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("run", "verify"))
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--check-binary", action="store_true")
    args = parser.parse_args()
    if args.mode == "run":
        binary = args.binary or Path(json.loads(SOURCE_RECEIPT.read_text())["local_product_binary_path"])
        run(binary)
    else:
        verify(args.check_binary)
