#!/usr/bin/env python3
"""Relecture locale des plans L/L1/L2 et de la provenance L1. Aucun appel distant."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def hashed_json(path, sha):
    body = path.read_bytes()
    need(hashlib.sha256(body).hexdigest() == sha, 'artefact different : ' + path.name)
    return json.loads(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--session-dir', type=Path, required=True)
    parser.add_argument('--plans-dir', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_bytes())
    for row in (HERE / 'SHA256SUMS').read_text().splitlines():
        sha, name = row.split('  ', 1)
        need(hashlib.sha256((HERE / name).read_bytes()).hexdigest() == sha, 'recu modifie : ' + name)
    for name, sha in cap['source_sha256'].items():
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['source_pin'] + ':' + name])
        need(hashlib.sha256(body).hexdigest() == sha, 'source Git differente : ' + name)
    pre = hashed_json(args.session_dir / 'preflight.json', cap['preflight']['sha256'])
    launch = hashed_json(args.session_dir / 'launch.json', cap['launch']['sha256'])
    need(launch['started_utc'] == cap['launch']['started_utc'], 'date de lancement')
    need(pre['commit'] == cap['source_pin'], 'source du lancement')
    bundles = Path(pre['data_dir']).parent
    counts = {}
    for folder, expected in cap['manifests'].items():
        doc = hashed_json(bundles / folder / 'bundle_manifest.json', expected['sha256'])
        got = {c['name']: (c['distinct'] if c.get('bundled') == 'distinct' else c).get('count', c['count'])
               for c in doc['cases']}
        need(got == expected['cases'], 'effectifs declares')
        need(set(c['profile'] for c in doc['cases']) == {'u21'}, 'profil declare')
        counts.update(got)
    plans, stats = {}, {}
    for name, sha in cap['plan_sha256'].items():
        doc = hashed_json(args.plans_dir / name, sha)
        command = doc['commands'][0]
        argv = command['argv']
        opts = dict(zip(argv[2::2], argv[3::2]))
        cases = [tuple(case.split(':')) for case in opts['--cas'].split(',')]
        need(all(opts[k] == v for k, v in {'--fils': '48', '--budget-gio': '160',
                                         '--budget-appareil-gio': '88', '--empreinte-max-sites': '1600000'}.items()),
             'regime different')
        plans[name] = cases
        stats[name] = dict(scenes=len({c[0] for c in cases}), processes=len(cases),
                           passes=sum(int(c[3]) for c in cases),
                           after_first=sum(int(c[3]) - 1 for c in cases),
                           single_pass=sum(c[3] == '1' for c in cases),
                           digest_processes=sum(counts[c[0]] <= 1_600_000 for c in cases),
                           digest_passes=sum(int(c[3]) for c in cases if counts[c[0]] <= 1_600_000))
    before = set(plans['plan_b_l.json'])
    after = set(plans['plan_b_l1.json'] + plans['plan_b_l2.json'])
    need(after - before == {('ign_lyon_0842_6521', '5', 'appareil', '1')} and not before - after,
         'union des plans')
    b = pre['budget']
    need(b['worker_window_seconds'] == b['guest_seconds'] - b['upload_estimate_seconds'] -
         b['closing_reserve_seconds'] - 120, 'fenetre')
    need(b['command_timeouts_sum_seconds'] <= b['worker_window_seconds'] - b['build_and_setup_seconds'] and
         b['oversubscribed'] is False, 'admission temporelle')
    life = args.session_dir / 'host/lifecycle.txt'
    unchanged = life.exists() and hashlib.sha256(life.read_bytes()).hexdigest() == cap['lifecycle_observation']['sha256']
    need(stats['plan_b_l1.json']['digest_processes'] == 5 and stats['plan_b_l1.json']['digest_passes'] == 9 and
         stats['plan_b_l2.json']['digest_processes'] == 0, 'cohorte empreintes')
    print(json.dumps(dict(ok=True, sources=len(cap['source_sha256']), plans=stats,
                         launch_started_utc=launch['started_utc'], lifecycle_same_as_observation=unchanged,
                         original_l_no_vm_proved=False, any_time_admitted=False, remote_calls=False), sort_keys=True))


if __name__ == '__main__':
    main()
