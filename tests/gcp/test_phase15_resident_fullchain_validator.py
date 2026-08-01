#!/usr/bin/env python3

import ast
from copy import deepcopy
import json
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
REMOTE_PATH = ROOT / "gcp-migration" / "phase3_remote_qualification.sh"
WRAPPER_PATH = ROOT / "gcp-migration" / "run_phase3_qualification.sh"
GIT_SHA = "a" * 40


def python_heredocs(source: str) -> list[str]:
    return re.findall(r"<<'PY'\n(.*?)\nPY(?:\n|$)", source, flags=re.S)


def valid_fullchain_result() -> dict:
    sha256 = "1" * 64
    timings = {
        key: 1
        for key in (
            "generation",
            "canonicalization",
            "lbvh_build",
            "automatic_recipe_catalog_wall",
            "scheduler_setup_wall",
            "scheduler_wall",
            "cut_validation_wall",
            "pair_adapter_wall",
            "higher_support_wall",
            "bridge_wall",
            "bridge_output_inspection_wall",
            "provider_replay_wall",
            "forest_reduction_wrapper_wall",
            "forest_plan_wall",
            "forest_reducer_setup_wall",
            "forest_reducer_stream_wall",
            "forest_finish_wall",
            "forest_reduction_internal_total_wall",
            "vertical_target_proposal_pipeline_wall",
            "vertical_journal_wall",
            "total",
        )
    }
    timings["forest_reduction_internal_total_wall"] = 4
    timings["forest_reduction_wrapper_wall"] = 4
    timings["total"] = 100
    return {
        "schema": (
            "morsehgp3d.phase15."
            "transactional_pair_to_conditional_forest_qualification.v4"
        ),
        "backend": "cuda_g4_plus_reference_cpu",
        "git_sha": GIT_SHA,
        "profile": "hgp_reduced",
        "mode": (
            "automatic_exact_prune_recipes_to_complete_direct_terminal_source_"
            "to_conditional_forest_to_forest_relative_multiorder_vertical_"
            "targets_and_zero_missing_label_vertical_journal"
        ),
        "public_status": "not_claimed",
        "fixture": "eight_clusters_12",
        "point_count": 12,
        "maximum_order": 5,
        "maximum_closed_rank": 6,
        "require_complete": True,
        "qualified": True,
        "recipe_catalog_certified": True,
        "cut_certified": True,
        "pair_authority_certified": True,
        "higher_terminal": True,
        "bridge_certified": True,
        "provider_replay_certified": True,
        "forest_reduction_certified": True,
        "vertical_target_pipeline_certified": True,
        "vertical_journal_certified": True,
        "automatic_recipe_catalog": {
            "decision": 9,
            "product_visits": 4,
            "maximum_frontier_blocks": 2,
            "receipt_attempts": 3,
            "certified_receipts": 2,
            "inconclusive_receipts": 1,
            "automatic_search_node_visits": 5,
            "exact_phi_aabb_maximum_evaluations": 6,
            "recipes": 2,
            "terminal_pairs_without_payload": 60,
            "pruned_mass": 6,
            "terminal_mass": 60,
            "mass_closed": True,
        },
        "pair_cut": {
            "universe": 66,
            "pruned": 6,
            "terminal": 60,
            "submitted_recipes": 2,
            "matched_recipes": 2,
            "unused_recipes": 0,
            "certified_prunes": 2,
            "kernel_launches": 1,
            "synchronizations": 3,
            "kernel_elapsed_nanoseconds": 1,
            "cuda_device": 0,
            "serial_device_reference": True,
            "scale_eligible": False,
        },
        "pair_classification": {
            "terminal_pairs": 60,
            "above_rank": 0,
            "records": 60,
            "node_visits": 1,
        },
        "reducer_source": {
            "events": 1,
            "seeds": 1,
            "batches": 1,
            "visited_batches": 1,
        },
        "forest_reduction": {
            "decision": 9,
            "plan_decision": 7,
            "last_preparation_decision": 5,
            "last_live_commit_decision": 8,
            "last_reducer_fold_decision": 8,
            "plan_lanes": 1,
            "prepared_tickets": 1,
            "committed_batches": 1,
            "resolved_keys": 0,
            "arm_joins": 0,
            "maximum_transient_closure_nodes": 1,
            "birth_records": 1,
            "saddles": 0,
            "atomic_groups": 1,
            "nodes": 1,
            "final_roots": 1,
            "logical_output_entries": 1,
            "conditional_h0_candidate": True,
        },
        "vertical_target_pipeline": {
            "decision": 15,
            "source_batches_scanned": 1,
            "source_groups_scanned": 1,
            "target_order_lookups": 1,
            "required_sessions": 1,
            "initialized_sessions": 1,
            "session_audits": 1,
            "required_groups": 1,
            "preflight_plans": 1,
            "executed_plans": 1,
            "replay_advances": 1,
            "closure_builds": 1,
            "proposal_adapters": 1,
            "group_audits": 1,
            "representatives": 2,
            "projected_target_facets": 4,
            "distinct_target_facets": 3,
            "retained_key_point_references": 7,
            "closure_terminal_summaries": 3,
            "closure_unresolved_terminals": 1,
            "closure_active_latent_terminals": 1,
            "closure_resolved_terminals": 1,
            "proposals": 2,
            "unresolved_proposals": 1,
            "resolved_proposals": 1,
            "equal_level_same_target_order_groups": 0,
            "matching_canonical_point_namespace_required": True,
            "forest_to_cloud_namespace_identity_certified": False,
            "forest_relative_only": True,
            "global_forbidden_structure_materialized": False,
            "external_target_authority_replayed": False,
            "vertical_maps_complete": False,
        },
        "vertical_journal": {
            "decision": 9,
            "expected_labels": 2,
            "missing_labels": 0,
            "unresolved_labels": 1,
            "resolved_labels": 1,
            "partial_groups": 1,
            "complete_groups": 0,
            "external_target_authority_replayed": False,
            "all_naturality_squares_replayed": False,
            "vertical_maps_complete": False,
            "public_status_claimed": False,
        },
        "digests": {
            "submitted_recipe_fnv1a": 1,
            "final_cut_fnv1a": 1,
            "pair_cloud_sha256": sha256,
            "pair_lbvh_sha256": sha256,
            "pair_output_sha256": sha256,
            "pair_semantic_sha256": sha256,
            "higher_semantic_sha256": sha256,
            "higher_output_chain_sha256": sha256,
            "higher_checkpoint_sha256": sha256,
            "normalized_terminal_output_sha256": sha256,
            "reducer_source_manifest_sha256": sha256,
        },
        "timings_nanoseconds": timings,
        "claims": {
            "ordinary_or_higher_order_delaunay": False,
            "global_pair_matrix": False,
            "hierarchy_reduction": True,
            "conditional_h0_only": True,
            "forest_relative_vertical_target_proposals": True,
            "vertical_target_authority": False,
            "vertical_maps_complete": False,
            "public_exact": False,
        },
        "qualified_scope": (
            "automatic_exact_prune_cut_to_terminal_direct_supports_bounded_"
            "conditional_h0_forest_forest_relative_multiorder_vertical_target_"
            "proposals_and_zero_missing_label_conditional_vertical_journal_only"
        ),
    }


