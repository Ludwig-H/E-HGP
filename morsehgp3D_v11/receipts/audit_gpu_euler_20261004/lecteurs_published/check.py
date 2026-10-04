#!/usr/bin/env python3
"""Contrelecture statique et modèle scalaire, sans import ni exécution du produit.

Le contrôle porte sur les corrections de diagnostic de d5, et ne rejoue pas
la porte produit full_pipeline_reader_test. Chaque calendrier scalaire a des
débuts et fins de même tâche ordonnés ; aucune barrière ne relie les voies.
La somme des phases R, P-R, V-P ne mesure pas les durées des tâches qui se
chevauchent. Une queue nulle ne détermine pas la fin d'une tâche ; les nouvelles
extrémités permettent désormais le calcul end-start pour cette tâche aussi.
"""
import ast
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CHECKS = 0


def check(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(reason)


def assigned(module, name):
    for node in module.body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == name for t in node.targets):
            return ast.literal_eval(node.value)
    raise ValueError('assignment missing: ' + name)


def comparison(module, fields):
    for node in ast.walk(module):
        if isinstance(node, ast.Compare):
            names = {n.slice.value for n in ast.walk(node) if isinstance(n, ast.Subscript)
                     and isinstance(n.slice, ast.Constant) and isinstance(n.slice.value, str)}
            if fields <= names:
                return True
    return False


def old_lane_model(starts, ends, wall):
    return max(starts) <= min(ends) <= wall


def published_lane_model(starts, ends, wall):
    return max(starts) <= wall and min(ends) <= max(ends) <= wall


def main():
    metadata = json.loads((ROOT / 'SOURCE.json').read_text())
    for entry in metadata['files']:
        raw = (ROOT / 'sources' / entry['revision'] / entry['path']).read_bytes()
        check(hashlib.sha256(raw).hexdigest() == entry['sha256'], 'source hash')
        check(len(raw) == entry['bytes'], 'source bytes')
        if entry['revision'] == 'published':
            check(entry['same_at_latest'] and entry['latest_sha256'] == entry['sha256'], 'publication reconciliation')
    base = ROOT / 'sources/published/morsehgp3D_v11'
    current = ast.parse((base / 'bench/full_acceleration_diagnostics.py').read_text())
    historical = ast.parse((ROOT / 'sources/historical/morsehgp3D_v11/bench/full_acceleration_diagnostics.py').read_text())
    gate = ast.parse((base / 'tests/tower/full_pipeline_reader_test.py').read_text())
    for relative in ('bench/ab_g4.py', 'bench/ab_summary.py'):
        ast.parse((base / relative).read_text())
        check(True, 'Python source parses without execution')
    lane_fields = assigned(current, 'PIPELINE_LANES')
    order_fields = assigned(current, 'PIPELINE_ORDER')
    check(lane_fields - assigned(historical, 'PIPELINE_LANES') == {'lanes_last_finish_ns'}, 'lane end field')
    check(order_fields - assigned(historical, 'PIPELINE_ORDER') == {'publish_end_ns', 'vertical_end_ns'}, 'task end fields')
    check(set(assigned(current, 'VERTICAL_TASK')) == {'vertical_start_ns', 'vertical_end_ns', 'vertical_cpu_ns', 'vertical_wait_ns'}, 'order-one empty vertical task')
    contested = {'lanes_last_start_ns', 'lanes_first_finish_ns'}
    check(comparison(historical, contested), 'historical overlap condition exists')
    check(not comparison(current, contested), 'published no global overlap condition')
    check(comparison(current, {'lanes_first_finish_ns', 'lanes_last_finish_ns', 'forest_ns'}), 'published lane endpoint ordering')
    for kind in ('publish', 'vertical'):
        check(comparison(current, {kind + '_start_ns', kind + '_end_ns', 'forest_ns'}), 'same-task endpoint ordering')
    pipeline = (base / 'src/tower/forest_pipeline.cpp').read_text()
    bridge = (base / 'bench/full_probe.cpp').read_text()
    for field in ('publish_end_ns', 'vertical_end_ns'):
        check('o.' + field + ' = end' in pipeline, 'record actual same-task finish')
        check('t.' + field in bridge, 'JSON exports endpoint')
    check('timings.lanes_last_finish_ns = resolved;' in pipeline, 'lane last finish is R')
    check('forest_timings.lanes_last_finish_ns' in bridge, 'JSON exports last lane finish')
    main_node = next(n for n in gate.body if isinstance(n, ast.FunctionDef) and n.name == 'main')
    check(any(isinstance(n, ast.Compare) and isinstance(n.left, ast.Name) and n.left.id == 'CHECKS'
              and any(isinstance(v, ast.Constant) and v.value == 17 for v in n.comparators)
              for n in ast.walk(main_node)), 'gate declares seventeen checks, not executed')
    registered = (base / 'tests/tower/tests.cmake').read_text()
    check('mhgp11_tower_full_pipeline_reader' in registered and 'checks17' in registered, 'target registered')
    calendars = []
    for lanes in range(1, 9):
        starts = [3 * i + 1 for i in range(lanes)]
        ends = [v + 1 for v in starts]
        wall = max(ends) + 7
        check(all(s <= e <= wall for s, e in zip(starts, ends)), 'valid private calendar')
        check(published_lane_model(starts, ends, wall), 'no-barrier calendar remains admitted')
        check(old_lane_model(starts, ends, wall) == (lanes == 1), 'old condition falsely refuses separated lanes')
        check(not published_lane_model(starts, ends, max(ends) - 1), 'wall endpoint still bounded')
        calendars.append({'lanes': lanes, 'last_start': max(starts), 'first_finish': min(ends),
                          'last_finish': max(ends), 'wall': wall,
                          'historical_admits': old_lane_model(starts, ends, wall),
                          'published_model_admits': published_lane_model(starts, ends, wall)})
    starts, ends, wall = [1, 1], [5, 7], 10
    check(old_lane_model(starts, ends, wall) and published_lane_model(starts, ends, wall), 'overlapping calendars remain admitted')
    start, r, possible_ends = 2, 20, [7, 19]
    tails = [max(0, end - r) for end in possible_ends]
    durations = [end - start for end in possible_ends]
    check(tails == [0, 0] and durations == [5, 17], 'same zero tail conceals distinct old durations')
    check(all(start + duration == end for duration, end in zip(durations, possible_ends)), 'new endpoints determine elapsed time')
    r, publication_ends, vertical_ends = 20, [7, 25], [19, 30]
    p, v = max([r] + publication_ends), max([r] + publication_ends + vertical_ends)
    check([r, p - r, v - p] == [20, 5, 5], 'same disjoint phase construction')
    check(r + (p - r) + (v - p) == v, 'phases telescope, not task durations')
    output = {'verdict': 'conforme_modele_statique', 'checks': CHECKS, 'native_executed': False,
              'product_gates_executed': False, 'historical_finding': 'e49 overlap rejection corrected in source d5',
              'calendars': calendars,
              'zero_tail': {'start': start, 'R': 20, 'ends': possible_ends, 'tails': tails, 'durations': durations},
              'scope': 'AST inspection and independent scalar chronology only; no native qualification or timing gain'}
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
