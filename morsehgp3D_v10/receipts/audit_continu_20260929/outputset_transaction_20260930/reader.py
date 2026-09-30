#!/usr/bin/env python3
"""Text-only closed archive judge: code/capture hashes, no native replay, no assert."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

PAYLOADS = (
    "capture.json", "source_pins.json", "protocol.md", "probe.cpp",
    "reader.py",
    "snapshot/core/cli_output.hpp", "snapshot/core/cli_options.hpp",
    "snapshot/core/status.hpp", "snapshot/core/types.hpp", "snapshot/core/reasons.def",
)
EXPECTED = {
    "success": ("ok", "none", "NEW_A", "NEW_B", 2, True, False, True),
    "no_commit": ("ok", "none", "OLD_A", "OLD_B", 0, False, False, True),
    "writer_bad_alloc": ("resource_exhausted", "memory_budget", "OLD_A", "OLD_B", 0, False, False, True),
    "rename2_eio": ("resource_exhausted", "output_unwritable", "NEW_A", "OLD_B", 2, True, False, False),
    "idempotent": ("ok", "none", "NEW_A", "NEW_B", 2, True, True, True),
}
ROW_KEYS = {"case", "status", "reason", "a", "b", "rename_calls", "temp_count",
            "fds_before", "fds_after", "did_commit", "second_commit_ok",
            "escaped", "contract_all_or_nothing", "observation_matches_expected"}

class Refusal(Exception):
    pass

def must(condition, message):
    if not condition:
        raise Refusal(message)

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

def check_digest(raw, wanted, where):
    must(type(wanted) is str and len(wanted) == 64, "invalid digest: " + where)
    must(digest(raw) == wanted, "hash mismatch: " + where)

def inventory(root):
    for item in (root,) + tuple(root.parents):
        must(not item.is_symlink(), "symlink root component")
    must(root.is_dir(), "archive is not a directory")
    files = set()
    dirs = set()
    for p in root.rglob("*"):
        must(not p.is_symlink(), "symlink archive entry")
        name = p.relative_to(root).as_posix()
        if p.is_dir():
            dirs.add(name)
        elif p.is_file():
            files.add(name)
        else:
            raise Refusal("nonregular archive entry")
    must(dirs == {"snapshot", "snapshot/core"}, "foreign directory inventory")
    must(files == set(PAYLOADS) | {"manifest.json"}, "foreign file inventory")

def judge_rows(rows):
    must(type(rows) is list and len(rows) == 5, "five native cases required")
    must([r.get("case") if type(r) is dict else None for r in rows] == list(EXPECTED),
         "native case order/inventory")
    for row in rows:
        case = row["case"]
        must(set(row) == ROW_KEYS, "row schema: " + case)
        for key in ("rename_calls", "temp_count", "fds_before", "fds_after"):
            must(type(row[key]) is int, "integer field: " + key)
        for key in ("did_commit", "second_commit_ok", "escaped",
                    "contract_all_or_nothing", "observation_matches_expected"):
            must(type(row[key]) is bool, "boolean field: " + key)
        observed = tuple(row[k] for k in ("status", "reason", "a", "b", "rename_calls",
                                         "did_commit", "second_commit_ok", "contract_all_or_nothing"))
        must(observed == EXPECTED[case], "causal result: " + case)
        must(row["temp_count"] == 0 and row["fds_before"] > 0 and
             row["fds_before"] == row["fds_after"], "resources: " + case)
        must(not row["escaped"] and row["observation_matches_expected"], "execution: " + case)
        both_old = row["a"] == "OLD_A" and row["b"] == "OLD_B"
        both_new = row["a"] == "NEW_A" and row["b"] == "NEW_B"
        computed = both_old if not row["did_commit"] or row["status"] != "ok" else both_new
        must(row["contract_all_or_nothing"] == computed, "recomputed whole set: " + case)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", required=True)
    parser.add_argument("--manifest-sha256", required=True)
    args = parser.parse_args()
    root = Path(args.archive).absolute()
    inventory(root)
    manifest_raw = (root / "manifest.json").read_bytes()
    check_digest(manifest_raw, args.manifest_sha256, "manifest.json external pin")
    manifest = json.loads(manifest_raw)
    must(type(manifest) is dict and set(manifest) == {"schema", "sha256"} and
         manifest["schema"] == "outputset_transaction_text_v2", "manifest schema")
    hashes = manifest["sha256"]
    must(type(hashes) is dict and set(hashes) == set(PAYLOADS), "manifest payload inventory")
    payloads = {p: (root / p).read_bytes() for p in PAYLOADS}
    for p in PAYLOADS:
        check_digest(payloads[p], hashes[p], p)
    # All hashes have now passed. Only now load the source/capture records.
    pins = json.loads(payloads["source_pins.json"])
    must(pins["working_head"] == "6d2d3bc5d1c34deca95f3ba6cc7929179517680c", "HEAD pin")
    must(pins["sha256_before"] == pins["sha256_after"], "source changed during capture")
    must(set(pins["sha256_before"]) == {"cli_output.hpp", "cli_options.hpp", "status.hpp",
                                      "types.hpp", "reasons.def"}, "source pins inventory")
    for name, sha in pins["sha256_before"].items():
        check_digest(payloads["snapshot/core/" + name], sha, "source snapshot " + name)
    capture = json.loads(payloads["capture.json"])
    meta = capture["archive_metadata"]
    must(meta["native_reexecuted_by_reader"] is False, "reader scope metadata")
    must(meta["omitted_binary_sha256"] == "e859cfe91830ee2f300dba5ec229a8b8deaf5a3612ec8bc975460d12b684fc03",
         "historical binary digest metadata")
    must(meta["original_manifest_sha256"] == "ee24d8178f5f0620b8f8a7d47137e12be45a762cec4aa14e7ebf8ca879f28462",
         "original archive metadata")
    compiler = meta["compiler_identification"]
    must(compiler["argv"] == ["g++", "--version"] and compiler["exit_code"] == 0 and
         compiler["output"].startswith("g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0\n"),
         "GNU version metadata")
    must(type(capture["compile"]["exit_code"]) is int and capture["compile"]["exit_code"] == 0,
         "compile did not finish successfully")
    must(capture["compile"]["argv"] == ["timeout", "10s", "g++", "-std=c++20", "-O0",
         "-Wall", "-Wextra", "-Werror", "-Isnapshot", "probe.cpp", "-Wl,--wrap=rename", "-o", "probe"],
         "compile binding")
    must(capture["compile"]["output"] == "", "compiler diagnostics")
    must(type(capture["run"]["exit_code"]) is int and capture["run"]["exit_code"] == 0, "native run")
    must(type(capture["run"]["argv"]) is list and len(capture["run"]["argv"]) == 2 and
         capture["run"]["argv"][0] == "./probe", "run binding")
    rows = [json.loads(line) for line in capture["run"]["output"].splitlines()]
    judge_rows(rows)
    # Negative control 1: altered bytes cannot pass the original payload digest.
    rejected_hash = False
    try:
        check_digest(payloads["snapshot/core/cli_output.hpp"] + b" ", hashes["snapshot/core/cli_output.hpp"],
                     "snapshot/core/cli_output.hpp")
    except Refusal as e:
        rejected_hash = str(e) == "hash mismatch: snapshot/core/cli_output.hpp"
    must(rejected_hash, "hash mutant survived")
    # Negative control 2: rehashed/fabricated claim of transaction success cannot pass the causal judge.
    mutant = copy.deepcopy(rows)
    mutant[3]["contract_all_or_nothing"] = True
    rejected_causal = False
    try:
        judge_rows(mutant)
    except Refusal as e:
        rejected_causal = str(e) == "causal result: rename2_eio"
    must(rejected_causal, "causal mutant survived")
    inventory(root)
    check_digest((root / "manifest.json").read_bytes(), args.manifest_sha256, "manifest after")
    for p in PAYLOADS:
        check_digest((root / p).read_bytes(), hashes[p], "after " + p)
    print("outputset_transaction_archive_ok cases=5 defect_rename2_eio=1 reader_mutants=2/2 native_reexecuted=0")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (Refusal, OSError, ValueError, KeyError, TypeError) as e:
        print("outputset_transaction_audit_refused: " + str(e), file=sys.stderr)
        raise SystemExit(1)
