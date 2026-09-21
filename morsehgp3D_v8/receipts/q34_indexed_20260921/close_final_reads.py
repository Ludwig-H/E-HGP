#!/usr/bin/env python3
"""Close tranche32 receipts by reading them; execute no native benchmark/build."""
import base64
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q34_indexed_checks as checks
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

NEW = (
    ("mutations_final/compiled_foznl6jv", "mutation"),
    ("mutations_final/compiled_yh3lps69", "mutation"),
    ("lidar/lidar_86twby55", "lidar"),
    ("separation/lidar_2w21ewdx", "lidar"),
)
LARGE = ("lidar/lidar_2x8pm0nw", "lidar/lidar_86twby55", "separation/lidar_2w21ewdx",
         "lidar/lidar_tgo9mbax", "lidar/lidar_b7fo1a0i")
SMALL = ("qualifications/smoke_2x8dvljr", "qualifications/smoke_nc5osoja", "qualifications/smoke_qbh9yxw5")


def add_hashes(target, values):
    for name, value in values.items():
        require(name not in target or target[name] == value, "conflicting captured pin: " + name)
        target[name] = value


def inherited_readback(name, sources, inputs, artifacts):
    path = HERE / name
    value = read_json(path)
    require(value["status"] == "passed" and value["error"] is None and value["closing_errors"] == [] and
            value["native_reexecutions"] == 0 and value["normal_optimized_identical"] is True,
            "inherited readback is not closed")
    require(value["source_sha256"] == sources == value["source_sha256_after"], "readback sources changed")
    for key in ("input_sha256", "artifact_sha256"):
        require(value[key] == value[key + "_after"] and pins(value[key]) == value[key], "readback inputs changed")
    for record in value["records"]:
        require(record["status"] == "passed", "previous reader failed")
        expected = record.get("expected_reader_success", True)
        require(type(record["exit_code"]) is int and (record["exit_code"] == 0) is expected,
                "previous reader exit differs")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"], validate=True).decode(errors="replace") == record[stream],
                    "previous reader raw/decoded text differs")
    inputs.update(value["input_sha256"])
    inputs.add(str(path.relative_to(ROOT)))
    add_hashes(artifacts, value["artifact_sha256"])
    return dict(path=name, sha256=digest(path), commands=value["commands"], status="passed")


