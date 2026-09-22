#!/usr/bin/env python3
"""Lecteur du recu de la premiere tour v9 (valide sous python3 -O, sans assert).

Relit raw/*.json (schema mhgp9_tower_probe_v1) et raw/*.time (GNU time -v),
refuse toute ligne incomplete ou incoherente, puis ecrit SUMMARY.json. Codes :
0 conforme, 1 incoherence, 2 usage.
"""
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
RAW = HERE / 'raw'
LINES = [(s, k) for k in (5, 10) for s in ('00', '01', '02')]
SITES = {'00': 39885, '01': 35551, '02': 45845}


class Refusal(Exception):
    pass


def need(ok, why):
    if not ok:
        raise Refusal(why)


def gnu_time(path):
    text = path.read_text()
    out = {}
    for key, pattern in (('user_s', r'User time \(seconds\): ([0-9.]+)'),
                         ('sys_s', r'System time \(seconds\): ([0-9.]+)'),
                         ('max_rss_kb', r'Maximum resident set size \(kbytes\): (\d+)'),
                         ('rc', r'^rc=(\d+)$')):
        found = re.findall(pattern, text, re.M)
        need(len(found) == 1, f'{path.name}: {key} absent')
        out[key] = float(found[0]) if key.endswith('_s') else int(found[0])
    wall = re.findall(r'Elapsed \(wall clock\) time \(h:mm:ss or m:ss\): (\S+)', text)
    need(len(wall) == 1, f'{path.name}: wall absent')
    parts = [float(p) for p in wall[0].split(':')]
    seconds = 0.0
    for p in parts:
        seconds = seconds * 60 + p
    out['wall_s'] = seconds
    return out


def main(argv):
    if len(argv) != 1:
        print('usage: summarize.py', file=sys.stderr)
        return 2
    try:
        context = dict(line.split('=', 1) for line in (RAW / 'context.txt').read_text().splitlines() if '=' in line)
        need(re.fullmatch('[0-9a-f]{40}', context.get('commit', '')), 'context commit')
        rows = []
        digests = {}
        for scene, k in LINES:
            tag = f's{scene}_k{k}_w8'
            probe = json.loads((RAW / f'{tag}.json').read_text())
            timing = gnu_time(RAW / f'{tag}.time')
            need(probe.get('schema') == 'mhgp9_tower_probe_v1', f'{tag}: schema')
            need(probe['status'] == 'complete_relative' and timing['rc'] == 0, f'{tag}: status')
            need(probe['input']['sites'] == SITES[scene], f'{tag}: sites')
            need(probe['options']['K'] == k and probe['options']['workers'] == 8, f'{tag}: options')
            need(len(probe['orders']) == k and [o['K'] for o in probe['orders']] == list(range(1, k + 1)), f'{tag}: orders')
            cat = probe['catalogue']
            need(cat['balls'] == cat['unique_keys'] == sum(cat['by_qmin']), f'{tag}: catalogue identity')
            need(cat['shell_over_12'] == 0, f'{tag}: shell cap')
            need(probe['orders'][0]['births'] == SITES[scene], f'{tag}: K1 births')
            t = probe['times_ms']
            rows.append(dict(scene=scene, K=k, sites=SITES[scene], wall_s=timing['wall_s'],
                             cpu_s=timing['user_s'] + timing['sys_s'], max_rss_kb=timing['max_rss_kb'],
                             chain_ms=t['chain_total'], q2_ms=t['q2'], q34_ms=t['q34'], census_ms=t['census'],
                             tower_ms=t['tower'], balls=cat['balls'], max_shell=cat['max_shell'],
                             extra_shell_balls=cat['extra_shell_balls'],
                             nodes=sum(o['nodes'] for o in probe['orders']), digest=probe['tower_digest'],
                             load_before=(RAW / f'{tag}.load_before').read_text().strip(),
                             load_after=(RAW / f'{tag}.load_after').read_text().strip()))
            digests[tag] = probe['tower_digest']
        summary = dict(schema='mhgp9_first_tower_summary_v1', status='passed', public_status='not_claimed',
                       commit=context['commit'], probe_sha256=context.get('probe_sha256'), host_cpu=context.get('host_cpu'),
                       nproc=context.get('nproc'), inputs={k: v for k, v in context.items() if k.startswith('input_')},
                       lines=rows)
        (HERE / 'SUMMARY.json').write_text(json.dumps(summary, indent=1, sort_keys=True) + '\n')
        print(json.dumps(dict(status='passed', lines=len(rows)), sort_keys=True))
        return 0
    except (Refusal, OSError, ValueError, KeyError) as error:
        print(json.dumps(dict(status='refused', reason=str(error)), sort_keys=True))
        return 1


if __name__ == '__main__':
    raise SystemExit(main(sys.argv))
