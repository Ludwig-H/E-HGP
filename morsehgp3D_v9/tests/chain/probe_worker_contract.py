#!/usr/bin/env python3
"""Porte de raccord : la VRAIE sonde v9 jugee par le validateur du worker G4.

La session G4 R2 (23 septembre 2026) a refuse ses treize sorties parce que le
selftest du protocole jugeait une fausse sonde qui omettait deux champs MEB
(contre-audit B du meme jour). Cette porte lance mhgp9_tower_probe avec
EXACTEMENT la queue d'arguments d'un cas G4 (tower_worker_v9.expected_probe_tail)
sur un petit nuage deterministe, puis exige :

- que validate_probe du worker accepte la sortie reelle (complete_relative)
  et que validate_external_wall tienne contre le mur mesure ici ;
- que les deux voies geometriques epinglees (saturation, census q3 sur
  feuille) soient publiees telles que demandees et donnent le meme objet
  (catalogue, ordres, condense) que les voies eteintes ;
- que des mutants de schema soient refuses : champ texte ou tableau
  inattendu, histogramme MEB malforme, comptabilite MEB non epinglee, mode
  retourne.

Codes : 0 conforme, 1 desaccord, 2 refus avant calcul. Aucun assert : la porte
tient sous python3 -O. Ce petit nuage est un juge de raccord, pas une mesure.
"""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import time


def load_worker(path):
    spec = importlib.util.spec_from_file_location('mhgp9_tower_worker_under_gate', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def cloud(n, seed):
    """Nuage u18 deterministe (LCG 64 bits), trois grappes et un bruit."""
    state = seed
    out = bytearray()
    for i in range(n):
        centre = (i % 3) * 40000 + 20000
        coords = []
        for _ in range(3):
            state = (state * 6364136223846793005 + 1442695040888963407) % (1 << 64)
            coords.append(centre + (state >> 40) % 9000)
        out += struct.pack('<3I', *coords)
    return bytes(out)


def main(argv):
    if len(argv) != 4:
        print('usage: probe_worker_contract.py <mhgp9_tower_probe> <tower_worker_v9.py> <workdir>', file=sys.stderr)
        return 2
    probe, worker_path, workdir = Path(argv[1]), Path(argv[2]), Path(argv[3])
    if not probe.is_file() or not worker_path.is_file():
        print('refusal: probe or worker absent', file=sys.stderr)
        return 2
    worker = load_worker(worker_path)
    workdir.mkdir(parents=True, exist_ok=True)
    raw = cloud(360, 3)
    data_file = workdir / 'probe_worker_contract.u32le'
    data_file.write_bytes(raw)
    inputs = {'gate': dict(n=len(raw) // 12, fnv=worker.input_fnv(raw))}
    failures = []

    def check(ok, label):
        if not ok:
            failures.append(label)
        return ok

    def run(case):
        argv_probe = [str(probe), str(data_file)] + worker.expected_probe_tail(case)
        started = time.monotonic()
        done = subprocess.run(argv_probe, capture_output=True, timeout=600)
        elapsed = time.monotonic() - started
        value = worker.strict_json(done.stdout)
        return value, done.returncode, elapsed

    base = dict(scene='gate', file=data_file.name, n=inputs['gate']['n'], k=5, s=8, workers=2, static_threads=2,
                saturate_deep=True, q3_leaf=True, repeat=0)
    results = {}
    for label, case in (('pinned_on', base),
                        ('pinned_off', dict(base, saturate_deep=False, q3_leaf=False, workers=1, static_threads=0))):
        try:
            value, code, elapsed = run(case)
            outcome = worker.validate_probe(value, case, code, inputs=inputs)
            worker.validate_external_wall(value, elapsed)
        except (ValueError, KeyError, TypeError, UnicodeError, subprocess.TimeoutExpired) as error:
            check(False, label + ': real probe refused by the worker validator: ' + type(error).__name__ + ': ' +
                  str(error))
            continue
        check(outcome == 'complete_relative', label + ': outcome ' + outcome)
        check(value['options']['atlas_saturate_deep'] is case['saturate_deep'] and
              value['options']['q3_leaf_census'] is case['q3_leaf'], label + ': published modes')
        results[label] = (case, value)
    if len(results) == 2:
        on, off = results['pinned_on'][1], results['pinned_off'][1]
        check(worker.logical_result(on) == worker.logical_result(off), 'modes on/off change the object')
        check(len(on['orders']) == 5 and on['catalogue']['balls'] >= 1000,
              'coverage floor: 5 orders and >= 1000 catalogue balls, got ' +
              str(len(on['orders'])) + ' / ' + str(on['catalogue']['balls']))
        case, good = results['pinned_on']
        mutants = [
            ('tower_work text field', lambda v: v['tower_work'].update(selftest_note='x')),
            ('tower_work list field', lambda v: v['tower_work'].update(records=[1])),
            ('meb histogram negative', lambda v: v['tower_work'].update(meb_supports_by_size=[0, -1])),
            ('meb histogram text', lambda v: v['tower_work'].update(meb_supports_by_size='0,1')),
            ('meb accounting unpinned', lambda v: v['tower_work'].update(meb_accounting='other_v3')),
            ('meb accounting absent', lambda v: v['tower_work'].pop('meb_accounting')),
            ('saturation mode flipped', lambda v: v['options'].update(atlas_saturate_deep=False)),
            ('leaf mode flipped', lambda v: v['options'].update(q3_leaf_census=False)),
            ('leaf mode absent', lambda v: v['options'].pop('q3_leaf_census')),
            ('stage times beyond total', lambda v: v['times_ms'].update(q34=v['times_ms']['chain_total'] + 60.0)),
        ]
        killed = 0
        for label, mutate in mutants:
            bad = copy.deepcopy(good)
            mutate(bad)
            try:
                worker.validate_probe(bad, case, 0, inputs=inputs)
            except (ValueError, KeyError, TypeError):
                killed += 1
                continue
            check(False, 'mutant accepted: ' + label)
        try:
            worker.validate_external_wall(good, good['times_ms']['chain_total'] / 1000.0 - 5.0)
            check(False, 'mutant accepted: external wall shorter than the chain total')
        except ValueError:
            killed += 1
        print('probe_worker_contract mutants_killed=' + str(killed) + '/' + str(len(mutants) + 1))
    for failure in failures:
        print('FAIL ' + failure)
    print(json.dumps(dict(gate='probe_worker_contract', failures=len(failures),
                          digest=results.get('pinned_on', (None, {}))[1].get('tower_digest')), sort_keys=True))
    return 1 if failures else 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv))