class Phase15ResidentFullchainValidatorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        remote = REMOTE_PATH.read_text(encoding="utf-8")
        wrapper = WRAPPER_PATH.read_text(encoding="utf-8")
        cls.remote_validator = next(
            block
            for block in python_heredocs(remote)
            if "automatic_recipe_catalog: exact recipe catalog did not close" in block
        )
        wrapper_block = next(
            block
            for block in python_heredocs(wrapper)
            if "def validate_reducer_run" in block
            and "reducer automatic recipe catalog does not close" in block
        )
        tree = ast.parse(wrapper_block)
        names = {"fail", "exact_object", "integer", "validate_reducer_run"}
        functions = [
            node
            for node in tree.body
            if isinstance(node, ast.FunctionDef) and node.name in names
        ]
        namespace = {"git_sha": GIT_SHA, "re": re}
        exec(
            compile(ast.Module(body=functions, type_ignores=[]), "<validator>", "exec"),
            namespace,
        )
        cls.wrapper_validator = staticmethod(namespace["validate_reducer_run"])

    def assert_remote_accepts(self, value: dict, expected: bool) -> None:
        with tempfile.TemporaryDirectory() as directory:
            result_path = Path(directory) / "result.json"
            result_path.write_text(
                json.dumps(value, separators=(",", ":")) + "\n",
                encoding="utf-8",
            )
            completed = subprocess.run(
                ["python3", "-B", "-", str(result_path), "5", GIT_SHA],
                cwd=ROOT,
                input=self.remote_validator,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                timeout=5,
            )
        if expected:
            self.assertEqual(0, completed.returncode, completed.stderr)
        else:
            self.assertNotEqual(0, completed.returncode)

    def assert_both_accept(self, value: dict) -> None:
        self.wrapper_validator(value, 5)
        self.assert_remote_accepts(value, True)

    def assert_both_reject(self, value: dict) -> None:
        with self.assertRaises(SystemExit):
            self.wrapper_validator(value, 5)
        self.assert_remote_accepts(value, False)

    def test_v4_mixed_resolved_and_unresolved_contract_is_accepted(self) -> None:
        self.assert_both_accept(valid_fullchain_result())

    def test_catalog_receipt_and_mass_partitions_are_strict(self) -> None:
        mutations = {
            "recipe_count": ("automatic_recipe_catalog", "recipes", 3),
            "receipt_partition": (
                "automatic_recipe_catalog",
                "receipt_attempts",
                4,
            ),
            "mass_partition": ("automatic_recipe_catalog", "pruned_mass", 5),
            "terminal_payload": (
                "automatic_recipe_catalog",
                "terminal_pairs_without_payload",
                59,
            ),
        }
        for name, (section, key, replacement) in mutations.items():
            with self.subTest(name=name):
                value = deepcopy(valid_fullchain_result())
                value[section][key] = replacement
                self.assert_both_reject(value)

    def test_scheduler_recipe_consumption_counters_are_strict(self) -> None:
        for key, replacement in {
            "submitted_recipes": 3,
            "matched_recipes": 3,
            "unused_recipes": 1,
            "certified_prunes": 3,
        }.items():
            with self.subTest(counter=key):
                value = deepcopy(valid_fullchain_result())
                value["pair_cut"][key] = replacement
                self.assert_both_reject(value)

    def test_vertical_pipeline_and_journal_partitions_are_strict(self) -> None:
        mutations = {
            "missing_label": ("vertical_journal", "missing_labels", 1),
            "closure_partition": (
                "vertical_target_pipeline",
                "closure_resolved_terminals",
                0,
            ),
            "proposal_partition": (
                "vertical_target_pipeline",
                "resolved_proposals",
                0,
            ),
            "label_partition": ("vertical_journal", "resolved_labels", 0),
        }
        for name, (section, key, replacement) in mutations.items():
            with self.subTest(name=name):
                value = deepcopy(valid_fullchain_result())
                value[section][key] = replacement
                self.assert_both_reject(value)

    def test_v3_shape_and_missing_pipeline_timing_are_rejected(self) -> None:
        value = valid_fullchain_result()
        value["schema"] = (
            "morsehgp3d.phase15."
            "transactional_pair_to_conditional_forest_qualification.v3"
        )
        self.assert_both_reject(value)

        value = valid_fullchain_result()
        del value["timings_nanoseconds"]["vertical_target_proposal_pipeline_wall"]
        self.assert_both_reject(value)


if __name__ == "__main__":
    unittest.main()
