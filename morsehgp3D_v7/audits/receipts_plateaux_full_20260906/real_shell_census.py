"""Four complete census checks using independently regenerated 50k input.

Uses CPython's MT19937 core with the C++ single-word seed expansion, not
the constructor's Python generator. No constructor code is imported.
Only these four balls are scanned; no global catalogue or parent is judged.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
from typing import Any

from shell_diagnostic import analyze, parse_records

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
CAPTURE = ROOT / 'morsehgp3D_v7/receipts/full_extra_shell_50000_20260906'
RAW_SHA = '3cd74b330c62978d8c3eedd175e12bf5fe02893facb2e008150c32b5054aea72'
ANALYZER_SHA = '1032cc5ef86da67e0064a56f68b35f0395181c16192ce92d863a1339254f426a'
INPUT_SHA = '3f7c6dd47bcba4222e511c94f90aaeeeb80198b0d5ac8a6721e4ff55feedab3f'
Point = tuple[int, int, int]
SOURCE_HASHES = {
    'bench/full_gabriel_semantic_digest.hpp': '671b2dfb51f1385ee7301bd6b03ef64e62c0d768c92534a6f09589726ce9adc3',
    'src/cloud/families.hpp': 'bff4ff92368c3ced40b0367b426d4aff2e56371896b03603b72089ef5944ed11',
    'src/core/morton.hpp': '67e9f2bc5d388f995977aac3d2191800f5d7065b4db3e2506717c8e85590406e',
    'src/core/sha256.hpp': 'b675835e9bd05a0ca1fb63dfeaa6fa99d3521b761c35c50b1b51f0aac259798d',
    'src/tree/cloud_index.hpp': '8c5acf166ce378b0271e15850c54ca1740a8f6cb899d34a60c832a533504ad95',
}


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generator(seed: int) -> random.Random:
    # std::mt19937 scalar initialization; Random.seed(seed) is NOT equivalent.
    words = [seed]
    for index in range(1, 624):
        previous = words[-1]
        words.append((1812433253 * (previous ^ (previous >> 30)) + index) % (1 << 32))
    result = random.Random(0)
    result.setstate((3, tuple(words + [624]), None))
    return result


def regenerate() -> tuple[list[Point], int]:
    reference = generator(5489)
    need([reference.getrandbits(32) for _ in range(10)] ==
         [3499211612, 581869302, 3890346734, 3586334585, 545404204,
          4161255391, 3922919429, 949333985, 2715962298, 1323567403],
         'MT19937_reference_vector')
    rng = generator(3)
    points: list[Point] = []
    unique: set[Point] = set()
    for attempts in range(1, 200 * 50000 + 1):
        # Multiply-high downscaling by 65536 has zero rejection threshold.
        point = (rng.getrandbits(32) >> 16, rng.getrandbits(32) >> 16,
                 rng.getrandbits(32) >> 16)
        if point not in unique:
            unique.add(point)
            points.append(point)
            if len(points) == 50000:
                return points, attempts
    raise ValueError('input_generation_guard')


def labelled_digest(points: list[Point]) -> str:
    tag = b'mhgp7-full-semantic-v1:input'
    digest = hashlib.sha256(struct.pack('<Q', len(tag)) + tag + struct.pack('<Q', len(points)))
    for point_id, point in enumerate(points):
        digest.update(struct.pack('<QQQQ', point_id, *point))
    return digest.hexdigest()


def morton_ranks(points: list[Point]) -> tuple[list[int], str]:
    # Byte lookup expansion, independent of the product's staged bit masks.
    spread = [sum(((value >> bit) & 1) << (3 * bit) for bit in range(8))
              for value in range(256)]

    def key(point: Point) -> int:
        return sum((spread[value & 255] | (spread[value >> 8] << 24)) << axis
                   for axis, value in enumerate(point))

    order = sorted(range(len(points)), key=lambda point_id: (key(points[point_id]), point_id))
    ranks = [0] * len(points)
    digest = hashlib.sha256(b'audit-morton-point-id-permutation-v1\0')
    for geometry_index, point_id in enumerate(order):
        ranks[point_id] = geometry_index
        digest.update(struct.pack('<Q', point_id))
    return ranks, digest.hexdigest()


def scan(record: dict[str, Any], points: list[Point]) -> dict[str, Any]:
    raw_key = record['ball_key']
    a, c = int(raw_key['a']), int(raw_key['c'])
    b = tuple(map(int, raw_key['b']))
    radius_scaled = sum(value * value for value in b) - 4 * a * c
    interior, shell = [], []
    exterior_margin: int | None = None
    interior_margin: int | None = None
    for point_id, point in enumerate(points):
        # Distance to the centre in coordinates multiplied by 2A; no
        # polynomial-power helper or spatial pruning from the constructor.
        difference = sum((2 * a * x + y) ** 2 for x, y in zip(point, b)) - radius_scaled
        if difference < 0:
            interior.append(point_id)
            interior_margin = difference if interior_margin is None else max(interior_margin, difference)
        elif difference == 0:
            shell.append(point_id)
        else:
            exterior_margin = difference if exterior_margin is None else min(exterior_margin, difference)
    return dict(interior_point_ids=interior, shell_point_ids=shell,
                exterior_count=len(points) - len(interior) - len(shell),
                minimum_exterior_scaled_distance_margin=exterior_margin,
                maximum_interior_scaled_distance_margin=interior_margin,
                points_tested=len(points))


def bind(record: dict[str, Any], points: list[Point], ranks: list[int],
         census: dict[str, Any]) -> None:
    need(record['n'] == len(points) and record['input_digest'] == INPUT_SHA, 'whole_input_binding')
    for population in ('interior', 'shell'):
        point_ids = []
        for row in record[population]:
            point_id = row['point_id']
            need(type(point_id) is int and 0 <= point_id < len(points), 'campaign_point_id')
            need(tuple(row['xyz']) == points[point_id], 'coordinate_binding')
            need(row['geometry_index'] == ranks[point_id], 'Morton_geometry_index_binding')
            point_ids.append(point_id)
        need(sorted(point_ids) == census[population + '_point_ids'], 'complete_' + population + '_census')


def mutants(record: dict[str, Any], points: list[Point], ranks: list[int],
            census: dict[str, Any]) -> list[dict[str, str]]:
    cases: list[tuple[str, str, dict[str, Any]]] = []
    missing_interior = copy.deepcopy(record)
    missing_interior['interior'].pop()
    cases.append(('omit_strict_interior', 'complete_interior_census', missing_interior))
    missing_shell = copy.deepcopy(record)
    missing_shell['shell'].pop()
    cases.append(('omit_shell_site', 'complete_shell_census', missing_shell))
    wrong_coordinate = copy.deepcopy(record)
    wrong_coordinate['shell'][0]['xyz'][0] ^= 1
    cases.append(('change_coordinate', 'coordinate_binding', wrong_coordinate))
    wrong_index = copy.deepcopy(record)
    wrong_index['shell'][0]['geometry_index'] ^= 1
    cases.append(('change_Morton_rank', 'Morton_geometry_index_binding', wrong_index))
    wrong_input = copy.deepcopy(record)
    wrong_input['input_digest'] = '0' * 64
    cases.append(('change_input_digest', 'whole_input_binding', wrong_input))
    results = []
    for name, reason, changed in cases:
        try:
            bind(changed, points, ranks, census)
        except ValueError as error:
            need(str(error) == reason, 'mutant_specific_reason:' + name)
            results.append(dict(mutant=name, rejection=reason))
        else:
            raise ValueError('mutant_survived:' + name)
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stderr', type=Path, default=CAPTURE / 'run_r3/n50000_k10.stderr')
    arguments = parser.parse_args()
    need(sha(HERE / 'shell_diagnostic.py') == ANALYZER_SHA, 'local_analyzer_pin')
    references = {}
    for name, pin in SOURCE_HASHES.items():
        path = CAPTURE.parent / 'full_pipeline_threads_micro_20260906/sources/morsehgp3D_v7' / name
        need(sha(path) == pin, 'recipe_source_pin:' + name)
        references[str(path.relative_to(ROOT))] = pin
    raw = arguments.stderr.read_bytes()
    need(hashlib.sha256(raw).hexdigest() == RAW_SHA, 'four_diagnostic_capture_pin')
    lines = raw.decode().split('\n')
    need(all(line.startswith('{') for line in lines[:4]) and
         not any(line.lstrip().startswith('{') for line in lines[4:]), 'exact_four_record_prefix')
    records = parse_records('\n'.join(lines[:4]))
    need(len(records) == 4 and [r['ball_index'] for r in records] ==
         [174406, 254569, 996863, 1251653], 'fixed_diagnostic_indices')
    analyses = [analyze(record) for record in records]
    points, attempts = regenerate()
    need(labelled_digest(points) == INPUT_SHA, 'regenerated_complete_input_digest')
    ranks, permutation_digest = morton_ranks(points)
    results = []
    for record, local in zip(records, analyses):
        census = scan(record, points)
        bind(record, points, ranks, census)
        results.append(dict(ball_index=record['ball_index'], complete_census_verified=True,
                            coordinate_and_Morton_bindings_verified=True,
                            census=census, local_analysis=local))
    rejected = mutants(records[0], points, ranks, results[0]['census'])
    output = dict(status='passed', public_status='not_claimed', diagnostic_only=True,
        script_sha256=sha(Path(__file__)), local_analyzer_sha256=ANALYZER_SHA,
        source_stderr_sha256=RAW_SHA, source_recipe_references=references,
        independence=dict(input_generator='CPython_MT19937_explicit_scalar_seed_state_high16',
            constructor_reader_imported=False, product_geometry_or_index_imported=False,
            Morton='byte_lookup_bit_interleaving_and_full_sort',
            census='four_full_scans_of_integer_scaled_squared_distances',
            local_geometry='auditor_shell_diagnostic_Gram_and_boolean_shell_quotient'),
        input=dict(n=len(points), coord=65536, seed=3, digest=INPUT_SHA,
                   generated_triples=attempts, duplicate_triples_skipped=attempts - len(points),
                   MT19937_words_consumed=3 * attempts, Morton_permutation_digest=permutation_digest),
        complete_census_verified_for_supplied_balls=True, scanned_balls=4,
        scanned_point_ball_pairs=4 * len(points), global_catalogue_completeness_verified=False,
        global_parents_verified=False, full_tower_verified=False,
        local_analysis_retains_its_local_only_scope=True,
        records=results, targeted_mutants=rejected, engine_executed=False, GCP_used=False)
    print(json.dumps(output, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
