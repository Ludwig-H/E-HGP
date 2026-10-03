#!/usr/bin/env python3
"""Audit borné : hashes, métriques conservées et modèles Python ; aucun natif/cloud."""
from pathlib import Path
import hashlib, json, statistics, subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PROOF_SHA = "43e4f97fb4182d8aa0e41291e7f053a304a092c2bd6691cea62c0fc519d149b6"

def need(ok, message):
    if not ok:
        raise ValueError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    raw = (HERE / 'proof.json').read_bytes()
    need(sha(raw) == PROOF_SHA, 'proof pin')
    p = json.loads(raw)
    for path, digest in p['product_sources'].items():
        blob = subprocess.check_output(['git', '-C', str(ROOT), 'show', p['source'] + ':' + path])
        need(sha(blob) == digest, 'Git source ' + path)
    for path, digest in p['artifacts'].items():
        need(sha((HERE / path).read_bytes()) == digest, 'artifact ' + path)
    for path, digest in p['existing_receipt_pins'].items():
        need(sha((ROOT / path).read_bytes()) == digest, 'existing receipt ' + path)
    first = HERE.parent / 'ecart_v10_v11'
    second = HERE.parent / 'ecart_v10_v11_tranche2'
    for folder in (first, second):
        for flags in ([], ['-O']):
            result = subprocess.run(['python3', '-B', *flags, str(folder/'check.py')],
                                    capture_output=True, text=True, check=True)
            need('recu_ok' in result.stdout, 'reader receipt')
    one = json.loads((first/'records.json').read_text())
    two = json.loads((second/'records.json').read_text())
    need(len(one) == 35 and len(two) == 13, '48 captured runs')
    derived = json.loads((HERE/'performance.json').read_text())
    for row in derived['observed_table']:
        selected = [r for r in one if r['case'] == row['case'] and r['workers'] == 4]
        for build, mode, label in [('base', 2047, 'base2047'), ('new', 2047, 'new2047'), ('new', 16379, 'new16379')]:
            group = [r for r in selected if r['build'] == build and r['mode'] == mode]
            need(len(group) == 3, 'three local repeats')
            need(statistics.median(r['wall_ms'] for r in group) == row[label]['wall_ms'], 'median ' + label)
        group = [r for r in two if r['case'] == row['case'] and r['workers'] == 4]
        need(len(group) == 3, 'three tranche2 repeats')
        need(statistics.median(r['wall_ms'] for r in group) == row['tranche2_16379']['wall_ms'], 'tranche2 median')
    for name, expected in p['models'].items():
        for flags in ([], ['-O']):
            result = subprocess.run(['python3', '-B', *flags, str(HERE/name)], capture_output=True, text=True, check=True)
            need(json.loads(result.stdout) == expected, 'model ' + name)
    print(json.dumps({'status':'ok','source':p['source'],'product_sources':len(p['product_sources']),
                      'changed_product_paths':len(p['changed_product_paths']),'captured_runs':48,
                      'models':len(p['models']),'native_executions':0,'cloud_commands':0}, sort_keys=True))

if __name__ == '__main__':
    main()
