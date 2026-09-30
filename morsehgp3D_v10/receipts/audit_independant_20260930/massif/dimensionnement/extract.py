#!/usr/bin/env python3
"""Extraction en lecture seule ; scénarios arithmétiques, jamais prédictions."""
import argparse, csv, hashlib, json, subprocess
from collections import Counter
from decimal import Decimal
from pathlib import Path

V10 = "morsehgp3D_v10/"
S5 = V10 + "receipts/g4_session5_scale_20260929/"
SOURCE_PATHS = [
    S5 + "scale.csv", S5 + "MANIFEST_entrees.json", S5 + "vm_facts.json",
    S5 + "plan_s5_scale.json", S5 + "session/receipt.json", S5 + "SHA256SUMS",
    V10 + "receipts/ERRATA.md", V10 + "PASSATION.md", V10 + "docs/SPEC_V10.md",
    V10 + "docs/conception/EVAL_v2.md", V10 + "docs/conception/ARCH_v2.md",
    V10 + "bench/scaling/scale_run.py", V10 + "src/catalogue/generator.cpp",
    V10 + "src/catalogue/catalogue.hpp", V10 + "src/tower/tower.cpp",
    V10 + "src/core/buffer.hpp", "docs/PERFORMANCE_MORSEHGP3D.md",
    "docs/SPECIFICATION_MORSEHGP3D.md", "morsehgp3D_v7/docs/CONTRAT_PERFORMANCE.md",
    V10 + "audits/REPONSE_CLAUDE_ZERO_ET_LECTEUR_CUDA_20260930.md",
    V10 + "audits/REPONSE_CLAUDE_CINQ_SITES_ET_R2_20260930.md",
    V10 + "receipts/g4_session7_cuda_probe_20260929/cuda_probe.json",
    V10 + "receipts/g4_session7_cuda_probe_20260929/SHA256SUMS",
]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def ledger_result(folder):
    failures = []
    count = 0
    for line in (folder / "SHA256SUMS").read_text().splitlines():
        if not line.strip():
            continue
        expected, name = line.split(maxsplit=1)
        count += 1
        path = folder / name.lstrip("*")
        if not path.is_file() or sha(path) != expected:
            failures.append(name)
    return {"checked_payloads": count, "failures": failures, "scope": "integrity_only_not_scientific_qualification"}


