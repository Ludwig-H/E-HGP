#!/usr/bin/env python3
"""Read only the closed local metadata; no controller, probe or remote action."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


def need(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--session', required=True, type=Path)
    ap.add_argument('--repo', required=True, type=Path)
    args = ap.parse_args()
    root = args.session
    cap = json.loads(Path(__file__).with_name('capture.json').read_text())
    pinned = {}
    for name, pin in cap['local_files'].items():
        raw = (root / name).read_bytes()
        need(len(raw) == pin['bytes'] and hashlib.sha256(raw).hexdigest() == pin['sha256'],
             'primary changed: ' + name)
        pinned[name] = raw
    rec = json.loads(pinned['receipt.json'])
    need(rec['commit'] == cap['source_git'], 'source')
    need(rec['package_sha256'] == cap['package_sha256'] and
         rec['plan_sha256'] == cap['plan_sha256'], 'package/plan')
    for key, expected in cap['closure'].items():
        need(type(rec[key]) is type(expected) and rec[key] == expected, 'closure: ' + key)
    need(rec['errors'] == [cap['public_error']] and cap['errors_count'] == 1, 'retrieval error')
    need(re.fullmatch(r'rapatriement : Refusal: espace local insuffisant pour \d+ octets : \d+ libres',
                      cap['public_error']) is not None, 'error scope')
    need(int(pinned['DONE']) == cap['done'] == 3, 'DONE')
    for key, expected in cap['stop_observations'].items():
        need({k: rec[key][k] for k in expected} == expected, 'stop observations')
    need(rec['observed_before_stop']['status'] == 'RUNNING' and
         rec['observed_after']['status'] == 'TERMINATED', 'stop transition')
    announced = re.findall(r'(?<![a-f0-9])[a-f0-9]{64}(?![a-f0-9])',
                           pinned['host/logs/021_results_hash.stdout'].decode())
    need(announced == [cap['reported_remote_archive_sha256']] * 2, 'announced hash')
    need(rec['results_bytes'] == cap['reported_remote_archive_bytes'] == 575450, 'announced size')
    plan = json.loads(pinned['package/plan.json'])
    need([{'name': c['name'], 'timeout_seconds': c['timeout_seconds']} for c in plan['commands']]
         == cap['declared_commands'], 'declared commands')
    need(plan['build_timeout_seconds'] == cap['build_timeout_seconds'], 'build limit')
    launch = json.loads(pinned['launch.json'])
    need(launch['started_utc'] == cap['launch_started_utc'], 'launch')
    preflight = json.loads(pinned['preflight.json'])
    need(preflight['commit'] == cap['source_git'], 'preflight source')
    source = cap['retrieval_guard_source']
    raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', source['git'] + ':' + source['path']])
    need(len(raw) == source['bytes'] and hashlib.sha256(raw).hexdigest() == source['sha256'], 'guard source pin')
    need(b'LOCAL_FREE_MARGIN = 2 ** 30' in raw and b'free >= 2 * size + LOCAL_FREE_MARGIN' in raw,
         'retrieval space rule')
    need(hashlib.sha256(pinned['package/plan.json']).hexdigest() == cap['plan_sha256'], 'plan hash')
    for name, raw in pinned.items():
        need((root / name).read_bytes() == raw, 'primary changed during reading')
    print('A6 attempt verified: worker1/DONE3, local retrieval refusal, certified stop; '
          'no returned archive or timing admitted by this receipt')


if __name__ == '__main__':
    main()
