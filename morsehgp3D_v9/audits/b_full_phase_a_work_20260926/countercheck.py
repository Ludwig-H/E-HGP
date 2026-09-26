#!/usr/bin/env python3
"""Independent exhaustive fixture enumeration for the finite event model.

This imports its two algorithms unchanged. It is not a third reconstruction
algorithm, a test of product geometry, or a parallel implementation.
"""
from hashlib import sha256
import importlib.util
import itertools
import json
from pathlib import Path


def check():
    path = Path(__file__).with_name("event_model.py")
    source_before = path.read_bytes()
    spec = importlib.util.spec_from_file_location("phase_a_countercheck_model", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("event_model_loader_missing")
    model = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(model)
    levels = [0, 1, 1, 2, 3]
    choices = []
    for index, level in enumerate(levels):
        allowed = [j for j in range(index) if levels[j] < level]
        options = []
        for bits in range(1 << len(allowed)):
            targets = [j for bit, j in enumerate(allowed) if (bits >> bit) & 1]
            for contribution in ((False, True) if targets else (True,)):
                options.append((level, targets, contribution))
        choices.append(options)
    fixtures = comparisons = 0
    for blocks in itertools.product(*choices):
        expected = model.reference(blocks)
        for root_high in (False, True):
            observed, _ = model.candidate(blocks, root_high=root_high)
            if observed != expected:
                raise RuntimeError("event_counterexample: " + repr((blocks, root_high)))
            comparisons += 1
        fixtures += 1
    if fixtures != 4185 or comparisons != 8370:
        raise RuntimeError("exhaustive_enumeration_cardinality_changed")
    if path.read_bytes() != source_before:
        raise RuntimeError("event_model_changed_during_check")
    return {
        "schema": "mhgp9_phase_a_event_exhaustive_countercheck_v1",
        "status": "pass",
        "scope": "finite_event_model_only",
        "event_model_sha256": sha256(source_before).hexdigest(),
        "countercheck_sha256": sha256(Path(__file__).read_bytes()).hexdigest(),
        "levels": levels,
        "options_per_block": [len(items) for items in choices],
        "fixtures": fixtures,
        "rootings_each": 2,
        "exact_result_comparisons": comparisons,
        "fields_compared": ["nodes", "successors", "actions", "anchors"],
        "enumeration": "all strictly earlier target subsets and legal contribution booleans",
        "independent_fixture_enumeration": True,
        "third_independent_reconstruction_algorithm": False,
        "geometric_catalogue_tested": False,
        "FULL_product_tested": False,
        "parallel_implementation_tested": False,
        "GPU_used": False,
        "global_subquadratic_claim": False,
    }


if __name__ == "__main__":
    print(json.dumps(check(), sort_keys=True, indent=2))
