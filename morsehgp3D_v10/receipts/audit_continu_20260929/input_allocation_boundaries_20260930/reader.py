#!/usr/bin/env python3
"""Closed text/source/capture reader. Does not execute or verify the omitted binary."""
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import sys

PAYLOADS = {
    "snapshot/core/cli_options.hpp", "snapshot/cloud/u32le_input.hpp",
    "snapshot/core/status.hpp", "snapshot/core/types.hpp", "snapshot/core/reasons.def",
    "callers/cli/mhgp10_catalogue.cpp", "callers/cli/mhgp10_tower.cpp",
    "callers/cli/mhgp10_cluster.cpp", "callers/tests/head/mreach_cluster.cpp",
    "probe.cpp", "source_pins.json", "capture.json", "PROTOCOL.md", "reader.py",
}
ROW_KEYS = {
    "case", "inject", "fault_hit", "refused_size", "returned", "bad_alloc_escaped",
    "other_escaped", "value_ok", "status", "reason", "fds_before", "fds_after",
    "files_before", "files_after", "fixtures_before", "fixtures_after",
}

def need(ok, message):
    if not ok:
        raise ValueError(message)

def sha(blob):
    return hashlib.sha256(blob).hexdigest()

def strict_json(blob):
    def pairs(items):
        out = {}
        for k, v in items:
            need(k not in out, "duplicate JSON key")
            out[k] = v
        return out
    def bad(value):
        raise ValueError("nonfinite JSON constant " + value)
    return json.loads(blob.decode("utf-8"), object_pairs_hook=pairs, parse_constant=bad)

def inventory(root):
    need(root.is_absolute(), "absolute archive path required")
    need(all(not p.is_symlink() for p in (root, *root.parents)), "archive path symlink")
    need(not root.is_symlink() and root.is_dir(), "archive directory/symlink")
    files = set()
    seen_dirs = set()
    expected_dirs = {str(Path(p).parent) for p in PAYLOADS if str(Path(p).parent) != "."}
    expected_dirs |= {str(p) for name in PAYLOADS for p in Path(name).parents if str(p) != "."}
    for current, dirs, names in os.walk(root, followlinks=False):
        for name in dirs:
            need(not (Path(current) / name).is_symlink(), "directory symlink")
            seen_dirs.add((Path(current) / name).relative_to(root).as_posix())
        for name in names:
            p = Path(current) / name
            need(not p.is_symlink() and p.is_file(), "payload symlink/nonfile")
            files.add(p.relative_to(root).as_posix())
    need(seen_dirs == expected_dirs, "closed directory inventory")
    return files

def load_closed(root, external_sha):
    need(re.fullmatch(r"[0-9a-f]{64}", external_sha) is not None, "external SHA syntax")
    need(inventory(root) == PAYLOADS | {"manifest.json"}, "closed inventory")
    raw = (root / "manifest.json").read_bytes()
    need(sha(raw) == external_sha, "external manifest SHA mismatch")
    manifest = strict_json(raw)
    need(set(manifest) == {"schema", "files"}, "manifest keys")
    need(manifest["schema"] == "input_allocation_manifest_v1", "manifest schema")
    files = manifest["files"]
    need(type(files) is dict and set(files) == PAYLOADS, "manifest payload whitelist")
    blobs = {}
    for name in sorted(PAYLOADS):
        expected = files[name]
        need(type(expected) is dict and set(expected) == {"sha256", "size"}, "manifest entry keys")
        need(type(expected["size"]) is int and expected["size"] >= 0, "manifest byte size")
        need(type(expected["sha256"]) is str and re.fullmatch(r"[0-9a-f]{64}", expected["sha256"]) is not None,
             "manifest payload SHA syntax")
        blob = (root / name).read_bytes()
        need(len(blob) == expected["size"] and sha(blob) == expected["sha256"], "payload hash mismatch " + name)
        blob.decode("utf-8")
        blobs[name] = blob
    return manifest, blobs

def recheck_memory_hashes(manifest, blobs):
    for name in PAYLOADS:
        need(sha(blobs[name]) == manifest["files"][name]["sha256"], "payload hash mismatch " + name)

