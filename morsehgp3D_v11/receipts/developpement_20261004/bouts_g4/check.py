#!/usr/bin/env python3
"""Lecteur du reçu bouts_g4 : empreintes, arrêt certifié, déballage, porte stricte, scènes, comptes publiés.

    python3 -B morsehgp3D_v11/receipts/developpement_20261004/bouts_g4/check.py

Code 0 si les empreintes concordent, si l'arrêt ciblé est certifié (TERMINATED relu), si l'archive des bouts a été
déballée sans écart, si la porte est conforme (fixtures et mutants), si les 360 scènes sont « ok », et si les comptes
recalculés ici, indépendamment de Zoltan/demos/tools/choisir_bouts.py, égalent ceux de Zoltan/demos/bouts_hgp/
evalues.json ; 1 sinon. Python 3.10 nu, aucun assert.
"""
import hashlib
import json
from pathlib import Path
import sys
import tarfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
ORDERS = ('2', '3', '5', '10')


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    problems = []
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        if not (HERE / name).is_file() or sha256(HERE / name) != digest:
            problems.append('empreinte : ' + name)
    session = HERE / 'sessions' / 'claudebouts1'
    receipt = json.loads((session / 'receipt.json').read_text())
    if receipt.get('targeted_shutdown_certified') is not True or \
            (receipt.get('observed_after') or {}).get('status') != 'TERMINATED':
        problems.append('arret cible non certifie')
    print('session', receipt.get('status'), receipt.get('evidence_grade'), (receipt.get('commit') or '')[:12],
          {c['name']: (c['status'], c['exit_code']) for c in receipt.get('commands', [])})
    bouts = {e['name']: e for e in json.loads((HERE / 'bouts_lot1.json').read_text())['bouts']}
    counts = {}
    scenes = 0
    with tarfile.open(session / 'results.tar.gz') as tar:
        for member in tar.getmembers():
            if not member.isfile():
                continue
            data = tar.extractfile(member).read()
            if member.name.endswith('000_unpack/stdout'):
                text = data.decode()
                print(' ', text.strip())
                if 'ecarts 0' not in text:
                    problems.append('deballage avec ecarts')
            elif member.name.endswith('/gate.json'):
                gate = json.loads(data)
                mutants = gate.get('mutants', {})
                print('  porte', gate['verdict'], 'nuages', gate['clouds'], 'fixtures',
                      sum(f['ok'] for f in gate['fixtures']), '/', len(gate['fixtures']),
                      'mutants', sum(m['killed'] for m in mutants.values()), '/', len(mutants))
                if gate['verdict'] != 'conforme' or gate['disagreements'] or \
                        not all(m['killed'] for m in mutants.values()) or len(mutants) < 4:
                    problems.append('porte non conforme')
            elif '/files/lidar/' in member.name and member.name.endswith('.json'):
                result = json.loads(data)
                scenes += 1
                if result.get('status') != 'ok':
                    problems.append('scene en echec : ' + str(result.get('name')))
                    continue
                kind = bouts[result['name']]['kind']
                c = counts.setdefault(kind, dict(bouts=0, hdbscan_echoue=0, gagnes=0, gagnes_tous_ordres=0, perdus=0))
                c['bouts'] += 1
                fails = {k: min(result['orders'][k]['hdbscan']['best']) <= 0.5 for k in ORDERS}
                hgp_ok = {k: min(result['orders'][k]['margin_r']['best']) > 0.5 for k in ORDERS}
                wins = [k for k in ORDERS if fails[k] and hgp_ok[k]]
                c['hdbscan_echoue'] += any(fails.values())
                c['gagnes'] += bool(wins)
                c['gagnes_tous_ordres'] += bool(wins) and all(fails.values())
                c['perdus'] += any(not hgp_ok[k] and not fails[k] for k in ORDERS)
    print('  scenes', scenes, 'comptes', json.dumps(counts, sort_keys=True))
    if scenes != len(bouts):
        problems.append('scenes %d pour %d bouts' % (scenes, len(bouts)))
    published = json.loads((ROOT / 'Zoltan' / 'demos' / 'bouts_hgp' / 'evalues.json').read_text())['comptes']
    if published != counts:
        problems.append('comptes differents de Zoltan/demos/bouts_hgp/evalues.json')
    print('bouts_g4_verdict', 'conforme' if not problems else 'refus', problems)
    return 0 if not problems else 1


if __name__ == '__main__':
    sys.exit(main())
