#!/usr/bin/env python3
"""Porte de raccord : la VRAIE sonde v9 jugee par le validateur du worker G4.

La session G4 R2 (23 septembre 2026) a refuse ses treize sorties parce que le
selftest du protocole jugeait une fausse sonde qui omettait deux champs MEB
(contre-audit B du meme jour). Cette porte lance mhgp9_tower_probe avec
EXACTEMENT la queue d'arguments d'un cas G4 (tower_worker_v9.expected_probe_tail)
sur un petit nuage deterministe, puis exige :

- que validate_probe du worker accepte la sortie reelle (complete_relative)
  et que validate_external_wall tienne contre le mur mesure ici ;
- que tous les leviers epingles (saturation, census q3 sur feuille,
  certificat de voie morte, cache des temoins) soient publies tels que
  demandes et donnent le meme objet (catalogue, ordres, condense) que les
  leviers eteints ;
- que des mutants de schema soient refuses : champ texte ou tableau
  inattendu, histogramme MEB malforme, comptabilite MEB non epinglee, mode
  retourne.

Codes : 0 conforme, 1 desaccord, 2 refus avant calcul. Aucun assert : la porte
tient sous python3 -O. Ce petit nuage est un juge de raccord, pas une mesure.
"""
import copy
import importlib.util
import json
import os
from pathlib import Path
import signal
import struct
import subprocess
import sys
import time

# Delai interne inferieur au delai CTest (300 s) : a l'expiration, le groupe
# de processus de la sonde est tue, jamais laisse orphelin.
PROBE_TIMEOUT_SECONDS = 240


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
        process = subprocess.Popen(argv_probe, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
        try:
            stdout, _ = process.communicate(timeout=PROBE_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
            raise
        elapsed = time.monotonic() - started
        value = worker.strict_json(stdout)
        return value, process.returncode, elapsed

    base = dict(scene='gate', file=data_file.name, n=inputs['gate']['n'], k=5, s=8, workers=2, static_threads=2,
                levers={name: True for name in worker.LEVER_NAMES}, repeat=0)
    results = {}
    for label, case in (('pinned_on', base),
                        ('pinned_off', dict(base, levers={name: False for name in worker.LEVER_NAMES}, workers=1,
                                            static_threads=0))):
        try:
            value, code, elapsed = run(case)
            outcome = worker.validate_probe(value, case, code, inputs=inputs)
            worker.validate_external_wall(value, elapsed)
        except (ValueError, KeyError, TypeError, UnicodeError, subprocess.TimeoutExpired) as error:
            check(False, label + ': real probe refused by the worker validator: ' + type(error).__name__ + ': ' +
                  str(error))
            continue
        check(outcome == 'complete_relative', label + ': outcome ' + outcome)
        check(value['options']['levers'] == case['levers'], label + ': published levers')
        results[label] = (case, value)
    if len(results) == 2:
        on, off = results['pinned_on'][1], results['pinned_off'][1]
        check(worker.logical_result(on) == worker.logical_result(off), 'modes on/off change the object')
        # Non-vacuite : les trois voies epinglees sont reellement exercees.
        ledger = on['ledger']
        check(ledger['q3_leaf_censuses'] > 0 and ledger['atlas_deep_cells'] > 0 and ledger['dead_q3_proved'] > 0 and
              ledger['dead_q4_proved'] > 0 and ledger['dead_q3_open'] > 0 and ledger['dead_q4_open'] > 0,
              'pinned modes not exercised: ' + json.dumps({key: ledger[key] for key in (
                  'q3_leaf_censuses', 'atlas_deep_cells', 'dead_q3_proved', 'dead_q4_proved', 'dead_q3_open',
                  'dead_q4_open')}, sort_keys=True))
        check(off['ledger']['dead_loads'] == 0 and off['ledger']['q3_leaf_censuses'] == 0 and
              off['ledger']['witness_cache_queries'] == 0 and ledger['witness_cache_rejected_pairs'] > 0,
              'levers off still ran, or the witness cache never rejected a pair')
        # Noyau diametral : des aretes closes par lui, des voies des deux
        # sortes prouvees par lui, et rien quand son levier est coupe.
        check(ledger['core_closed_edges'] > 0 and ledger['dead_core_q3_proved'] > 0 and
              ledger['dead_core_q4_proved'] > 0 and off['ledger']['core_builds'] == 0,
              'diametral core not exercised: ' + json.dumps({key: ledger[key] for key in (
                  'core_builds', 'core_closed_edges', 'dead_core_q3_proved', 'dead_core_q4_proved')}, sort_keys=True))
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
            ('saturation lever flipped', lambda v: v['options']['levers'].update(atlas_saturate_deep=False)),
            ('leaf lever flipped', lambda v: v['options']['levers'].update(q3_leaf_census=False)),
            ('leaf lever absent', lambda v: v['options']['levers'].pop('q3_leaf_census')),
            ('dead-lane lever flipped', lambda v: v['options']['levers'].update(q34_dead_lanes=False)),
            ('witness-cache lever flipped', lambda v: v['options']['levers'].update(q34_witness_cache=False)),
            ('core lever flipped', lambda v: v['options']['levers'].update(q34_dead_core=False)),
            ('core closure uncounted', lambda v: v['ledger'].update(core_closed_edges=0)),
            ('core cover visits hidden', lambda v: v['ledger'].update(core_cover_node_visits=0)),
            ('cache rejections without queries', lambda v: v['ledger'].update(witness_cache_queries=0)),
            ('both-lane edges beyond q3', lambda v: v['ledger'].update(both_edges=v['ledger']['q3_edges'] + 1)),
            ('shell above 12 under complete', lambda v: v['catalogue'].update(shell_over_12=1)),
            ('stage times beyond total', lambda v: v['times_ms'].update(q34=v['times_ms']['chain_total'] + 60.0)),
            ('meb histogram short', lambda v: v['tower_work'].update(meb_supports_by_size=[0])),
            ('tower_work unknown integer', lambda v: v['tower_work'].update(extra=1)),
            ('tower_work missing records', lambda v: v['tower_work'].pop('records')),
            ('generator missing field', lambda v: v['generator'].pop('q34_expanded_pairs')),
            ('ledger missing field', lambda v: v['ledger'].pop('q3_seeds')),
            ('by_qmin empty', lambda v: v['catalogue'].update(by_qmin=[])),
            ('by_shell empty', lambda v: v['catalogue'].update(by_shell=[])),
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