def make_receipt(root):
    sources = [{"path": q, "size_bytes": (root / q).stat().st_size, "sha256": sha(root / q)} for q in SOURCE_PATHS]
    rows = list(csv.DictReader((root / S5 / "scale.csv").open()))
    chosen = [r for r in rows if r["file"] == "syn_clusters_density_x128.u32le" and r["k"] in ("5", "10")]
    if len(chosen) != 2 or any(r["status"] != "ok" or r["sites"] != "1024000" for r in chosen):
        raise ValueError("Unexpected S5 source rows")
    chosen.sort(key=lambda r: int(r["k"]))
    manifest = json.loads((root / S5 / "MANIFEST_entrees.json").read_text())
    facts = json.loads((root / S5 / "vm_facts.json").read_text())
    session = json.loads((root / S5 / "session/receipt.json").read_text())
    arithmetic_rows = []
    for r in chosen:
        rss_bytes = int(r["max_rss_kb"]) * 1024
        arithmetic_rows.append({"k": int(r["k"]), "rss_bytes": rss_bytes,
            "rss_gib": str(Decimal(rss_bytes) / Decimal(2**30)),
            "rss_gb_decimal": str(Decimal(rss_bytes) / Decimal(10**9)),
            "rss_bytes_per_ball": str(Decimal(rss_bytes) / Decimal(r["balls"])),
            "catalogue_plus_tower_seconds": str(Decimal(r["catalogue_s"]) + Decimal(r["tower_s"]))})
    scenarios = []
    for n in (10_000_000, 30_000_000, 50_000_000):
        for rho in (120, 460):
            balls = n * rho
            scenarios.append({"interpretation": "scenario_only_not_prediction_not_bound_not_qualification",
                "sites_assumed": n, "balls_per_site_assumed": rho, "balls_scenario": balls,
                "rss_bytes_per_ball_assumed_min": "303.6", "rss_bytes_per_ball_assumed_max": "314.9",
                "rss_bytes_scenario_min": str(Decimal(balls) * Decimal("303.6")),
                "rss_bytes_scenario_max": str(Decimal(balls) * Decimal("314.9")),
                "rss_gb_decimal_scenario_min": str(Decimal(balls) * Decimal("303.6") / Decimal(10**9)),
                "rss_gb_decimal_scenario_max": str(Decimal(balls) * Decimal("314.9") / Decimal(10**9)),
                "rss_gib_scenario_min": str(Decimal(balls) * Decimal("303.6") / Decimal(2**30)),
                "rss_gib_scenario_max": str(Decimal(balls) * Decimal("314.9") / Decimal(2**30)),
                "balls_scenario_reaches_reserved_u32_none": balls >= 2**32 - 1})
    receipt = {
        "schema": "mhgp10.audit.massif.dimensionnement.v1", "date_utc": "2026-09-30",
        "mode": "read_only_extraction_and_decimal_arithmetic", "public_status": "not_claimed",
        "gcp_used_by_this_audit": False, "gpu_used_by_this_audit": False,
        "large_compute_used_by_this_audit": False,
        "checkout_head_observed": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
        "sources": sources,
        "scope": {"measured_backend": "cpu_reference_on_g4", "threads": 48, "physical_cores": 24,
            "measured_object": "catalogue_then_FULL_tower_1_to_Kmax_with_no_points",
            "excluded_from_catalogue_plus_tower_timing": ["input_file_read", "cloud_preparation", "ground_mask", "quantization_deduplication", "point_attachment", "head", "projection_to_original_returns", "canonical_export_and_durability"],
            "process_wall_includes": ["input_file_read", "cloud_preparation", "catalogue", "tower", "summary_output", "process_lifecycle"],
            "csv_ball_count_source": "separate_catalogue_process_before_tower_process",
            "csv_timing_rss_source": "tower_process_regenerating_catalogue",
            "historical_runner_does_not_compare_the_two_catalogue_counts": True,
            "large_output_archived": "summary_only_not_full_canonical_tower_export",
            "one_seed_one_execution_per_configuration": True},
        "s5_campaign": {"rows": len(rows), "statuses": dict(Counter(r["status"] for r in rows)),
            "unique_inputs": len({r["file"] for r in rows}),
            "manifest_input_kinds": dict(Counter(e["kind"] for e in manifest["entries"])),
            "synthetic_seed": manifest["seed"], "lidar_sequence": "08",
            "lidar_frames": ["000000", "000100", "000200"], "lidar_regime": "ground_removed_grid_1mm",
            "largest_input": next(e for e in manifest["entries"] if e["file"] == "syn_clusters_density_x128.u32le"),
            "worker_commit": session["worker"]["commit"],
            "session_status": session["status"], "session_status_note": "failed_remote from pip, no failed scientific command according to preserved receipt",
            "generation": session["generation"], "targeted_shutdown_certified": session["targeted_shutdown_certified"]},
        "s5_csv_rows_exact_strings": chosen,
        "measured_row_arithmetic": arithmetic_rows,
        "machine_facts_exact": {k: facts[k] for k in ("free", "df", "lscpu", "nvidia_smi_query")},
        "machine_fact_interpretation": {"host_total_free_g_gib_integer": 176, "host_available_gib_integer": 173,
            "root_df_h_size": "97G", "root_df_h_available": "79G", "gpu_memory_total_mib": 97887,
            "scope": "captured_S5_preflight_not_current_infrastructure"},
        "scenario_rules": {"interpretation": "scenario_only_not_prediction_not_bound_not_qualification",
            "formula": "sites_assumed * balls_per_site_assumed * rss_bytes_per_ball_assumed",
            "assumption_warning": "Neither constant balls per site nor constant RSS per ball is established for LiDAR or larger inputs. Values 303.6 to 314.9 refer to corrected large measured K10 cases, not every case.",
            "excluded_resources": ["extra_point_frontier_model_cost", "raw_returns_and_masks", "mapping_and_pose_buffers", "serialized_outputs", "external_sort_spool", "checkpoint_copies", "OS_and_runtime_margin"],
            "reserved_u32_none": 2**32 - 1,
            "representation_warning": "Catalogue has unchecked casts to u32; tower checks additional cell and representative limits. Scenario ball count alone is not a capacity verdict."},
        "scenarios": scenarios,
        "slo_status": {"v10_massive_slo_found": False,
            "v10_current_perf_scope": "EVAL_v2 section 12: full LiDAR frames and cumulative 1/2/4/8-frame diagnostic, not 10/30/50M",
            "historical_phase15_v4_wall_caps_seconds": {"1000000": 600, "10000001": 3600, "30000000": 7200},
            "historical_caps_explicitly_not_migrated_to_v10": True,
            "historical_caps_are_not_current_v10_latency_targets": True,
            "historical_60s_600s_targets_not_in_current_SPEC_section15": True,
            "no_implicit_100ms_massive_target": True},
        "claude_response_status": {"cuda_strict_reader_does_not_qualify_full_gpu_engine": True,
            "old_signed_S7_receipt_intentionally_rejected_by_new_reader": True,
            "copies_r2_not_qualified_before_common_integration_capture": True},
        "integrity": {"s5": ledger_result(root / S5),
            "s7": ledger_result(root / V10 / "receipts/g4_session7_cuda_probe_20260929")},
        "validation": {"source_hashes_rechecked_before_write": True, "two_csv_rows_exact": True,
            "decimal_arithmetic_no_large_execution": True}
    }
    if any(sha(root / q["path"]) != q["sha256"] for q in sources):
        raise RuntimeError("Source changed during extraction")
    return receipt


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    result = make_receipt(args.root)
    with args.out.open("x") as f:
        json.dump(result, f, ensure_ascii=False, indent=2)
        f.write("\n")

if __name__ == "__main__":
    main()