def check_capture(c):
    need(c["schema"] == "input_allocation_boundary_text_v1", "capture schema")
    for stage in ("compile", "run"):
        need(type(c[stage]["exit_code"]) is int and type(c[stage]["timeout_seconds"]) is int,
             "command integer types")
    need(c["compile"]["exit_code"] == 0 and c["run"]["exit_code"] == 0, "command exit")
    need(c["compile"]["timeout_seconds"] == 10 and c["run"]["timeout_seconds"] == 10, "command bound")
    need(c["compile"]["output"] == "", "nonempty compiler output")
    need(c["native_binary"]["archived"] is False, "binary must not be archived")
    need(re.fullmatch(r"[0-9a-f]{64}", c["native_binary"]["sha256"]) is not None, "historical binary SHA")
    need(c["compiler"]["toolchain_prehashed"] is False and "g++" in c["compiler"]["version_output"],
         "compiler provenance scope")
    rows = c["run"]["rows"]
    need(type(rows) is list and len(rows) == 8, "case count")
    expected = [(name, inject) for inject in (False, True) for name in ("reader", "real", "list", "configs")]
    need([(r.get("case"), r.get("inject")) for r in rows] == expected, "case identity/order")
    stream_rows = [strict_json(x.encode("utf-8")) for x in c["run"]["output"].splitlines()]
    need(stream_rows == rows, "capture stream/rows mismatch")
    for r in rows:
        need(set(r) == ROW_KEYS, "row keys")
        for key in ("inject", "fault_hit", "returned", "bad_alloc_escaped", "other_escaped", "value_ok",
                    "fixtures_before", "fixtures_after"):
            need(type(r[key]) is bool, "row boolean type " + key)
        for key in ("refused_size", "fds_before", "fds_after", "files_before", "files_after"):
            need(type(r[key]) is int, "row integer type " + key)
        need(r["fds_before"] >= 0 and r["fds_before"] == r["fds_after"], "descriptor leak")
        need(r["files_before"] == r["files_after"] == 2 and r["fixtures_before"] and r["fixtures_after"],
             "fixture preservation")
        need(not r["other_escaped"], "unknown exception")
        need(r["status"] == "ok" and r["reason"] == "none", "recorded default Outcome fields")
        if r["inject"]:
            need(r["fault_hit"] and r["refused_size"] > 0 and r["bad_alloc_escaped"] and not r["returned"]
                 and not r["value_ok"], "missing escaped allocation defect")
        else:
            need(not r["fault_hit"] and r["refused_size"] == 0 and not r["bad_alloc_escaped"] and r["returned"]
                 and r["value_ok"], "positive control failed")
    need(type(c["summary"]) is dict and all(type(v) is int for v in c["summary"].values()), "summary integer types")
    need(c["summary"] == {
        "positive_controls":4, "injected_cases":4, "bad_alloc_escapes":4, "returned_memory_budget":0,
        "descriptor_leaks":0, "fixture_changes":0}, "false capture summary")
    f = c["fixtures"]
    need(f["inventory"] == ["cloud-with-long-name-for-filesystem.u32le", "sentinel"], "fixture inventory")
    need(f["contents"] == {"cloud_u32le":[1,2,3], "sentinel_ascii":"unchanged sentinel payload\n"},
         "fixture contents")
    cloud_bytes = b"".join(x.to_bytes(4, "little") for x in (1,2,3))
    need(f["cloud_sha256"] == sha(cloud_bytes) and f["sentinel_sha256"] == sha(b"unchanged sentinel payload\n"),
         "fixture final hashes")

def check_pins(pins, blobs):
    need(pins["schema"] == "input_allocation_source_pins_v1", "pins schema")
    rows = pins["sources"]
    need(type(rows) is list and len(rows) == 9, "source pin count")
    paths = [r["snapshot"] for r in rows]
    need(len(set(paths)) == 9 and set(paths) == {p for p in PAYLOADS if p.startswith(("snapshot/", "callers/"))},
         "pin whitelist")
    for r in rows:
        need(r["sha256_before"] == r["sha256_after"] == sha(blobs[r["snapshot"]]), "source pin mismatch")
        need(r["source"].startswith("/workspaces/E-HGP/build/v10-integration-r2/src/morsehgp3D_v10/"),
             "source provenance path")
        need(type(r["compiled"]) is bool and r["compiled"] == r["snapshot"].startswith("snapshot/"),
             "caller/compiled scope")
    need(sum(r["compiled"] for r in rows) == 5, "compiled local transdependencies")

def must_reject(fn, token):
    try:
        fn()
    except ValueError as e:
        need(token in str(e), "wrong mutation failure cause")
        return
    raise ValueError("reader mutation survived")

def main():
    need(len(sys.argv) == 3, "usage: reader.py ARCHIVE EXTERNAL_MANIFEST_SHA256")
    root = Path(sys.argv[1])
    manifest, blobs = load_closed(root, sys.argv[2])
    pins = strict_json(blobs["source_pins.json"])
    cap = strict_json(blobs["capture.json"])
    check_pins(pins, blobs)
    check_capture(cap)
    altered = dict(blobs)
    altered["snapshot/core/cli_options.hpp"] = altered["snapshot/core/cli_options.hpp"] + b" "
    must_reject(lambda: recheck_memory_hashes(manifest, altered), "payload hash mismatch")
    false_summary = copy.deepcopy(cap)
    false_summary["summary"]["returned_memory_budget"] = 4
    must_reject(lambda: check_capture(false_summary), "false capture summary")
    final_manifest, final_blobs = load_closed(root, sys.argv[2])
    need(final_manifest == manifest and final_blobs == blobs, "after hashes changed")
    print("input_allocation_archive_ok cases=8 escaped_bad_alloc=4 reader_mutants=2/2 native_reexecuted=0")

if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, TypeError, UnicodeError, json.JSONDecodeError) as e:
        print("input_allocation_archive_rejected: " + str(e), file=sys.stderr)
        sys.exit(1)