def main():
    output = HERE / "FINAL_READBACK.json"
    require(not output.exists(), "refuse overwriting final readback")
    sources = pins(checks.SOURCES)
    inputs = {str(Path(__file__).resolve().relative_to(ROOT))}
    artifacts, inventory, rows, smoke_rows = {}, [], [], []
    reused = [inherited_readback(name, sources, inputs, artifacts)
              for name in ("READBACK.json", "DEFAULT_COMPATIBILITY_READBACK.json")]
    # The eight targeted measures already have eight normal/-O live/historical
    # reads; validate those results and their closed input hashes, not rerun them.
    benchmark_path = HERE / "BENCH_8K_READBACKS.json"
    benchmark = read_json(benchmark_path)
    require(len(benchmark["readbacks"]) == 8, "targeted readback count differs")
    grouped = {}
    for record in benchmark["readbacks"]:
        require(type(record["exit_code"]) is int and record["exit_code"] == 0, "targeted reader failed")
        result = parse_result(record["output"].encode())
        require(result["status"] == "passed" and result["measurements"] == 4, "targeted reader scope differs")
        grouped.setdefault(result["path"], []).append(result)
    require(len(grouped) == 2 and all(len(items) == 4 and all(x == items[0] for x in items) for items in grouped.values()),
            "targeted normal/-O/live results differ")
    combined = benchmark["combined_analysis"]
    require(type(combined["exit_code"]) is int and combined["exit_code"] == 0, "targeted combined analysis failed")
    combined_row = parse_result(combined["output"].encode())
    require(combined_row["status"] == "passed" and combined_row["records"] == 8 and
            combined_row["all_payload_digests_equal"] is True and combined_row["same_remaining_geometry"] is True and
            combined_row["pins_before"] == combined_row["pins_after"] == pins(combined_row["pins_before"]),
            "targeted combined analysis closure differs")
    inputs.add(str(benchmark_path.relative_to(ROOT)))
    inputs.update(combined_row["pins_before"])
    reused.append(dict(path=benchmark_path.name, sha256=digest(benchmark_path), commands=8, status="passed"))
    # All capture manifests in this receipt tree are conserved, including failed
    # smokes and the pre-reader-fix snapshots. They are checked, not promoted.
    for manifest_path in sorted(HERE.rglob("MANIFEST.json")):
        path = manifest_path.parent
        relative = str(path.relative_to(HERE))
        manifest, completion = read_json(manifest_path), read_json(path / "COMPLETION.json")
        require(completion["manifest_sha256"] == digest(manifest_path) and completion["closing_errors"] == [],
                "manifest/closure differs: " + relative)
        for key in ("source_sha256", "artifact_sha256", "input_sha256"):
            if key in manifest:
                require(manifest[key] == completion[key + "_after"], "capture pin closure differs: " + relative)
        differences = {name: dict(captured=value, current=sources[name])
                       for name, value in manifest["source_sha256"].items() if value != sources[name]}
        require(set(manifest["source_sha256"]) == checks.SOURCES, "capture source inventory differs")
        if differences:
            checker = "morsehgp3D_v8/bench/run_q34_indexed_checks.py"
            require(set(differences) == {checker} and digest(HERE / "preflight/before_empty_calls_fix/run_q34_indexed_checks.py") ==
                    manifest["source_sha256"][checker], "unarchived source change")
        add_hashes(artifacts, manifest["artifact_sha256"])
        inputs.update(manifest.get("input_sha256", {}))
        inputs.update(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file())
        if "compiler" in manifest:
            require(digest(Path(manifest["compiler"])) == manifest["compiler_sha256"] == completion["compiler_sha256_after"],
                    "mutation compiler changed")
            inputs.add(manifest["compiler"])
            for evidence in completion["evidence"]:
                for field in ("binary", "object"):
                    add_hashes(artifacts, {evidence[field]: evidence[field + "_sha256"]})
        for item in completion["records"]:
            require(digest(path / item["path"]) == item["sha256"], "record changed: " + relative)
            record = read_json(path / item["path"])
            if relative in LARGE or relative in SMALL:
                if record["kind"] == "probe":
                    (rows if relative in LARGE else smoke_rows).append(record["row"])
        if completion.get("xml_sha256") is not None:
            require(digest(path / "result.xml") == completion["xml_sha256"], "regression XML changed")
            require(len(checks.validate_xml(path / "result.xml")) == 94, "regression test inventory differs")
        inventory.append(dict(path=relative, status=completion["status"], records=len(completion["records"]),
            source_differences=differences, manifest_sha256=digest(manifest_path),
            completion_sha256=digest(path / "COMPLETION.json")))
    require(len(rows) == 29 and len(smoke_rows) == 36, "measurement coverage count differs")
    checks.paired(rows)
    checks.paired(smoke_rows)
    require(pins(artifacts) == artifacts, "captured executable/object/cache changed")
    files = pins(inputs)
    commands = []
    for relative, kind in NEW:
        reader = ROOT / ("morsehgp3D_v8/tests/q34_indexed_mutations.py" if kind == "mutation"
                         else "morsehgp3D_v8/bench/run_q34_indexed_lidar.py")
        commands.append([str(reader), "read", str(HERE / relative), "--check-live", *(["--compact"] if kind == "lidar" else [])])
    records, summaries, error, status, started = [], [], None, "failed", utc_stamp()
    previous = {s: signal.signal(s, on_signal) for s in (signal.SIGINT, signal.SIGTERM)}
    try:
        for optimized in (False, True):
            values = []
            for command in commands:
                argv = [sys.executable, "-B", *(["-O"] if optimized else []), *command]
                record = dict(command=argv, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                              exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(argv, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0 and not record["stderr"], "final reader command failed")
                    result = parse_result(record["stdout"].encode())
                    require(result["status"] == "passed", "final reader result failed")
                    values.append(result)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
            summaries.append(values)
        require(summaries[0] == summaries[1], "final normal/-O read results differ")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, function):
            try:
                return function()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        result = dict(schema="mhgp8_q34_indexed_final_readback_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records,
            results=summaries[0] if summaries else [], reused_readbacks=reused, captures=inventory,
            native_reexecutions=0, large_measurements=29, small_measurements=36, compatibility_pairs=8,
            release_ctests=94, sanitizer_native_gates=3, sanitizer_probe_measurements=12,
            final_compiled_mutants=6, historical_captures_promoted=False,
            paired_large_payload_digests=status == "passed", paired_small_full_records=status == "passed",
            normal_optimized_identical=len(summaries) == 2 and summaries[0] == summaries[1],
            source_sha256=sources, source_sha256_after=close("sources", lambda: pins(checks.SOURCES)),
            input_sha256=files, input_sha256_after=close("inputs", lambda: pins(inputs)),
            artifact_sha256=artifacts, artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            closing_errors=errors, full_contract_qualified=False, universal_subquadratic_claim=False, gcp_used=False)
        if errors or any(result[k] != result[k + "_after"] for k in ("source_sha256", "input_sha256", "artifact_sha256")):
            result["status"] = "failed"
            result["error"] = error or "final readback closure changed"
        write_json(output, result)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(status=result["status"], error=result["error"], commands=len(records),
            sources=len(sources), inputs=len(files), artifacts=len(artifacts), large_measurements=29, small_measurements=36)))
    require(result["status"] == "passed", "final readback failed")


if __name__ == "__main__":
    main()
