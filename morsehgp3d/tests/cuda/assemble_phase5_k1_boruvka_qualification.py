#!/usr/bin/env python3
"""Validate and assemble the provisional Phase 5 K1 Boruvka G4 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from typing import Any, NoReturn, Sequence

SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_qualification.v4"
REPLAY_SCHEMA = "morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v3"
PHASE3_SCHEMA = "morsehgp3d.phase3.qualification.v1"
SCIENTIFIC_SCOPE = (
    "gpu_proposed_bounded_morton_seed_bounded_candidate_emission_"
    "cpu_exact_full_boruvka_local_emst_witness_only"
)
PROPOSAL_SEMANTICS = "gpu_stackless_lbvh_fixed_seed_candidate_superset"
DECISION_SEMANTICS = "cpu_exact_seed_replay_and_kappa_resolution"
PROOF_BASIS = "gpu_candidate_superset_cpu_exact_boruvka_v1"
BOUNDED_EMISSION_PROOF_BASIS = (
    "gpu_complete_source_ranges_bounded_candidate_payload_v1"
)
MONOTONE_SEED_PROOF_BASIS = (
    "gpu_bounded_morton_seed_cpu_exact_monotone_cutoff_v1"
)
EMISSION_MODE = "bounded_complete_source_ranges"
SOURCE_PARTITION = "complete_contiguous_unsplit"
CANONICAL_SEED_MODE = "canonical_external_fallback"
MORTON_SEED_MODE = "gpu_morton_window_cpu_exact_monotone"
MORTON_SEED_STATUS = (
    "bounded_morton_window_external_exact_monotone_certified"
)
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
SM_RE = re.compile(r"sm_[0-9]+")
SANITIZER_ERROR_RE = re.compile(r"ERROR SUMMARY:\s*([0-9]+) errors")
RACECHECK_SUMMARY_LINE_RE = re.compile(
    r"^[ \t]*=+[ \t]*RACECHECK SUMMARY:[ \t]*"
    r"([0-9]+) hazards? displayed[ \t]*"
    r"\([ \t]*([0-9]+) errors?,[ \t]*([0-9]+) warnings?[ \t]*\)"
    r"[ \t]*$",
    re.MULTILINE,
)
RACECHECK_FAILURE_RE = re.compile(
    r"Internal Sanitizer Error|Target application returned an error|"
    r"process (?:didn't|did not) terminate successfully|"
    r"No attachable process found|Potential [^\n]* hazard|Race reported",
    re.IGNORECASE,
)
REPLAY_KEYS = {
    "bounded_emission_proof_basis",
    "case_count",
    "cases",
    "decision_backend",
    "decision_semantics",
    "hierarchy_reduction_status",
    "mode",
    "monotone_seed_proof_basis",
    "phase",
    "profile",
    "proof_basis",
    "proposal_backend",
    "proposal_semantics",
    "schema",
    "scientific_result_claimed",
    "scientific_scope",
    "status",
    "verification_proposal_backend",
}
CASE_KEYS = {
    "component_count_path",
    "emst_edge_count",
    "exact_weights",
    "fixture",
    "morton_seed_comparison",
    "point_count",
    "producer",
    "rounds",
    "status",
    "trusted_chunking_policy",
    "trusted_morton_seed_policy",
    "verifier",
}
CHUNKING_POLICY_KEYS = {
    "max_candidate_records_per_chunk",
    "source_partition",
}
LEVEL_KEYS = {"denominator", "numerator"}
PRODUCER_KEYS = {
    "certificates",
    "chunked_emission_counters",
    "counters",
    "emission_mode",
    "morton_seed_counters",
    "morton_seed_policy",
    "seed_mode",
}
PRODUCER_CERTIFICATE_KEYS = {
    "bounded_candidate_emission_chain_certified",
    "bounded_morton_seed_chain_certified",
    "canonical_contraction_chain_certified",
    "cpu_exact_decision_chain_certified",
    "emst_witness_certified",
    "proposal_chain_certified",
    "reference_cpu_witness_certified",
}
CHUNKED_EMISSION_COUNTER_KEYS = {
    "candidate_payload_peak_bytes",
    "candidate_record_budget",
    "candidate_record_size_bytes",
    "count_kernel_launch_count",
    "device_candidate_capacity_high_water",
    "emit_kernel_launch_count",
    "host_candidate_capacity_high_water",
    "logical_candidate_count",
    "max_source_candidate_count",
    "peak_chunk_candidate_count",
    "peak_chunk_source_count",
    "round_count",
    "source_chunk_count",
    "synchronization_count",
}
PRODUCER_COUNTER_KEYS = {
    "accepted_edge_count",
    "component_contraction_count",
    "component_minimum_count",
    "cpu_exact_aabb_bound_evaluation_count",
    "cpu_exact_candidate_distance_evaluation_count",
    "cpu_required_candidate_count",
    "final_component_count",
    "first_buffer_epoch",
    "frozen_component_label_count",
    "gpu_candidate_count",
    "gpu_count_pass_node_visit_count",
    "gpu_emit_pass_node_visit_count",
    "gpu_invalid_bound_descent_count",
    "gpu_kernel_launch_count",
    "gpu_output_capacity_sum",
    "gpu_strict_aabb_prune_count",
    "gpu_synchronization_count",
    "gpu_uniform_component_prune_count",
    "last_buffer_epoch",
    "lbvh_node_count",
    "peak_gpu_output_capacity",
    "point_count",
    "round_count",
    "theoretical_max_round_count",
}
ROUND_KEYS = {
    "audit",
    "contraction",
    "decision",
    "emission_audit",
    "emission_status",
    "morton_seed_audit",
    "proposal_status",
    "seed_status",
}
EMISSION_AUDIT_KEYS = {
    "candidate_payload_peak_bytes",
    "candidate_record_budget",
    "candidate_record_size_bytes",
    "certificates",
    "count_kernel_launch_count",
    "device_candidate_capacity_high_water",
    "emit_kernel_launch_count",
    "host_candidate_capacity_high_water",
    "logical_candidate_count",
    "max_source_candidate_count",
    "peak_chunk_candidate_count",
    "peak_chunk_source_count",
    "source_chunk_count",
    "synchronization_count",
}
EMISSION_CERTIFICATE_KEYS = {
    "candidate_payload_physical_bound_certified",
    "complete_source_partition_certified",
    "count_emit_cardinality_and_visit_count_certified",
}
AUDIT_KEYS = {
    "buffer_epoch",
    "candidate_count",
    "certificates",
    "count_pass_node_visit_count",
    "cpu_exact_aabb_bound_evaluation_count",
    "cpu_exact_candidate_distance_evaluation_count",
    "emit_pass_node_visit_count",
    "exact_seed_count",
    "invalid_bound_descent_count",
    "kernel_launch_count",
    "mixed_lbvh_node_count",
    "output_capacity",
    "proposal_digest_fnv1a",
    "required_candidate_count",
    "resident_node_count",
    "resident_point_count",
    "strict_aabb_prune_count",
    "synchronization_count",
    "uniform_component_prune_count",
}
AUDIT_CERTIFICATE_KEYS = {
    "candidate_superset_certified",
    "cpu_exact_resolution_complete",
    "exact_capacity_certified",
    "frozen_labels_certified",
    "no_truncation_certified",
    "rope_topology_certified",
}
CONTRACTION_KEYS = {
    "accepted_edge_count",
    "post_round_component_count",
    "status",
}
DECISION_KEYS = {
    "component_minimum_count",
    "frozen_component_count",
    "round_index",
    "status",
}
VERIFIER_KEYS = {"certificates", "counters"}
VERIFIER_CERTIFICATE_KEYS = {
    "bounded_candidate_emission_chain_certified",
    "bounded_morton_seed_chain_certified",
    "canonical_contractions_certified",
    "counters_certified",
    "cpu_exact_decision_chain_certified",
    "emst_witness_certified",
    "emission_mode_certified",
    "exact_weights_certified",
    "hierarchy_status_separation_certified",
    "index_identity_certified",
    "proposal_chain_certified",
    "reference_cpu_witness_certified",
    "round_count_bound_certified",
    "seed_mode_certified",
    "spanning_tree_certified",
}
VERIFIER_COUNTER_KEYS = {
    "gpu_replay_candidate_payload_peak_bytes",
    "gpu_replay_kernel_launch_count",
    "gpu_replay_synchronization_count",
    "gpu_replayed_component_minimum_count",
    "gpu_replayed_round_count",
    "gpu_replay_peak_chunk_candidate_count",
    "gpu_replay_seed_inspected_neighbor_count",
    "gpu_replay_seed_kernel_launch_count",
    "gpu_replay_seed_selected_proposal_count",
    "gpu_replay_seed_strict_improvement_count",
    "gpu_replay_seed_synchronization_count",
    "gpu_replay_source_chunk_count",
    "reference_component_minimum_count",
    "reference_round_count",
}
MORTON_SEED_POLICY_KEYS = {"window_radius"}
MORTON_SEED_COUNTER_KEYS = {
    "exact_fallback_count",
    "exact_seed_distance_evaluation_count",
    "exact_selected_proposal_count",
    "exact_strict_improvement_count",
    "external_neighbor_count",
    "floating_proposal_count",
    "gpu_kernel_launch_count",
    "gpu_synchronization_count",
    "inspected_neighbor_count",
    "maximum_inspected_neighbor_count_per_source",
    "neighbor_inspection_budget_per_source",
    "round_count",
    "source_count",
    "window_radius",
}
MORTON_SEED_AUDIT_COUNTER_KEYS = MORTON_SEED_COUNTER_KEYS - {"round_count"}
MORTON_SEED_AUDIT_KEYS = MORTON_SEED_AUDIT_COUNTER_KEYS | {"certificates"}
MORTON_SEED_CERTIFICATE_KEYS = {
    "bounded_window_certified",
    "complete_source_coverage_certified",
    "exact_monotone_cutoff_certified",
    "external_targets_recertified",
}
MORTON_SEED_COMPARISON_KEYS = {"baseline", "certificates", "refined"}
MORTON_SEED_COMPARISON_COUNTER_KEYS = {
    "logical_candidate_count",
    "seed_mode",
    "source_chunk_count",
}
MORTON_SEED_COMPARISON_CERTIFICATE_KEYS = {
    "canonical_contractions_unchanged",
    "emst_edges_unchanged",
    "exact_decisions_unchanged",
    "exact_weights_unchanged",
}
TRUE_PRODUCER_CERTIFICATES = {
    key: True for key in sorted(PRODUCER_CERTIFICATE_KEYS)
}
TRUE_AUDIT_CERTIFICATES = {
    key: True for key in sorted(AUDIT_CERTIFICATE_KEYS)
}
TRUE_EMISSION_CERTIFICATES = {
    key: True for key in sorted(EMISSION_CERTIFICATE_KEYS)
}
TRUE_VERIFIER_CERTIFICATES = {
    key: True for key in sorted(VERIFIER_CERTIFICATE_KEYS)
}
TRUE_MORTON_SEED_CERTIFICATES = {
    key: True for key in sorted(MORTON_SEED_CERTIFICATE_KEYS)
}
FALSE_MORTON_SEED_CERTIFICATES = {
    key: False for key in sorted(MORTON_SEED_CERTIFICATE_KEYS)
}
ZERO_MORTON_SEED_COUNTERS = {
    key: 0 for key in sorted(MORTON_SEED_COUNTER_KEYS)
}
ZERO_MORTON_SEED_AUDIT = {
    key: 0 for key in sorted(MORTON_SEED_AUDIT_COUNTER_KEYS)
}
EXPECTED_FIXTURES = (
    {
        "chunk_budget": 1,
        "chunked_emission_counters": {
            "candidate_payload_peak_bytes": 0,
            "candidate_record_budget": 1,
            "candidate_record_size_bytes": 16,
            "count_kernel_launch_count": 0,
            "device_candidate_capacity_high_water": 0,
            "emit_kernel_launch_count": 0,
            "host_candidate_capacity_high_water": 0,
            "logical_candidate_count": 0,
            "max_source_candidate_count": 0,
            "peak_chunk_candidate_count": 0,
            "peak_chunk_source_count": 0,
            "round_count": 0,
            "source_chunk_count": 0,
            "synchronization_count": 0,
        },
        "component_count_path": [1],
        "emst_edge_count": 0,
        "fixture": "singleton_terminal",
        "hgp_weight": ("0", "1"),
        "point_count": 1,
        "producer_counters": {
            "accepted_edge_count": 0,
            "component_contraction_count": 0,
            "component_minimum_count": 0,
            "cpu_exact_aabb_bound_evaluation_count": 0,
            "cpu_exact_candidate_distance_evaluation_count": 0,
            "cpu_required_candidate_count": 0,
            "final_component_count": 1,
            "first_buffer_epoch": 0,
            "frozen_component_label_count": 0,
            "gpu_candidate_count": 0,
            "gpu_count_pass_node_visit_count": 0,
            "gpu_emit_pass_node_visit_count": 0,
            "gpu_invalid_bound_descent_count": 0,
            "gpu_kernel_launch_count": 0,
            "gpu_output_capacity_sum": 0,
            "gpu_strict_aabb_prune_count": 0,
            "gpu_synchronization_count": 0,
            "gpu_uniform_component_prune_count": 0,
            "last_buffer_epoch": 0,
            "lbvh_node_count": 1,
            "peak_gpu_output_capacity": 0,
            "point_count": 1,
            "round_count": 0,
            "theoretical_max_round_count": 0,
        },
        "rounds": (),
        "squared_weight": ("0", "1"),
        "verifier_counters": {
            "gpu_replay_candidate_payload_peak_bytes": 0,
            "gpu_replay_kernel_launch_count": 0,
            "gpu_replay_peak_chunk_candidate_count": 0,
            "gpu_replay_source_chunk_count": 0,
            "gpu_replay_synchronization_count": 0,
            "gpu_replayed_component_minimum_count": 0,
            "gpu_replayed_round_count": 0,
            "reference_component_minimum_count": 0,
            "reference_round_count": 0,
        },
    },
    {
        "chunk_budget": 7,
        "chunked_emission_counters": {
            "candidate_payload_peak_bytes": 224,
            "candidate_record_budget": 7,
            "candidate_record_size_bytes": 16,
            "count_kernel_launch_count": 3,
            "device_candidate_capacity_high_water": 7,
            "emit_kernel_launch_count": 16,
            "host_candidate_capacity_high_water": 7,
            "logical_candidate_count": 86,
            "max_source_candidate_count": 7,
            "peak_chunk_candidate_count": 7,
            "peak_chunk_source_count": 4,
            "round_count": 3,
            "source_chunk_count": 16,
            "synchronization_count": 19,
        },
        "component_count_path": [8, 4, 2, 1],
        "emst_edge_count": 7,
        "fixture": "chain_three_rounds",
        "hgp_weight": ("8127", "4"),
        "point_count": 8,
        "producer_counters": {
            "accepted_edge_count": 7,
            "component_contraction_count": 7,
            "component_minimum_count": 14,
            "cpu_exact_aabb_bound_evaluation_count": 212,
            "cpu_exact_candidate_distance_evaluation_count": 86,
            "cpu_required_candidate_count": 86,
            "final_component_count": 1,
            "first_buffer_epoch": 1,
            "frozen_component_label_count": 24,
            "gpu_candidate_count": 86,
            "gpu_count_pass_node_visit_count": 236,
            "gpu_emit_pass_node_visit_count": 236,
            "gpu_invalid_bound_descent_count": 0,
            "gpu_kernel_launch_count": 19,
            "gpu_output_capacity_sum": 86,
            "gpu_strict_aabb_prune_count": 20,
            "gpu_synchronization_count": 19,
            "gpu_uniform_component_prune_count": 24,
            "last_buffer_epoch": 3,
            "lbvh_node_count": 15,
            "peak_gpu_output_capacity": 36,
            "point_count": 8,
            "round_count": 3,
            "theoretical_max_round_count": 3,
        },
        "rounds": (
            {
                "audit": {
                    "buffer_epoch": 1,
                    "candidate_count": 36,
                    "count_pass_node_visit_count": 92,
                    "cpu_exact_aabb_bound_evaluation_count": 84,
                    "cpu_exact_candidate_distance_evaluation_count": 36,
                    "emit_pass_node_visit_count": 92,
                    "exact_seed_count": 8,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 7,
                    "mixed_lbvh_node_count": 7,
                    "output_capacity": 36,
                    "proposal_digest_fnv1a": "88d6d04ed0c2e06b",
                    "required_candidate_count": 36,
                    "resident_node_count": 15,
                    "resident_point_count": 8,
                    "strict_aabb_prune_count": 6,
                    "synchronization_count": 7,
                    "uniform_component_prune_count": 8,
                },
                "contraction": {
                    "accepted_edge_count": 4,
                    "post_round_component_count": 4,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 8,
                    "frozen_component_count": 8,
                    "round_index": 0,
                    "status": "cpu_exact_kappa_minima_certified",
                },
                "emission_audit": {
                    "candidate_payload_peak_bytes": 224,
                    "candidate_record_budget": 7,
                    "candidate_record_size_bytes": 16,
                    "count_kernel_launch_count": 1,
                    "device_candidate_capacity_high_water": 7,
                    "emit_kernel_launch_count": 6,
                    "host_candidate_capacity_high_water": 7,
                    "logical_candidate_count": 36,
                    "max_source_candidate_count": 7,
                    "peak_chunk_candidate_count": 7,
                    "peak_chunk_source_count": 3,
                    "source_chunk_count": 6,
                    "synchronization_count": 7,
                },
            },
            {
                "audit": {
                    "buffer_epoch": 2,
                    "candidate_count": 30,
                    "count_pass_node_visit_count": 80,
                    "cpu_exact_aabb_bound_evaluation_count": 72,
                    "cpu_exact_candidate_distance_evaluation_count": 30,
                    "emit_pass_node_visit_count": 80,
                    "exact_seed_count": 8,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 6,
                    "mixed_lbvh_node_count": 3,
                    "output_capacity": 30,
                    "proposal_digest_fnv1a": "8ba818d0f95004dd",
                    "required_candidate_count": 30,
                    "resident_node_count": 15,
                    "resident_point_count": 8,
                    "strict_aabb_prune_count": 6,
                    "synchronization_count": 6,
                    "uniform_component_prune_count": 8,
                },
                "contraction": {
                    "accepted_edge_count": 2,
                    "post_round_component_count": 2,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 4,
                    "frozen_component_count": 4,
                    "round_index": 1,
                    "status": "cpu_exact_kappa_minima_certified",
                },
                "emission_audit": {
                    "candidate_payload_peak_bytes": 208,
                    "candidate_record_budget": 7,
                    "candidate_record_size_bytes": 16,
                    "count_kernel_launch_count": 1,
                    "device_candidate_capacity_high_water": 7,
                    "emit_kernel_launch_count": 5,
                    "host_candidate_capacity_high_water": 6,
                    "logical_candidate_count": 30,
                    "max_source_candidate_count": 6,
                    "peak_chunk_candidate_count": 6,
                    "peak_chunk_source_count": 4,
                    "source_chunk_count": 5,
                    "synchronization_count": 6,
                },
            },
            {
                "audit": {
                    "buffer_epoch": 3,
                    "candidate_count": 20,
                    "count_pass_node_visit_count": 64,
                    "cpu_exact_aabb_bound_evaluation_count": 56,
                    "cpu_exact_candidate_distance_evaluation_count": 20,
                    "emit_pass_node_visit_count": 64,
                    "exact_seed_count": 8,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 6,
                    "mixed_lbvh_node_count": 1,
                    "output_capacity": 20,
                    "proposal_digest_fnv1a": "4ca55683c3c49441",
                    "required_candidate_count": 20,
                    "resident_node_count": 15,
                    "resident_point_count": 8,
                    "strict_aabb_prune_count": 8,
                    "synchronization_count": 6,
                    "uniform_component_prune_count": 8,
                },
                "contraction": {
                    "accepted_edge_count": 1,
                    "post_round_component_count": 1,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 2,
                    "frozen_component_count": 2,
                    "round_index": 2,
                    "status": "cpu_exact_kappa_minima_certified",
                },
                "emission_audit": {
                    "candidate_payload_peak_bytes": 176,
                    "candidate_record_budget": 7,
                    "candidate_record_size_bytes": 16,
                    "count_kernel_launch_count": 1,
                    "device_candidate_capacity_high_water": 7,
                    "emit_kernel_launch_count": 5,
                    "host_candidate_capacity_high_water": 4,
                    "logical_candidate_count": 20,
                    "max_source_candidate_count": 4,
                    "peak_chunk_candidate_count": 4,
                    "peak_chunk_source_count": 4,
                    "source_chunk_count": 5,
                    "synchronization_count": 6,
                },
            },
        ),
        "squared_weight": ("8127", "1"),
        "verifier_counters": {
            "gpu_replay_candidate_payload_peak_bytes": 224,
            "gpu_replay_kernel_launch_count": 19,
            "gpu_replay_peak_chunk_candidate_count": 7,
            "gpu_replay_source_chunk_count": 16,
            "gpu_replay_synchronization_count": 19,
            "gpu_replayed_component_minimum_count": 14,
            "gpu_replayed_round_count": 3,
            "reference_component_minimum_count": 14,
            "reference_round_count": 3,
        },
    },
    {
        "chunk_budget": 3,
        "chunked_emission_counters": {
            "candidate_payload_peak_bytes": 96,
            "candidate_record_budget": 3,
            "candidate_record_size_bytes": 16,
            "count_kernel_launch_count": 1,
            "device_candidate_capacity_high_water": 3,
            "emit_kernel_launch_count": 4,
            "host_candidate_capacity_high_water": 3,
            "logical_candidate_count": 9,
            "max_source_candidate_count": 3,
            "peak_chunk_candidate_count": 3,
            "peak_chunk_source_count": 1,
            "round_count": 1,
            "source_chunk_count": 4,
            "synchronization_count": 5,
        },
        "component_count_path": [4, 1],
        "emst_edge_count": 3,
        "fixture": "square_equal_length_ties",
        "hgp_weight": ("3", "1"),
        "point_count": 4,
        "producer_counters": {
            "accepted_edge_count": 3,
            "component_contraction_count": 3,
            "component_minimum_count": 4,
            "cpu_exact_aabb_bound_evaluation_count": 24,
            "cpu_exact_candidate_distance_evaluation_count": 9,
            "cpu_required_candidate_count": 9,
            "final_component_count": 1,
            "first_buffer_epoch": 1,
            "frozen_component_label_count": 4,
            "gpu_candidate_count": 9,
            "gpu_count_pass_node_visit_count": 28,
            "gpu_emit_pass_node_visit_count": 28,
            "gpu_invalid_bound_descent_count": 0,
            "gpu_kernel_launch_count": 5,
            "gpu_output_capacity_sum": 9,
            "gpu_strict_aabb_prune_count": 3,
            "gpu_synchronization_count": 5,
            "gpu_uniform_component_prune_count": 4,
            "last_buffer_epoch": 1,
            "lbvh_node_count": 7,
            "peak_gpu_output_capacity": 9,
            "point_count": 4,
            "round_count": 1,
            "theoretical_max_round_count": 2,
        },
        "rounds": (
            {
                "audit": {
                    "buffer_epoch": 1,
                    "candidate_count": 9,
                    "count_pass_node_visit_count": 28,
                    "cpu_exact_aabb_bound_evaluation_count": 24,
                    "cpu_exact_candidate_distance_evaluation_count": 9,
                    "emit_pass_node_visit_count": 28,
                    "exact_seed_count": 4,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 5,
                    "mixed_lbvh_node_count": 3,
                    "output_capacity": 9,
                    "proposal_digest_fnv1a": "84154a051a6fc1af",
                    "required_candidate_count": 9,
                    "resident_node_count": 7,
                    "resident_point_count": 4,
                    "strict_aabb_prune_count": 3,
                    "synchronization_count": 5,
                    "uniform_component_prune_count": 4,
                },
                "contraction": {
                    "accepted_edge_count": 3,
                    "post_round_component_count": 1,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 4,
                    "frozen_component_count": 4,
                    "round_index": 0,
                    "status": "cpu_exact_kappa_minima_certified",
                },
                "emission_audit": {
                    "candidate_payload_peak_bytes": 96,
                    "candidate_record_budget": 3,
                    "candidate_record_size_bytes": 16,
                    "count_kernel_launch_count": 1,
                    "device_candidate_capacity_high_water": 3,
                    "emit_kernel_launch_count": 4,
                    "host_candidate_capacity_high_water": 3,
                    "logical_candidate_count": 9,
                    "max_source_candidate_count": 3,
                    "peak_chunk_candidate_count": 3,
                    "peak_chunk_source_count": 1,
                    "source_chunk_count": 4,
                    "synchronization_count": 5,
                },
            },
        ),
        "squared_weight": ("12", "1"),
        "verifier_counters": {
            "gpu_replay_candidate_payload_peak_bytes": 96,
            "gpu_replay_kernel_launch_count": 5,
            "gpu_replay_peak_chunk_candidate_count": 3,
            "gpu_replay_source_chunk_count": 4,
            "gpu_replay_synchronization_count": 5,
            "gpu_replayed_component_minimum_count": 4,
            "gpu_replayed_round_count": 1,
            "reference_component_minimum_count": 4,
            "reference_round_count": 1,
        },
    },
)
WORKER_LIFECYCLE = {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def reject_duplicate_json_keys(
    pairs: list[tuple[str, Any]],
) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON object key: {key}")
        value[key] = item
    return value


def require_exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        fail(
            f"{label} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def exact_json_equal(actual: Any, expected: Any) -> bool:
    if type(actual) is not type(expected):
        return False
    if isinstance(expected, dict):
        return set(actual) == set(expected) and all(
            exact_json_equal(actual[key], item) for key, item in expected.items()
        )
    if isinstance(expected, list):
        return len(actual) == len(expected) and all(
            exact_json_equal(left, right)
            for left, right in zip(actual, expected, strict=True)
        )
    return bool(actual == expected)


def contains_json_key(value: Any, key: str) -> bool:
    if isinstance(value, dict):
        return key in value or any(
            contains_json_key(item, key) for item in value.values()
        )
    if isinstance(value, list):
        return any(contains_json_key(item, key) for item in value)
    return False


def require_natural(value: Any, label: str, *, positive: bool = False) -> int:
    minimum = 1 if positive else 0
    if type(value) is not int or value < minimum:
        qualifier = "positive" if positive else "non-negative"
        fail(f"{label} must be a {qualifier} integer")
    return value


def require_true_certificates(
    value: dict[str, Any], expected: set[str], label: str
) -> None:
    require_exact_keys(value, expected, label)
    if any(value.get(key) is not True for key in expected):
        fail(f"{label} must contain only true certificates")


def require_expected_certificates(
    value: dict[str, Any], expected: dict[str, bool], label: str
) -> None:
    require_exact_keys(value, set(expected), label)
    if any(type(item) is not bool for item in value.values()):
        fail(f"{label} must contain only boolean certificates")
    if not exact_json_equal(value, expected):
        fail(f"{label} differ from the seed-mode contract")


def validate_morton_seed_comparison(
    comparison: Any,
    *,
    aggregate_emission: dict[str, Any],
    case_label: str,
    seed_mode: str,
) -> None:
    if seed_mode == CANONICAL_SEED_MODE:
        if comparison is not None:
            fail(f"{case_label} canonical seed comparison must be null")
        return
    if not isinstance(comparison, dict):
        fail(f"{case_label} Morton seed comparison must be an object")
    require_exact_keys(
        comparison, MORTON_SEED_COMPARISON_KEYS, f"{case_label} seed comparison"
    )
    baseline = comparison.get("baseline")
    refined = comparison.get("refined")
    certificates = comparison.get("certificates")
    if not isinstance(baseline, dict) or not isinstance(refined, dict):
        fail(f"{case_label} Morton seed comparison counters must be objects")
    if not isinstance(certificates, dict):
        fail(f"{case_label} Morton seed comparison certificates must be an object")
    require_exact_keys(
        baseline,
        MORTON_SEED_COMPARISON_COUNTER_KEYS,
        f"{case_label} baseline seed comparison",
    )
    require_exact_keys(
        refined,
        MORTON_SEED_COMPARISON_COUNTER_KEYS,
        f"{case_label} refined seed comparison",
    )
    require_true_certificates(
        certificates,
        MORTON_SEED_COMPARISON_CERTIFICATE_KEYS,
        f"{case_label} Morton seed comparison certificates",
    )
    expected_baseline = {
        "logical_candidate_count": 86,
        "seed_mode": CANONICAL_SEED_MODE,
        "source_chunk_count": 16,
    }
    expected_refined = {
        "logical_candidate_count": 41,
        "seed_mode": MORTON_SEED_MODE,
        "source_chunk_count": 9,
    }
    if not exact_json_equal(baseline, expected_baseline):
        fail(f"{case_label} Morton baseline must close at 86 candidates / 16 chunks")
    if not exact_json_equal(refined, expected_refined):
        fail(f"{case_label} Morton refinement must close at 41 candidates / 9 chunks")
    if (
        refined["logical_candidate_count"]
        != aggregate_emission.get("logical_candidate_count")
        or refined["source_chunk_count"]
        != aggregate_emission.get("source_chunk_count")
    ):
        fail(f"{case_label} Morton comparison does not bind producer counters")


def validate_morton_seed_contract(case: dict[str, Any], case_label: str) -> None:
    point_count = require_natural(
        case.get("point_count"), f"{case_label} point_count", positive=True
    )
    producer = case.get("producer")
    verifier = case.get("verifier")
    rounds = case.get("rounds")
    if (
        not isinstance(producer, dict)
        or not isinstance(verifier, dict)
        or not isinstance(rounds, list)
    ):
        fail(f"{case_label} Morton seed producer, verifier, or rounds are absent")
    seed_mode = producer.get("seed_mode")
    if seed_mode not in {CANONICAL_SEED_MODE, MORTON_SEED_MODE}:
        fail(f"{case_label} seed mode differs")

    producer_policy = producer.get("morton_seed_policy")
    trusted_policy = case.get("trusted_morton_seed_policy")
    aggregate = producer.get("morton_seed_counters")
    if not isinstance(producer_policy, dict):
        fail(f"{case_label} producer Morton seed policy must be an object")
    require_exact_keys(
        producer_policy, MORTON_SEED_POLICY_KEYS, f"{case_label} seed policy"
    )
    if not isinstance(aggregate, dict):
        fail(f"{case_label} Morton seed counters must be an object")
    require_exact_keys(
        aggregate, MORTON_SEED_COUNTER_KEYS, f"{case_label} Morton seed counters"
    )
    aggregate_values = {
        key: require_natural(value, f"{case_label} Morton seed counter {key}")
        for key, value in aggregate.items()
    }
    radius = require_natural(
        producer_policy.get("window_radius"),
        f"{case_label} Morton seed window radius",
    )

    producer_certificates = producer.get("certificates")
    verifier_certificates = verifier.get("certificates")
    if not isinstance(producer_certificates, dict):
        fail(f"{case_label} producer certificates must be an object")
    if not isinstance(verifier_certificates, dict):
        fail(f"{case_label} verifier certificates must be an object")
    expected_producer_certificates = dict(TRUE_PRODUCER_CERTIFICATES)
    expected_verifier_certificates = dict(TRUE_VERIFIER_CERTIFICATES)
    seed_certified = seed_mode == MORTON_SEED_MODE
    expected_producer_certificates["bounded_morton_seed_chain_certified"] = (
        seed_certified
    )
    expected_verifier_certificates["bounded_morton_seed_chain_certified"] = (
        seed_certified
    )
    require_expected_certificates(
        producer_certificates,
        expected_producer_certificates,
        f"{case_label} producer certificates",
    )
    require_expected_certificates(
        verifier_certificates,
        expected_verifier_certificates,
        f"{case_label} verifier certificates",
    )

    verifier_counters = verifier.get("counters")
    if not isinstance(verifier_counters, dict):
        fail(f"{case_label} verifier counters must be an object")
    replay_seed_mapping = (
        ("gpu_replay_seed_inspected_neighbor_count", "inspected_neighbor_count"),
        ("gpu_replay_seed_kernel_launch_count", "gpu_kernel_launch_count"),
        (
            "gpu_replay_seed_selected_proposal_count",
            "exact_selected_proposal_count",
        ),
        (
            "gpu_replay_seed_strict_improvement_count",
            "exact_strict_improvement_count",
        ),
        ("gpu_replay_seed_synchronization_count", "gpu_synchronization_count"),
    )
    for verifier_key, aggregate_key in replay_seed_mapping:
        if verifier_counters.get(verifier_key) != aggregate_values[aggregate_key]:
            fail(f"{case_label} verifier Morton seed counter {verifier_key} differs")

    if seed_mode == CANONICAL_SEED_MODE:
        if trusted_policy is not None or radius != 0:
            fail(f"{case_label} canonical seed policy must be null and zero")
        if any(aggregate_values.values()):
            fail(f"{case_label} canonical seed counters must all be zero")
        for round_index, round_value in enumerate(rounds):
            round_label = f"{case_label} round {round_index}"
            seed_audit = round_value.get("morton_seed_audit")
            if not isinstance(seed_audit, dict):
                fail(f"{round_label} Morton seed audit must be an object")
            require_exact_keys(seed_audit, MORTON_SEED_AUDIT_KEYS, round_label)
            certificates = seed_audit.get("certificates")
            if not isinstance(certificates, dict):
                fail(f"{round_label} Morton seed certificates must be an object")
            require_expected_certificates(
                certificates,
                FALSE_MORTON_SEED_CERTIFICATES,
                f"{round_label} Morton seed certificates",
            )
            if any(
                seed_audit.get(key) != 0 for key in MORTON_SEED_AUDIT_COUNTER_KEYS
            ):
                fail(f"{round_label} canonical Morton seed audit must be zero")
            if round_value.get("seed_status") != "not_certified":
                fail(f"{round_label} canonical seed status differs")
        validate_morton_seed_comparison(
            case.get("morton_seed_comparison"),
            aggregate_emission=producer["chunked_emission_counters"],
            case_label=case_label,
            seed_mode=seed_mode,
        )
        return

    if not isinstance(trusted_policy, dict):
        fail(f"{case_label} trusted Morton seed policy must be an object")
    require_exact_keys(
        trusted_policy,
        MORTON_SEED_POLICY_KEYS,
        f"{case_label} trusted Morton seed policy",
    )
    trusted_radius = require_natural(
        trusted_policy.get("window_radius"),
        f"{case_label} trusted Morton seed window radius",
        positive=True,
    )
    if radius != trusted_radius or aggregate_values["window_radius"] != radius:
        fail(f"{case_label} Morton seed policy identities differ")
    expected_budget = 2 * radius
    expected_inspected = min(radius, point_count - 1) * (
        2 * point_count - min(radius, point_count - 1) - 1
    )
    summed_keys = MORTON_SEED_AUDIT_COUNTER_KEYS - {
        "maximum_inspected_neighbor_count_per_source",
        "neighbor_inspection_budget_per_source",
        "window_radius",
    }
    summed = {key: 0 for key in summed_keys}
    maxima = {
        "maximum_inspected_neighbor_count_per_source": 0,
        "neighbor_inspection_budget_per_source": 0,
    }
    for round_index, round_value in enumerate(rounds):
        round_label = f"{case_label} round {round_index}"
        seed_audit = round_value.get("morton_seed_audit")
        if not isinstance(seed_audit, dict):
            fail(f"{round_label} Morton seed audit must be an object")
        require_exact_keys(seed_audit, MORTON_SEED_AUDIT_KEYS, round_label)
        certificates = seed_audit.get("certificates")
        if not isinstance(certificates, dict):
            fail(f"{round_label} Morton seed certificates must be an object")
        require_true_certificates(
            certificates,
            MORTON_SEED_CERTIFICATE_KEYS,
            f"{round_label} Morton seed certificates",
        )
        numeric = {
            key: require_natural(seed_audit.get(key), f"{round_label} seed {key}")
            for key in MORTON_SEED_AUDIT_COUNTER_KEYS
        }
        if round_value.get("seed_status") != MORTON_SEED_STATUS:
            fail(f"{round_label} Morton seed status differs")
        if not (
            numeric["source_count"] == point_count
            and numeric["window_radius"] == radius
            and numeric["neighbor_inspection_budget_per_source"]
            == expected_budget
            and numeric["maximum_inspected_neighbor_count_per_source"]
            <= expected_budget
            and numeric["inspected_neighbor_count"] == expected_inspected
            and numeric["external_neighbor_count"]
            <= numeric["inspected_neighbor_count"]
            and numeric["floating_proposal_count"]
            <= numeric["external_neighbor_count"]
            and numeric["floating_proposal_count"] <= point_count
            and numeric["exact_selected_proposal_count"]
            <= numeric["floating_proposal_count"]
            and numeric["exact_strict_improvement_count"]
            <= numeric["exact_selected_proposal_count"]
            and numeric["exact_fallback_count"]
            + numeric["exact_selected_proposal_count"]
            == point_count
            and point_count
            <= numeric["exact_seed_distance_evaluation_count"]
            <= point_count + numeric["floating_proposal_count"]
            and numeric["exact_selected_proposal_count"]
            <= numeric["exact_seed_distance_evaluation_count"] - point_count
            and numeric["gpu_kernel_launch_count"] == 1
            and numeric["gpu_synchronization_count"] == 1
        ):
            fail(f"{round_label} Morton seed accounting differs")
        for key in summed:
            summed[key] += numeric[key]
        for key in maxima:
            maxima[key] = max(maxima[key], numeric[key])
    if aggregate_values["round_count"] != len(rounds):
        fail(f"{case_label} aggregate Morton seed round count differs")
    for key, expected in summed.items():
        if aggregate_values[key] != expected:
            fail(f"{case_label} aggregate Morton seed sum {key} differs")
    for key, expected in maxima.items():
        if aggregate_values[key] != expected:
            fail(f"{case_label} aggregate Morton seed maximum {key} differs")
    validate_morton_seed_comparison(
        case.get("morton_seed_comparison"),
        aggregate_emission=producer["chunked_emission_counters"],
        case_label=case_label,
        seed_mode=seed_mode,
    )


def validate_chunked_emission_contract(
    case: dict[str, Any], case_label: str
) -> None:
    point_count = require_natural(
        case.get("point_count"), f"{case_label} point_count", positive=True
    )
    policy = case.get("trusted_chunking_policy")
    producer = case.get("producer")
    rounds = case.get("rounds")
    verifier = case.get("verifier")
    if not isinstance(policy, dict):
        fail(f"{case_label} trusted_chunking_policy must be an object")
    require_exact_keys(policy, CHUNKING_POLICY_KEYS, f"{case_label} policy")
    budget = require_natural(
        policy.get("max_candidate_records_per_chunk"),
        f"{case_label} chunk budget",
        positive=True,
    )
    if policy.get("source_partition") != SOURCE_PARTITION:
        fail(f"{case_label} source partition differs")
    if not isinstance(producer, dict) or not isinstance(rounds, list):
        fail(f"{case_label} producer or rounds are absent")
    if not isinstance(verifier, dict):
        fail(f"{case_label} verifier is absent")
    if producer.get("emission_mode") != EMISSION_MODE:
        fail(f"{case_label} producer emission mode differs")

    aggregate = producer.get("chunked_emission_counters")
    if not isinstance(aggregate, dict):
        fail(f"{case_label} chunked emission counters must be an object")
    require_exact_keys(
        aggregate,
        CHUNKED_EMISSION_COUNTER_KEYS,
        f"{case_label} chunked emission counters",
    )
    aggregate_values = {
        key: require_natural(value, f"{case_label} chunked counter {key}")
        for key, value in aggregate.items()
    }
    if aggregate_values["round_count"] != len(rounds):
        fail(f"{case_label} chunked round count differs")
    if aggregate_values["candidate_record_budget"] != budget:
        fail(f"{case_label} aggregate chunk budget differs")
    if aggregate_values["candidate_record_size_bytes"] != 16:
        fail(f"{case_label} candidate record size differs")

    summed = {
        "logical_candidate_count": 0,
        "source_chunk_count": 0,
        "count_kernel_launch_count": 0,
        "emit_kernel_launch_count": 0,
        "synchronization_count": 0,
    }
    maxima = {
        "peak_chunk_source_count": 0,
        "peak_chunk_candidate_count": 0,
        "max_source_candidate_count": 0,
        "device_candidate_capacity_high_water": 0,
        "host_candidate_capacity_high_water": 0,
        "candidate_payload_peak_bytes": 0,
    }
    for round_index, round_value in enumerate(rounds):
        round_label = f"{case_label} round {round_index}"
        emission = round_value.get("emission_audit")
        audit = round_value.get("audit")
        if not isinstance(emission, dict):
            fail(f"{round_label} emission audit must be an object")
        require_exact_keys(emission, EMISSION_AUDIT_KEYS, f"{round_label} emission")
        emission_certificates = emission.get("certificates")
        if not isinstance(emission_certificates, dict):
            fail(f"{round_label} emission certificates must be an object")
        require_true_certificates(
            emission_certificates,
            EMISSION_CERTIFICATE_KEYS,
            f"{round_label} emission certificates",
        )
        numeric = {
            key: require_natural(value, f"{round_label} emission {key}")
            for key, value in emission.items()
            if key != "certificates"
        }
        if round_value.get("emission_status") != (
            "complete_source_ranges_candidate_payload_bound_certified"
        ):
            fail(f"{round_label} emission status differs")
        if not isinstance(audit, dict):
            fail(f"{round_label} proposal audit must be an object")
        if numeric["logical_candidate_count"] != audit.get("candidate_count"):
            fail(f"{round_label} logical candidate count differs")
        if numeric["candidate_record_budget"] != budget:
            fail(f"{round_label} candidate budget differs")
        if numeric["candidate_record_size_bytes"] != 16:
            fail(f"{round_label} candidate record size differs")
        if not (
            0 < numeric["source_chunk_count"]
            and 0 < numeric["peak_chunk_source_count"] <= point_count
            and 0 < numeric["max_source_candidate_count"]
            <= numeric["peak_chunk_candidate_count"]
            <= budget
            and numeric["device_candidate_capacity_high_water"]
            >= numeric["peak_chunk_candidate_count"]
            and numeric["device_candidate_capacity_high_water"] <= budget
            and numeric["host_candidate_capacity_high_water"]
            >= numeric["peak_chunk_candidate_count"]
            and numeric["host_candidate_capacity_high_water"] <= budget
        ):
            fail(f"{round_label} physical chunk bounds differ")
        expected_payload = (
            numeric["device_candidate_capacity_high_water"]
            + numeric["host_candidate_capacity_high_water"]
        ) * numeric["candidate_record_size_bytes"]
        if (
            numeric["candidate_payload_peak_bytes"] != expected_payload
            or expected_payload > 2 * budget * 16
        ):
            fail(f"{round_label} candidate payload bound differs")
        if not (
            numeric["count_kernel_launch_count"] == 1
            and numeric["emit_kernel_launch_count"]
            == numeric["source_chunk_count"]
            and numeric["synchronization_count"]
            == numeric["count_kernel_launch_count"]
            + numeric["emit_kernel_launch_count"]
            and audit.get("kernel_launch_count")
            == numeric["synchronization_count"]
            and audit.get("synchronization_count")
            == numeric["synchronization_count"]
        ):
            fail(f"{round_label} chunk launch accounting differs")
        for key in summed:
            summed[key] += numeric[key]
        for key in maxima:
            maxima[key] = max(maxima[key], numeric[key])

    for key, expected in summed.items():
        if aggregate_values[key] != expected:
            fail(f"{case_label} aggregate sum {key} differs")
    for key, expected in maxima.items():
        if aggregate_values[key] != expected:
            fail(f"{case_label} aggregate maximum {key} differs")
    producer_counters = producer.get("counters")
    if not isinstance(producer_counters, dict):
        fail(f"{case_label} producer counters are absent")
    if aggregate_values["logical_candidate_count"] != producer_counters.get(
        "gpu_candidate_count"
    ):
        fail(f"{case_label} logical and producer candidate counts differ")
    verifier_counters = verifier.get("counters")
    if not isinstance(verifier_counters, dict):
        fail(f"{case_label} verifier counters are absent")
    for verifier_key, aggregate_key in (
        ("gpu_replay_source_chunk_count", "source_chunk_count"),
        ("gpu_replay_peak_chunk_candidate_count", "peak_chunk_candidate_count"),
        ("gpu_replay_candidate_payload_peak_bytes", "candidate_payload_peak_bytes"),
    ):
        if verifier_counters.get(verifier_key) != aggregate_values[aggregate_key]:
            fail(f"{case_label} verifier chunk counter {verifier_key} differs")


def expected_level(weight: tuple[str, str]) -> dict[str, str]:
    numerator, denominator = weight
    return {"denominator": denominator, "numerator": numerator}


def expected_round(
    specification: dict[str, Any], *, seed_mode: str
) -> dict[str, Any]:
    audit = dict(specification["audit"])
    audit["certificates"] = dict(TRUE_AUDIT_CERTIFICATES)
    emission_audit = dict(specification["emission_audit"])
    emission_audit["certificates"] = dict(TRUE_EMISSION_CERTIFICATES)
    morton_seed_audit = dict(
        specification.get("morton_seed_audit", ZERO_MORTON_SEED_AUDIT)
    )
    morton_seed_audit["certificates"] = dict(
        TRUE_MORTON_SEED_CERTIFICATES
        if seed_mode == MORTON_SEED_MODE
        else FALSE_MORTON_SEED_CERTIFICATES
    )
    return {
        "audit": audit,
        "contraction": dict(specification["contraction"]),
        "decision": dict(specification["decision"]),
        "emission_audit": emission_audit,
        "emission_status": (
            "complete_source_ranges_candidate_payload_bound_certified"
        ),
        "morton_seed_audit": morton_seed_audit,
        "proposal_status": "candidate_superset_certified",
        "seed_status": (
            MORTON_SEED_STATUS
            if seed_mode == MORTON_SEED_MODE
            else "not_certified"
        ),
    }


def expected_case(specification: dict[str, Any]) -> dict[str, Any]:
    seed_mode = specification.get("seed_mode", CANONICAL_SEED_MODE)
    producer_certificates = dict(TRUE_PRODUCER_CERTIFICATES)
    producer_certificates["bounded_morton_seed_chain_certified"] = (
        seed_mode == MORTON_SEED_MODE
    )
    verifier_certificates = dict(TRUE_VERIFIER_CERTIFICATES)
    verifier_certificates["bounded_morton_seed_chain_certified"] = (
        seed_mode == MORTON_SEED_MODE
    )
    verifier_counters = {
        "gpu_replay_seed_inspected_neighbor_count": 0,
        "gpu_replay_seed_kernel_launch_count": 0,
        "gpu_replay_seed_selected_proposal_count": 0,
        "gpu_replay_seed_strict_improvement_count": 0,
        "gpu_replay_seed_synchronization_count": 0,
        **specification["verifier_counters"],
    }
    return {
        "component_count_path": list(specification["component_count_path"]),
        "emst_edge_count": specification["emst_edge_count"],
        "exact_weights": {
            "hgp": expected_level(specification["hgp_weight"]),
            "squared": expected_level(specification["squared_weight"]),
        },
        "fixture": specification["fixture"],
        "morton_seed_comparison": specification.get("morton_seed_comparison"),
        "point_count": specification["point_count"],
        "producer": {
            "certificates": producer_certificates,
            "chunked_emission_counters": dict(
                specification["chunked_emission_counters"]
            ),
            "counters": dict(specification["producer_counters"]),
            "emission_mode": EMISSION_MODE,
            "morton_seed_counters": dict(
                specification.get(
                    "morton_seed_counters", ZERO_MORTON_SEED_COUNTERS
                )
            ),
            "morton_seed_policy": {
                "window_radius": specification.get("seed_window_radius", 0)
            },
            "seed_mode": seed_mode,
        },
        "rounds": [
            expected_round(round_specification, seed_mode=seed_mode)
            for round_specification in specification["rounds"]
        ],
        "status": "passed",
        "trusted_chunking_policy": {
            "max_candidate_records_per_chunk": specification["chunk_budget"],
            "source_partition": SOURCE_PARTITION,
        },
        "trusted_morton_seed_policy": (
            {"window_radius": specification["seed_window_radius"]}
            if seed_mode == MORTON_SEED_MODE
            else None
        ),
        "verifier": {
            "certificates": verifier_certificates,
            "counters": verifier_counters,
        },
    }


def expected_morton_seed_case() -> dict[str, Any]:
    case = expected_case(EXPECTED_FIXTURES[1])
    case["fixture"] = "chain_three_rounds_morton_seed"
    case["morton_seed_comparison"] = {
        "baseline": {
            "logical_candidate_count": 86,
            "seed_mode": CANONICAL_SEED_MODE,
            "source_chunk_count": 16,
        },
        "certificates": {
            key: True for key in sorted(MORTON_SEED_COMPARISON_CERTIFICATE_KEYS)
        },
        "refined": {
            "logical_candidate_count": 41,
            "seed_mode": MORTON_SEED_MODE,
            "source_chunk_count": 9,
        },
    }
    case["trusted_morton_seed_policy"] = {"window_radius": 1}

    producer = case["producer"]
    producer["certificates"]["bounded_morton_seed_chain_certified"] = True
    producer["chunked_emission_counters"] = {
        "candidate_payload_peak_bytes": 224,
        "candidate_record_budget": 7,
        "candidate_record_size_bytes": 16,
        "count_kernel_launch_count": 3,
        "device_candidate_capacity_high_water": 7,
        "emit_kernel_launch_count": 9,
        "host_candidate_capacity_high_water": 7,
        "logical_candidate_count": 41,
        "max_source_candidate_count": 6,
        "peak_chunk_candidate_count": 7,
        "peak_chunk_source_count": 7,
        "round_count": 3,
        "source_chunk_count": 9,
        "synchronization_count": 12,
    }
    producer["counters"].update(
        {
            "cpu_exact_aabb_bound_evaluation_count": 160,
            "cpu_exact_candidate_distance_evaluation_count": 41,
            "cpu_required_candidate_count": 41,
            "gpu_candidate_count": 41,
            "gpu_count_pass_node_visit_count": 184,
            "gpu_emit_pass_node_visit_count": 184,
            "gpu_kernel_launch_count": 12,
            "gpu_output_capacity_sum": 41,
            "gpu_strict_aabb_prune_count": 39,
            "gpu_synchronization_count": 12,
            "peak_gpu_output_capacity": 17,
        }
    )
    producer["morton_seed_counters"] = {
        "exact_fallback_count": 13,
        "exact_seed_distance_evaluation_count": 36,
        "exact_selected_proposal_count": 11,
        "exact_strict_improvement_count": 11,
        "external_neighbor_count": 22,
        "floating_proposal_count": 16,
        "gpu_kernel_launch_count": 3,
        "gpu_synchronization_count": 3,
        "inspected_neighbor_count": 42,
        "maximum_inspected_neighbor_count_per_source": 2,
        "neighbor_inspection_budget_per_source": 2,
        "round_count": 3,
        "source_count": 24,
        "window_radius": 1,
    }
    producer["morton_seed_policy"] = {"window_radius": 1}
    producer["seed_mode"] = MORTON_SEED_MODE

    round_audit_updates = (
        {
            "candidate_count": 8,
            "count_pass_node_visit_count": 56,
            "cpu_exact_aabb_bound_evaluation_count": 48,
            "cpu_exact_candidate_distance_evaluation_count": 8,
            "emit_pass_node_visit_count": 56,
            "kernel_launch_count": 3,
            "output_capacity": 8,
            "proposal_digest_fnv1a": "028191f9d06c374d",
            "required_candidate_count": 8,
            "strict_aabb_prune_count": 16,
            "synchronization_count": 3,
        },
        {
            "candidate_count": 16,
            "count_pass_node_visit_count": 66,
            "cpu_exact_aabb_bound_evaluation_count": 58,
            "cpu_exact_candidate_distance_evaluation_count": 16,
            "emit_pass_node_visit_count": 66,
            "kernel_launch_count": 4,
            "output_capacity": 16,
            "proposal_digest_fnv1a": "436a2d2c18ed11df",
            "required_candidate_count": 16,
            "strict_aabb_prune_count": 13,
            "synchronization_count": 4,
        },
        {
            "candidate_count": 17,
            "count_pass_node_visit_count": 62,
            "cpu_exact_aabb_bound_evaluation_count": 54,
            "cpu_exact_candidate_distance_evaluation_count": 17,
            "emit_pass_node_visit_count": 62,
            "kernel_launch_count": 5,
            "output_capacity": 17,
            "proposal_digest_fnv1a": "0691f0ec29352576",
            "required_candidate_count": 17,
            "strict_aabb_prune_count": 10,
            "synchronization_count": 5,
        },
    )
    round_emission_audits = (
        {
            "candidate_payload_peak_bytes": 224,
            "candidate_record_budget": 7,
            "candidate_record_size_bytes": 16,
            "count_kernel_launch_count": 1,
            "device_candidate_capacity_high_water": 7,
            "emit_kernel_launch_count": 2,
            "host_candidate_capacity_high_water": 7,
            "logical_candidate_count": 8,
            "max_source_candidate_count": 1,
            "peak_chunk_candidate_count": 7,
            "peak_chunk_source_count": 7,
            "source_chunk_count": 2,
            "synchronization_count": 3,
        },
        {
            "candidate_payload_peak_bytes": 208,
            "candidate_record_budget": 7,
            "candidate_record_size_bytes": 16,
            "count_kernel_launch_count": 1,
            "device_candidate_capacity_high_water": 7,
            "emit_kernel_launch_count": 3,
            "host_candidate_capacity_high_water": 6,
            "logical_candidate_count": 16,
            "max_source_candidate_count": 6,
            "peak_chunk_candidate_count": 6,
            "peak_chunk_source_count": 4,
            "source_chunk_count": 3,
            "synchronization_count": 4,
        },
        {
            "candidate_payload_peak_bytes": 192,
            "candidate_record_budget": 7,
            "candidate_record_size_bytes": 16,
            "count_kernel_launch_count": 1,
            "device_candidate_capacity_high_water": 7,
            "emit_kernel_launch_count": 4,
            "host_candidate_capacity_high_water": 5,
            "logical_candidate_count": 17,
            "max_source_candidate_count": 4,
            "peak_chunk_candidate_count": 5,
            "peak_chunk_source_count": 5,
            "source_chunk_count": 4,
            "synchronization_count": 5,
        },
    )
    round_seed_audits = (
        {
            "exact_fallback_count": 2,
            "exact_seed_distance_evaluation_count": 14,
            "exact_selected_proposal_count": 6,
            "exact_strict_improvement_count": 6,
            "external_neighbor_count": 14,
            "floating_proposal_count": 8,
            "gpu_kernel_launch_count": 1,
            "gpu_synchronization_count": 1,
            "inspected_neighbor_count": 14,
            "maximum_inspected_neighbor_count_per_source": 2,
            "neighbor_inspection_budget_per_source": 2,
            "source_count": 8,
            "window_radius": 1,
        },
        {
            "exact_fallback_count": 4,
            "exact_seed_distance_evaluation_count": 13,
            "exact_selected_proposal_count": 4,
            "exact_strict_improvement_count": 4,
            "external_neighbor_count": 6,
            "floating_proposal_count": 6,
            "gpu_kernel_launch_count": 1,
            "gpu_synchronization_count": 1,
            "inspected_neighbor_count": 14,
            "maximum_inspected_neighbor_count_per_source": 2,
            "neighbor_inspection_budget_per_source": 2,
            "source_count": 8,
            "window_radius": 1,
        },
        {
            "exact_fallback_count": 7,
            "exact_seed_distance_evaluation_count": 9,
            "exact_selected_proposal_count": 1,
            "exact_strict_improvement_count": 1,
            "external_neighbor_count": 2,
            "floating_proposal_count": 2,
            "gpu_kernel_launch_count": 1,
            "gpu_synchronization_count": 1,
            "inspected_neighbor_count": 14,
            "maximum_inspected_neighbor_count_per_source": 2,
            "neighbor_inspection_budget_per_source": 2,
            "source_count": 8,
            "window_radius": 1,
        },
    )
    for round_value, audit_updates, emission, seed_audit in zip(
        case["rounds"],
        round_audit_updates,
        round_emission_audits,
        round_seed_audits,
        strict=True,
    ):
        round_value["audit"].update(audit_updates)
        round_value["emission_audit"] = {
            **emission,
            "certificates": dict(TRUE_EMISSION_CERTIFICATES),
        }
        round_value["morton_seed_audit"] = {
            **seed_audit,
            "certificates": dict(TRUE_MORTON_SEED_CERTIFICATES),
        }
        round_value["seed_status"] = MORTON_SEED_STATUS

    verifier = case["verifier"]
    verifier["certificates"]["bounded_morton_seed_chain_certified"] = True
    verifier["counters"].update(
        {
            "gpu_replay_candidate_payload_peak_bytes": 224,
            "gpu_replay_kernel_launch_count": 12,
            "gpu_replay_peak_chunk_candidate_count": 7,
            "gpu_replay_seed_inspected_neighbor_count": 42,
            "gpu_replay_seed_kernel_launch_count": 3,
            "gpu_replay_seed_selected_proposal_count": 11,
            "gpu_replay_seed_strict_improvement_count": 11,
            "gpu_replay_seed_synchronization_count": 3,
            "gpu_replay_source_chunk_count": 9,
            "gpu_replay_synchronization_count": 12,
        }
    )
    return case


EXPECTED_REPLAY = {
    "bounded_emission_proof_basis": BOUNDED_EMISSION_PROOF_BASIS,
    "case_count": 4,
    "cases": [
        *(expected_case(specification) for specification in EXPECTED_FIXTURES),
        expected_morton_seed_case(),
    ],
    "decision_backend": "reference_cpu",
    "decision_semantics": DECISION_SEMANTICS,
    "hierarchy_reduction_status": "not_performed",
    "mode": "certified",
    "monotone_seed_proof_basis": MONOTONE_SEED_PROOF_BASIS,
    "phase": "5",
    "profile": "hgp_reduced",
    "proof_basis": PROOF_BASIS,
    "proposal_backend": "cuda_g4",
    "proposal_semantics": PROPOSAL_SEMANTICS,
    "schema": REPLAY_SCHEMA,
    "scientific_result_claimed": False,
    "scientific_scope": "local_emst_witness_only",
    "status": "passed",
    "verification_proposal_backend": "cuda_g4",
}


def read_json_object(path: Path, label: str) -> tuple[str, dict[str, Any]]:
    try:
        raw = path.read_text(encoding="utf-8")
        value = json.loads(raw, object_pairs_hook=reject_duplicate_json_keys)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must be a JSON object")
    return raw, value


def read_text(path: Path, label: str, *, allow_empty: bool = False) -> str:
    try:
        value = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not allow_empty and not value.strip():
        fail(f"{label} is empty")
    return value


def sha256_file(path: Path, label: str) -> str:
    if not path.is_file() or path.is_symlink():
        fail(f"{label} must be a regular non-symbolic file")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as error:
        fail(f"cannot hash {label}: {error}")
    return digest.hexdigest()


def validate_replay(path: Path) -> tuple[str, dict[str, Any]]:
    label = "Phase 5 full-loop K1 Boruvka replay"
    raw, value = read_json_object(path, label)
    canonical = json.dumps(
        value, ensure_ascii=True, sort_keys=True, separators=(",", ":")
    )
    if raw != canonical + "\n":
        fail(f"{label} must be canonical one-line JSON")
    require_exact_keys(value, REPLAY_KEYS, label)
    cases = value.get("cases")
    if not isinstance(cases, list) or len(cases) != 4:
        fail(f"{label} must contain exactly the four closed fixtures")
    for case_index, case in enumerate(cases):
        case_label = f"{label} case {case_index}"
        if not isinstance(case, dict):
            fail(f"{case_label} must be an object")
        require_exact_keys(case, CASE_KEYS, case_label)
        weights = case.get("exact_weights")
        if not isinstance(weights, dict):
            fail(f"{case_label} exact_weights must be an object")
        require_exact_keys(weights, {"hgp", "squared"}, f"{case_label} weights")
        for level_name in ("hgp", "squared"):
            level = weights.get(level_name)
            if not isinstance(level, dict):
                fail(f"{case_label} {level_name} weight must be an object")
            require_exact_keys(level, LEVEL_KEYS, f"{case_label} {level_name} weight")

        policy = case.get("trusted_chunking_policy")
        if not isinstance(policy, dict):
            fail(f"{case_label} trusted_chunking_policy must be an object")
        require_exact_keys(policy, CHUNKING_POLICY_KEYS, f"{case_label} policy")

        producer = case.get("producer")
        if not isinstance(producer, dict):
            fail(f"{case_label} producer must be an object")
        require_exact_keys(producer, PRODUCER_KEYS, f"{case_label} producer")
        producer_certificates = producer.get("certificates")
        chunked_emission_counters = producer.get("chunked_emission_counters")
        morton_seed_counters = producer.get("morton_seed_counters")
        morton_seed_policy = producer.get("morton_seed_policy")
        producer_counters = producer.get("counters")
        if not isinstance(producer_certificates, dict):
            fail(f"{case_label} producer certificates must be an object")
        if not isinstance(producer_counters, dict):
            fail(f"{case_label} producer counters must be an object")
        if not isinstance(chunked_emission_counters, dict):
            fail(f"{case_label} chunked emission counters must be an object")
        if not isinstance(morton_seed_counters, dict):
            fail(f"{case_label} Morton seed counters must be an object")
        if not isinstance(morton_seed_policy, dict):
            fail(f"{case_label} Morton seed policy must be an object")
        require_exact_keys(
            chunked_emission_counters,
            CHUNKED_EMISSION_COUNTER_KEYS,
            f"{case_label} chunked emission counters",
        )
        require_exact_keys(
            producer_counters,
            PRODUCER_COUNTER_KEYS,
            f"{case_label} producer counters",
        )
        require_exact_keys(
            morton_seed_counters,
            MORTON_SEED_COUNTER_KEYS,
            f"{case_label} Morton seed counters",
        )
        require_exact_keys(
            morton_seed_policy,
            MORTON_SEED_POLICY_KEYS,
            f"{case_label} Morton seed policy",
        )

        rounds = case.get("rounds")
        if not isinstance(rounds, list):
            fail(f"{case_label} rounds must be an array")
        for round_index, round_value in enumerate(rounds):
            round_label = f"{case_label} round {round_index}"
            if not isinstance(round_value, dict):
                fail(f"{round_label} must be an object")
            require_exact_keys(round_value, ROUND_KEYS, round_label)
            audit = round_value.get("audit")
            contraction = round_value.get("contraction")
            decision = round_value.get("decision")
            emission_audit = round_value.get("emission_audit")
            morton_seed_audit = round_value.get("morton_seed_audit")
            if not isinstance(audit, dict):
                fail(f"{round_label} audit must be an object")
            if not isinstance(contraction, dict):
                fail(f"{round_label} contraction must be an object")
            if not isinstance(decision, dict):
                fail(f"{round_label} decision must be an object")
            if not isinstance(emission_audit, dict):
                fail(f"{round_label} emission audit must be an object")
            if not isinstance(morton_seed_audit, dict):
                fail(f"{round_label} Morton seed audit must be an object")
            require_exact_keys(audit, AUDIT_KEYS, f"{round_label} audit")
            require_exact_keys(
                emission_audit,
                EMISSION_AUDIT_KEYS,
                f"{round_label} emission audit",
            )
            require_exact_keys(
                morton_seed_audit,
                MORTON_SEED_AUDIT_KEYS,
                f"{round_label} Morton seed audit",
            )
            emission_certificates = emission_audit.get("certificates")
            if not isinstance(emission_certificates, dict):
                fail(f"{round_label} emission certificates must be an object")
            require_true_certificates(
                emission_certificates,
                EMISSION_CERTIFICATE_KEYS,
                f"{round_label} emission certificates",
            )
            require_exact_keys(
                contraction,
                CONTRACTION_KEYS,
                f"{round_label} contraction",
            )
            require_exact_keys(decision, DECISION_KEYS, f"{round_label} decision")
            audit_certificates = audit.get("certificates")
            if not isinstance(audit_certificates, dict):
                fail(f"{round_label} audit certificates must be an object")
            require_true_certificates(
                audit_certificates,
                AUDIT_CERTIFICATE_KEYS,
                f"{round_label} audit certificates",
            )

        verifier = case.get("verifier")
        if not isinstance(verifier, dict):
            fail(f"{case_label} verifier must be an object")
        require_exact_keys(verifier, VERIFIER_KEYS, f"{case_label} verifier")
        verifier_certificates = verifier.get("certificates")
        verifier_counters = verifier.get("counters")
        if not isinstance(verifier_certificates, dict):
            fail(f"{case_label} verifier certificates must be an object")
        if not isinstance(verifier_counters, dict):
            fail(f"{case_label} verifier counters must be an object")
        require_exact_keys(
            verifier_counters,
            VERIFIER_COUNTER_KEYS,
            f"{case_label} verifier counters",
        )
        validate_chunked_emission_contract(case, case_label)
        validate_morton_seed_contract(case, case_label)

    if contains_json_key(value, "public_status"):
        fail(f"{label} must not contain public_status at any depth")
    if not exact_json_equal(value, EXPECTED_REPLAY):
        fail(f"{label} differs from the closed host fixture")
    return raw, value


def validate_memcheck_log(value: str) -> None:
    summaries = [int(match) for match in SANITIZER_ERROR_RE.findall(value)]
    if not summaries or any(count != 0 for count in summaries):
        fail("memcheck evidence must contain only zero-error summaries")


def validate_racecheck_log(value: str) -> None:
    summary_lines = [
        line for line in value.splitlines() if "RACECHECK SUMMARY:" in line
    ]
    summaries = [
        tuple(int(count) for count in match.groups())
        for match in RACECHECK_SUMMARY_LINE_RE.finditer(value)
    ]
    invalid = (
        not summaries
        or len(summary_lines) != len(summaries)
        or any(summary != (0, 0, 0) for summary in summaries)
        or SANITIZER_ERROR_RE.search(value) is not None
        or RACECHECK_FAILURE_RE.search(value) is not None
    )
    if invalid:
        fail(
            "racecheck evidence must contain only zero-hazard, zero-error, "
            "zero-warning summaries"
        )


def validate_binary_logs(
    *,
    elf_log: str,
    ptx_log: str,
    memcheck_log: str,
    racecheck_log: str,
) -> list[str]:
    architectures = SM_RE.findall(elf_log)
    if not architectures or set(architectures) != {"sm_120"}:
        fail("ELF evidence must be non-empty and contain only sm_120")
    if ptx_log.strip():
        fail("PTX evidence must contain zero entries")
    validate_memcheck_log(memcheck_log)
    validate_racecheck_log(racecheck_log)
    return sorted(set(architectures))


def validate_phase3_environment(
    value: dict[str, Any],
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> dict[str, Any]:
    if value.get("schema") != PHASE3_SCHEMA:
        fail("environment artifact has the wrong Phase 3 schema")
    expected_scalars = {
        "backend": "cuda_g4",
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"environment artifact field {field} differs")
    if value.get("scientific_result_claimed") is not False:
        fail("environment artifact must not claim a scientific result")
    if value.get("scientific_public_status") is not None:
        fail("environment artifact scientific_public_status must be null")
    if "public_status" in value:
        fail("environment artifact must not expose public_status")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("environment artifact does not bind the qualified Git SHA")
    if value.get("vm_lifecycle") != WORKER_LIFECYCLE:
        fail("environment artifact does not preserve the non-mutating worker lifecycle")

    image = value.get("image")
    if not isinstance(image, dict):
        fail("environment artifact image is absent")
    if (
        image.get("base_ref") != base_image_ref
        or image.get("ref") != image_ref
        or image.get("id") != image_id
    ):
        fail("environment artifact image identity differs")

    records = value.get("runtime_records")
    if not isinstance(records, list) or not records:
        fail("environment artifact runtime_records is absent")
    first = records[0]
    if not isinstance(first, dict) or not isinstance(first.get("manifest"), dict):
        fail("environment artifact runtime manifest is absent")
    manifest = first["manifest"]
    if (
        manifest.get("compiled_sm") != "sm_120"
        or manifest.get("gpu_runtime_sm") != "sm_120"
    ):
        fail("environment artifact is not compiled and executed on sm_120")
    if (
        manifest.get("gpu_compute_capability_major"),
        manifest.get("gpu_compute_capability_minor"),
    ) != (12, 0):
        fail("environment artifact compute capability differs from 12.0")
    return manifest


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument("--replay-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--memcheck-log", type=Path, required=True)
    parser.add_argument("--racecheck-log", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_arguments(arguments)
    if SHA_RE.fullmatch(args.git_sha) is None:
        fail("--git-sha must be a canonical lowercase 40-hex commit ID")
    if IMAGE_ID_RE.fullmatch(args.image_id) is None:
        fail("--image-id must be a canonical sha256 Docker image ID")
    if args.base_image_ref != BASE_IMAGE_REF:
        fail("--base-image-ref is not the pinned CUDA image")
    if args.image_ref != f"morsehgp3d-phase3:{args.git_sha}":
        fail("--image-ref is not tied to the qualified SHA")

    _, environment = read_json_object(
        args.environment_artifact, "Phase 3 environment artifact"
    )
    manifest = validate_phase3_environment(
        environment,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    replay_raw, replay = validate_replay(args.replay_log)
    elf_log = read_text(args.elf_log, "cuobjdump ELF log")
    ptx_log = read_text(args.ptx_log, "cuobjdump PTX log", allow_empty=True)
    ptx_stderr_log = read_text(
        args.ptx_stderr_log,
        "cuobjdump PTX stderr log",
        allow_empty=True,
    )
    memcheck_log = read_text(args.memcheck_log, "memcheck log")
    racecheck_log = read_text(args.racecheck_log, "racecheck log")
    architectures = validate_binary_logs(
        elf_log=elf_log,
        ptx_log=ptx_log,
        memcheck_log=memcheck_log,
        racecheck_log=racecheck_log,
    )

    artifact = {
        "backend": "cuda_g4",
        "binary": {
            "full_replay_sha256": sha256_file(
                args.replay, "Phase 5 full-loop K1 Boruvka replay binary"
            ),
        },
        "checks": {
            "aot_elf_architectures": sorted(set(architectures)),
            "aot_ptx_entry_count": 0,
            "bounded_candidate_emission_chain_certified": True,
            "bounded_morton_seed_chain_certified": True,
            "bounded_morton_window_certified": True,
            "candidate_payload_physical_bound_certified": True,
            "canonical_contractions_certified": True,
            "complete_source_seed_coverage_certified": True,
            "complete_source_partition_certified": True,
            "cpu_exact_monotone_seed_cutoff_certified": True,
            "cpu_exact_decision_chain_certified": True,
            "external_seed_targets_recertified": True,
            "gpu_multi_round_proposal_chain_certified": True,
            "independent_chunked_gpu_replay_certified": True,
            "independent_gpu_replay_certified": True,
            "independent_morton_seed_gpu_replay_certified": True,
            "local_emst_witness_certified": True,
            "memcheck": "passed",
            "racecheck": "passed",
            "replay": replay,
        },
        "environment": {
            "compute_capability": "12.0",
            "driver_version": manifest.get("cuda_driver_version_string"),
            "gpu_name": manifest.get("gpu_name"),
            "gpu_uuid": manifest.get("gpu_uuid"),
            "gpu_vram_bytes": manifest.get("gpu_vram_bytes"),
        },
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "logs": {
            "cuobjdump_elf": elf_log,
            "cuobjdump_ptx": ptx_log,
            "cuobjdump_ptx_stderr": ptx_stderr_log,
            "full_replay": replay_raw,
            "memcheck": memcheck_log,
            "racecheck": racecheck_log,
        },
        "mode": "certified",
        "phase": "5",
        "profile": "hgp_reduced",
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": sha256_file(
                args.environment_artifact, "Phase 3 environment artifact"
            ),
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": dict(WORKER_LIFECYCLE),
    }

    output = args.output
    if not output.is_absolute():
        fail("--output must be absolute")
    if output.is_symlink() or not output.exists() or not output.is_file():
        fail("--output must be the regular temporary file reserved by the worker")
    encoded = json.dumps(
        artifact, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    )
    try:
        with output.open("w", encoding="utf-8", newline="\n") as stream:
            stream.write(encoded)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
    except OSError as error:
        fail(f"cannot write Phase 5 qualification artifact {output}: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
