from __future__ import annotations

import json
from pathlib import Path
from types import SimpleNamespace
import sys

import numpy as np

BENCHMARK_PARENT = Path(__file__).resolve().parents[2]
if str(BENCHMARK_PARENT) not in sys.path:
    sys.path.insert(0, str(BENCHMARK_PARENT))

from benchmarks import cli, worker


def test_source_fingerprint_tracks_bytes_but_ignores_generated_caches(tmp_path: Path):
    source = tmp_path / "source"
    source.mkdir()
    module = source / "algorithm.py"
    module.write_text("VALUE = 1\n", encoding="utf-8")

    first = cli._source_fingerprint(roots=(source,), files=())
    assert first == cli._source_fingerprint(roots=(source,), files=())

    cache = source / "__pycache__"
    cache.mkdir()
    (cache / "algorithm.pyc").write_bytes(b"generated")
    (source / "generated-result.json").write_text("{}\n", encoding="utf-8")
    assert first == cli._source_fingerprint(roots=(source,), files=())

    module.write_text("VALUE = 2\n", encoding="utf-8")
    second = cli._source_fingerprint(roots=(source,), files=())
    assert second != first


def test_source_fingerprint_is_part_of_job_identity(tmp_path: Path):
    args = cli.build_parser().parse_args(
        [
            "run",
            "--methods",
            "hdbscan",
            "--datasets",
            "uniform",
            "--n",
            "3",
            "--k",
            "1",
            "--dry-run",
        ]
    )
    artifact = SimpleNamespace(
        metadata={
            "identity": {
                "generator_version": 1,
                "name": "uniform",
                "n": 3,
                "seed": 20260712,
            }
        },
        metadata_path=tmp_path / "dataset.json",
    )

    first = cli._build_jobs(
        args, [artifact], tmp_path, source_fingerprint="sha256:first"
    )
    second = cli._build_jobs(
        args, [artifact], tmp_path, source_fingerprint="sha256:second"
    )

    assert len(first) == len(second) == 1
    assert first[0]["source_fingerprint"] == "sha256:first"
    assert second[0]["source_fingerprint"] == "sha256:second"
    assert first[0]["job_id"] != second[0]["job_id"]
    assert first[0]["output_path"] != second[0]["output_path"]

    result_path = tmp_path / "existing.json"
    result_path.write_text(
        json.dumps({"job_id": first[0]["job_id"], "job": first[0]}),
        encoding="utf-8",
    )
    assert cli._resume_result_matches_job(result_path, first[0])
    assert not cli._resume_result_matches_job(result_path, second[0])


def test_candidate_cuts_include_vertex_births_as_well_as_edge_weights():
    forest = SimpleNamespace(
        births=np.asarray([0.25, 5.0], dtype=np.float64),
        weights=np.asarray([5.0], dtype=np.float64),
    )
    hierarchy = SimpleNamespace(
        base_forest=forest,
        inner_forest=forest,
        outer_forest=forest,
    )

    cuts = worker._candidate_cuts(hierarchy, 3)

    assert 0.25 in cuts
    assert 5.0 in cuts


def test_sampled_oracle_metric_keeps_explicit_schema_v1_alias():
    truth = np.asarray([0, 0, 1, 1], dtype=np.int32)
    forest = SimpleNamespace(
        births=np.asarray([0.1, 0.2, 0.3, 0.4], dtype=np.float64),
        weights=np.asarray([0.2, 0.3, 0.4], dtype=np.float64),
    )

    class Hierarchy:
        sites = SimpleNamespace(centers=np.zeros((4, 3), dtype=np.float32))
        base_forest = forest
        inner_forest = forest
        outer_forest = forest

        @staticmethod
        def labels_at_cut(radius, **kwargs):
            del radius, kwargs
            return truth.copy()

    result = worker._scan_powercover_cuts(
        Hierarchy(),
        truth,
        {
            "max_n_cut_scan": 4,
            "cut_count": 3,
            "min_cluster_size": 2,
            "max_candidate_cells_per_site": 100,
            "max_active_cells": 100,
            "max_total_cells": 100,
            "labels_path": None,
        },
    )

    assert result["status"] == "ok"
    assert result["sampled_oracle_best_ARI"] == 1.0
    assert result["oracle_best_ARI"] == 1.0
    assert result["compatibility_aliases"] == {
        "oracle_best_ARI": "sampled_oracle_best_ARI"
    }
    assert "vertex_births_and_msf_weights" in result["candidate_strategy"]

    old_row = cli._summary_row(
        {
            "method": "powercover_cpu",
            "method_result": {"cut_scan": {"oracle_best_ARI": 0.75}},
        }
    )
    assert old_row["sampled_oracle_best_ARI"] == 0.75
    assert old_row["oracle_best_ARI"] == 0.75
