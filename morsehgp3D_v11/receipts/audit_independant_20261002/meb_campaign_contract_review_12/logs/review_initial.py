#!/usr/bin/env python3
"""Frozen MEB protocol controls only; no native executable, build, or cloud call."""
import copy
import hashlib
import importlib.util
import json
from math import comb
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'sources/morsehgp3D_v11'
sys.path.insert(0, str(SOURCE / 'bench'))
import meb_collector_test as fixtures
import meb_semantic as semantic


def require(condition, label):
    if not condition:
        raise ValueError(label)


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def main():
    before = json.loads((HERE / 'sources_before.json').read_text())
    files = before['files']
    for item in files:
        raw = (HERE / item['copy']).read_bytes()
        require(len(raw) == item['bytes'] and hashlib.sha256(raw).hexdigest() == item['sha256'],
                'frozen_source_changed:' + item['copy'])
    reader = module('frozen_meb_reader', HERE / 'reader_copies/morsehgp3D_v11/receipts/meb_20261002/check.py')
    require(reader.SOURCE_COMMIT is None, 'snapshot_reader_not_pending')
    positive = 0
    for bits in (18, 21, 24):
        _, events = fixtures.fixture(bits)
        row = dict(events=events, count=12, coord_bits=bits)
        require(reader.events(row) == semantic.validate(events, bits, 12), 'reader_driver_disagreement')
        positive += 1
    rejected = []
    _, events = fixtures.fixture()
    changes = {
        'missing_query': lambda values: values.pop(2),
        'reference_false': lambda values: values[2].update(reference_ok=False),
        'bool_count': lambda values: values[2]['meb_logical'].update(containing=True),
        'u64_overflow': lambda values: values[2].update(meb_ns=2**64),
        'one_census_pass': lambda values: values[2]['census_logical'].update(passes=1),
        'wrong_threshold': lambda values: values[2].update(threshold=0),
        'wrapper_one_population': lambda values: values[2].update(wrapper_peak_bytes=468),
        'wrong_summary': lambda values: values[-2].update(meb_ns=97),
        'weights_not_sites': lambda values: values[0].update(points=13),
    }
    for name, change in changes.items():
        values = copy.deepcopy(events)
        change(values)
        row = dict(events=values, count=12, coord_bits=18)
        for judge in (lambda: reader.events(row), lambda: semantic.validate(values, 18, 12)):
            try:
                judge()
            except (ValueError, reader.old.foundation.Refusal):
                pass
            else:
                raise ValueError('corruption_accepted:' + name)
        rejected.append(name)
    manifest = json.loads((SOURCE / 'tests/mutants/tower.json').read_text())
    cmake = (SOURCE / 'tests/tower/tests.cmake').read_text()
    mutations = []
    for mutation in manifest['mutants']:
        text = (SOURCE / mutation['fichier']).read_text()
        count = text.count(mutation['cherche'])
        require(count == 1, 'mutation_match_not_unique:' + mutation['id'])
        gate = mutation['porte']
        if gate.startswith('mhgp11_tower_unit_'):
            require(gate.removeprefix('mhgp11_tower_unit_') in cmake.split(), 'unit_gate_absent:' + gate)
        else:
            require(gate in cmake, 'mutation_gate_absent:' + gate)
        mutations.append(dict(id=mutation['id'], search_matches=count, gate=gate))
    require(manifest['plancher'] == len(mutations) == 10, 'mutation_floor')
    part_checks = 0
    for count in (12, 13, 16, 31, 48, 8000, 16000, 32000, 39885, 35551, 45845):
        for ordinal in range(48):
            selected = semantic.part(ordinal, count)
            require(len(selected) == ordinal % 12 + 1 and selected == sorted(set(selected)) and
                    all(0 <= site < count for site in selected), 'selection_domain')
            part_checks += 1
    first_meb_presentations = 4 * sum(sum(comb(size, q) for q in range(1, min(size, 4) + 1))
                                     for size in range(1, 13))
    output = dict(scope='pure_protocol_only', native=0, frozen_sources=len(files),
                  reader_pending_source_commit=True, reader_driver_positive_profiles=positive,
                  both_judges_corruptions_rejected=rejected, mutation_applicability_only=mutations,
                  part_selection_checks=part_checks, first_meb_presentations_per_run=first_meb_presentations,
                  with_wrapper_repetition_presentations_per_run=2*first_meb_presentations)
    print(json.dumps(output, sort_keys=True))


if __name__ == '__main__':
    main()
