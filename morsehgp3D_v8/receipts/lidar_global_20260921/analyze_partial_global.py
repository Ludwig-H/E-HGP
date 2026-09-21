#!/usr/bin/env python3
"""Audit the completed 8k row of an interrupted campaign, never promote it.

No native execution; the strict frozen global row validator is reused, while
the FAILED campaign and incomplete command inventory are checked explicitly.
"""
import argparse
import base64
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_wspd_q34_lidar as reader
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import parse_result, require, write_json


def audit():
    path = HERE / "global_tsd9ofnm"
    archive = HERE / "preflight/wspd_q34_gate_before_boost_fix.cpp.txt"
    files = {str(p.relative_to(ROOT)) for p in path.iterdir() if p.is_file()}
    files.update((str(Path(__file__).relative_to(ROOT)), str(archive.relative_to(ROOT))))
    inputs_before, sources_before = pins(files), pins(reader.SOURCES)
    manifest = read_json(path / "MANIFEST.json")
    completion = read_json(path / "COMPLETION.json")
    require(manifest["schema"] == reader.SCHEMA and completion["status"] == "failed" and
            completion["error"] == "CampaignInterrupted: interrupted by signal 2" and
            completion["closing_errors"] == [], "not the preserved interrupted campaign")
    require(completion["manifest_sha256"] == digest(path / "MANIFEST.json"), "manifest changed")
    build, matrix = reader.configuration_from_launch(manifest["command"])
    datasets, commands, prepared = reader.prepare_matrix(build, matrix)
    require(matrix == dict(scans=[0], sizes=[8000,16000,32000], kmax=[5], s=[8], workers=[4],
                           backends=[28], payload="digest"), "unexpected requested matrix")
    require(manifest["matrix"] == matrix and manifest["datasets"] == datasets and
            manifest["commands"] == commands and len(commands) == 3, "launch/plan differs")
    require(set(manifest["source_sha256"]) == reader.SOURCES and
            set(manifest["artifact_sha256"]) == reader.artifacts_for(build) and
            set(manifest["input_sha256"]) == prepared, "incomplete pin inventory")
    closure = {}
    for key in ("source_sha256", "artifact_sha256", "input_sha256"):
        require(manifest[key] == completion[key + "_after"], "historical closure differs: " + key)
        closure[key] = manifest[key]
    artifacts_before = pins(manifest["artifact_sha256"])
    prepared_before = pins(prepared)
    require(artifacts_before == manifest["artifact_sha256"] and
            prepared_before == manifest["input_sha256"], "original binary/input changed")
    source_differences = {name: dict(captured=value, current=sources_before[name])
                          for name, value in manifest["source_sha256"].items()
                          if value != sources_before[name]}
    gate = "morsehgp3D_v8/tests/wspd_q34_gate.cpp"
    require(set(source_differences) == {gate} and digest(archive) == manifest["source_sha256"][gate],
            "only the independently archived subsequent Boost gate fix may differ")
    items = completion["records"]
    require(len(items) == 2 and {p.name for p in path.glob("record_*.json")} ==
            {"record_0000.json", "record_0001.json"}, "record inventory differs")
    records = []
    for number, item in enumerate(items):
        require(item["path"] == f"record_{number:04}.json" and
                item["sha256"] == digest(path / item["path"]), "record hash differs")
        record = read_json(path / item["path"])
        require(record["command"] == commands[number] and record["cwd"] == str(ROOT) and
                record["environment"] == manifest["environment"], "command/provenance differs")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"], validate=True).decode(errors="replace") ==
                    record[stream], "raw/decoded output differs")
        records.append(record)
    good, interrupted = records
    require(good["status"] == "passed" and type(good["exit_code"]) is int and good["exit_code"] == 0 and
            not good["stderr"], "8k row not completed successfully")
    row = parse_result(good["stdout"].encode())
    reader.validate(row, good["command"])
    require(row["input_hash"] == datasets[0]["input_hash"], "independent prepared prefix hash differs")
    row["scan"] = datasets[0]["scan"]
    require(row == good["result"], "raw/parsed row differs")
    require(interrupted["status"] == "failed" and interrupted["exit_code"] == -15 and
            interrupted["stdout"] == interrupted["stderr"] == "" and "result" not in interrupted,
            "16k interruption changed")
    try:
        reader.read(path)
    except reader.edge.InvalidReceipt as cause:
        campaign_rejection = str(cause)
    else:
        raise reader.edge.InvalidReceipt("official reader incorrectly accepted partial campaign")
    require(campaign_rejection == "capture identity/closure", "unexpected whole-campaign rejection")
    w, out, front = row["work"], row["output"], row["front"]
    q, local = w["q3"], w["local28"]
    ratios = {}
    def ratio(name, numerator, denominator, meaning):
        require(type(numerator) is int and type(denominator) is int and denominator > 0, "ratio operands")
        ratios[name] = dict(numerator=numerator, denominator=denominator,
                            ratio=numerator / denominator, meaning=meaning)
    ratio("retained_pair_fraction", w["expanded_pairs"], front["total_unordered_pairs"], "union q3/q4")
    ratio("front_rejected_both_fraction", front["total_unordered_pairs"] - w["expanded_pairs"],
          front["total_unordered_pairs"], "pairs rejected for both requested lanes")
    ratio("edges_per_rectangle", w["expanded_pairs"], w["input_rectangles"], "expanded union edges")
    ratio("mean_cover_sites_per_edge", w["cover_sites"], w["cover_builds"], "mass, not unique sites")
    ratio("cover_mass_per_output", w["cover_sites"], out["callbacks"], "all q3+q4 outputs")
    ratio("edges_per_output", w["expanded_pairs"], out["callbacks"], "not an output acceptance probability")
    ratio("q3_seeds_per_edge", q["seeds"], w["q3_edges"], "after acute and longest-edge owner tests")
    ratio("q4_seeds_per_edge", local["edge"]["seeds"], w["q4_edges"], "before atlas query")
    ratio("q3_census_sites_per_seed", q["census_point_tests"], q["seeds"], "scalar tests actually performed")
    ratio("q3_census_sites_per_output", q["census_point_tests"], q["emitted"], "scalar tests per emitted q3")
    ratio("q3_seeds_per_output", q["seeds"], q["emitted"], "includes seeds rejected by depth")
    ratio("q4_seeds_per_output", local["edge"]["seeds"], out["q4"], "includes atlas-pruned families")
    ratio("q3_outside_fraction", q["census_outside_sites"], q["census_point_tests"], "strictly outside")
    ratio("q3_depth_rejected_fraction", q["depth_rejections"], q["seeds"], "complete strict K-1 saturation")
    ratio("q3_fraction_of_potential_scalar_scan_read", q["census_point_tests"],
          q["census_point_tests"] + q["early_unread_sites"], "hypothetical full cover reads, not executed")
    ratio("q3_census_over_common_cover_mass", q["census_point_tests"], w["cover_sites"], "repeated reads")
    ratio("q4_leaf_queries_per_seed", local["sweep"]["leaf_queries"], local["edge"]["seeds"],
          "leaf-query count, not distinct surviving seed fraction")
    ratio("q4_atlas_query_visits_per_seed", local["sweep"]["query_visits"], local["edge"]["seeds"],
          "query traversal before local leaf sweeps")
    ratio("q3_scalar_tests_over_q4_event_sort_comparisons", q["census_point_tests"],
          local["sweep"]["sort_comparisons"], "unlike operations; never a runtime proportion")
    ratio("q4_frontier_copies_over_final_active_mass", local["atlas"]["partition"]["frontier_ids_copied"],
          local["atlas"]["active_sites_sum"], "construction copies vs final leaf population, overlapping stages")
    worker_shares = []
    for slot, worker in enumerate(row["workers_work"]):
        worker_shares.append(dict(slot=slot, **worker,
            edge_fraction=worker["expanded_pairs"] / w["expanded_pairs"],
            output_fraction=(worker["q3_emitted"] + worker["q4_emitted"]) / out["callbacks"]))
    result = dict(schema="mhgp8_partial_global_work_audit_v1", status="analysis_passed",
        campaign_status="failed", campaign_error=completion["error"], campaign_promoted=False,
        completed_rows=1, interrupted_rows=1, unstarted_rows=1, native_reexecutions=0,
        n_growth_available=False, universal_subquadratic_claim=False, full_contract_qualified=False,
        official_reader_rejection=campaign_rejection, capture=str(path.relative_to(ROOT)),
        manifest_sha256=digest(path / "MANIFEST.json"), completion_sha256=digest(path / "COMPLETION.json"),
        record_sha256=items, command=good["command"],
        completed_started_utc=good["started_utc"], completed_finished_utc=good["finished_utc"],
        timings_ms=row["timings_ms"],
        minutes=row["timings_ms"]["pipeline_including_shared_preparation"] / 60000,
        output=out, front=front, work=w, cloud_work=row["cloud_work"], index_work=row["index_work"],
        parallel=row["parallel"], workers_work=worker_shares, memory=row["memory"], ratios=ratios,
        historical_closure=closure, subsequent_source_changes=source_differences,
        original_gate_archive=str(archive.relative_to(ROOT)),
        input_sha256=inputs_before, input_sha256_after=pins(files),
        source_sha256=sources_before, source_sha256_after=pins(reader.SOURCES),
        artifact_sha256=artifacts_before, artifact_sha256_after=pins(manifest["artifact_sha256"]),
        prepared_sha256=prepared_before, prepared_sha256_after=pins(prepared),
        scope="completed-row structure/ledgers and exhaustive capture hashes; no large geometric oracle or paired digest",
        timing_limit="no per-lane or per-worker elapsed timings; counters are not additive CPU-cost units")
    for key in ("input_sha256", "source_sha256", "artifact_sha256", "prepared_sha256"):
        require(result[key] == result[key + "_after"], "analysis inputs changed: " + key)
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    require(not args.output.exists(), "refuse overwriting an analysis receipt")
    result = audit()
    write_json(args.output, result)
    print("analysis_passed; campaign_failed_preserved; completed8k=1; interrupted16k=1; unstarted32k=1")


if __name__ == "__main__":
    main()
