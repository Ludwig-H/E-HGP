#!/usr/bin/env python3
"""Contrelecture MES-B livree : JSON fictifs seulement ; portes Python en option."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent


def need(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=HERE.parents[3])
    parser.add_argument('--run-gates', action='store_true')
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_bytes())
    for row in (HERE / 'SHA256SUMS').read_text().splitlines():
        sha, name = row.split('  ', 1)
        need(hashlib.sha256((HERE / name).read_bytes()).hexdigest() == sha, 'recu modifie : ' + name)
    for name, sha in cap['sources_sha256'].items():
        body = (args.repo / name).read_bytes()
        need(hashlib.sha256(body).hexdigest() == sha, 'source differente : ' + name)
        git_body = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['pin'] + ':' + name])
        need(body == git_body, 'source non conforme au pin : ' + name)
    folder = args.repo / 'morsehgp3D_v12/microbancs/mes_b_scenes'
    sys.path.insert(0, str(folder))
    spec = importlib.util.spec_from_file_location('mes_b_fixture', folder / 'test_pilote_b.py')
    t = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(t)
    p = t.pilote_b
    good = t.device_output()
    samples = {
        'valid': good,
        'duplicate_cpu': good.replace('"cpu_ns": 5,', '"cpu_ns": 5, "cpu_ns": 5,', 1),
        'boolean_free_pass': good.replace('"phase": "liberation", "pass": 0,',
                                         '"phase": "liberation", "pass": false,', 1),
        'open_bad_reason': good.replace('"reason": "none", "wall_ns": 5',
                                       '"reason": "device_fault", "wall_ns": 5', 1),
        'u64_overflow': good.replace('"cpu_ns": 5,', '"cpu_ns": 18446744073709551616,', 1),
    }
    parsed = {name: p.parse_output(0, body, t.CASE, t.SITES, t.LABEL)['etat']
              for name, body in samples.items()}
    need(parsed == {name: 'ok' if name == 'valid' else 'illisible' for name in samples}, 'corruptions historiques')
    labels, used = [], set()
    for _ in range(12):
        labels.append(p.label_of('scene_synthetique_00000000000000000001', used))
    need(len(set(labels)) == 12 and max(map(len, labels)) <= 23, 'etiquettes')
    outcomes = []
    for k, sites, state, expected in [(5, 9_999_999, 'refus', 'non tenu'),
                                      (5, 10_000_000, 'refus', 'tenu'),
                                      (5, 12_000_000, 'echec', 'non tenu'),
                                      (10, 2_000_000, 'refus', 'non tenu')]:
        rows = [t.result('bonne', 1_000_000, 1),
                t.result('refusee', sites, 0, k=k, etat=state, passes=0)]
        v = p.verdicts(rows, [])
        key = 'B1' if k == 5 else 'B4'
        need(v[key]['etat'] == expected, 'regle des refus')
        outcomes.append(dict(k=k, sites=sites, state=state, criterion=key, verdict=v[key]['etat']))
    unavailable = {}
    for field in ('cpu_ns', 'rss_max_octets'):
        body = t.device_output(mutate=lambda row: row.update({field: None}))
        unavailable[field] = p.parse_output(0, body, t.CASE, t.SITES, t.LABEL)['etat']
    need(set(unavailable.values()) == {'illisible'}, 'metrique absente non refusee')

    def zero_wall(row):
        row['wall_ns'] = 0
        for name in ('etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns'):
            row[name] = dict.fromkeys(row[name], 0)

    admitted = p.parse_output(0, t.device_output(mutate=zero_wall), t.CASE, t.SITES, t.LABEL)
    need(admitted['etat'] == 'ok', 'temoin mur nul non reproduit')
    a, b = t.result('a', 1_000_000, 1), t.result('b', 2_000_000, 1)
    b['passes'] = admitted['passes']
    try:
        p.verdicts([a, b], [['a', 'b']])
    except ValueError as error:
        need(str(error) == 'math domain error', 'autre exception')
    else:
        raise ValueError('exception log(0) non reproduite')
    gates = []
    if args.run_gates:
        for row in cap['gate_results']:
            command = [sys.executable] + row['command'][1:]
            command[-1] = str(args.repo / 'morsehgp3D_v12' / command[-1])
            done = subprocess.run(command, capture_output=True, text=True, timeout=30,
                                  env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
            need(done.returncode == row['code'] and done.stdout == row['stdout'] and not done.stderr,
                 'porte Python differente')
            gates.append(dict(code=done.returncode, stdout=done.stdout.strip()))
    for name, sha in cap['sources_sha256'].items():
        need(hashlib.sha256((args.repo / name).read_bytes()).hexdigest() == sha, 'source modifiee pendant lecture')
    print(json.dumps(dict(ok=True, pins=len(cap['sources_sha256']), historical=parsed, refusals=outcomes,
                         nullable_metrics=unavailable, zero_wall=dict(admitted=True, exception='ValueError: log(0)'),
                         labels_unique=True, gates=gates, native_executed=False), sort_keys=True))


if __name__ == '__main__':
    main()
