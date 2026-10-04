#!/usr/bin/env python3
"""Lecteur du reçu bouts_g4 : empreintes, arrêts certifiés, déballage, porte stricte, scènes, catégories publiées.

    python3 -B morsehgp3D_v11/receipts/developpement_20261004/bouts_g4/check.py

Code 0 si les empreintes concordent ; si, pour les deux sessions, l'arrêt ciblé est certifié (TERMINATED relu),
l'archive a été déballée sans écart, la porte est conforme (fixtures et mutants) et toutes les scènes sont « ok » ; si
les catégories des 360 bouts, recalculées ici depuis la session claudebouts1 indépendamment de
Zoltan/demos/tools/choisir_bouts.py, égalent celles de Zoltan/demos/bouts_evalues.json ; si la session claudebouts2
publie le meilleur bloc de chaque objet (--members-all) pour les 31 bouts et les 5 démos du lot 2 ; 1 sinon.
Python 3.10 nu, aucun assert.
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


def outcome(hdb, hgp):
    hf, gs = min(hdb) <= 0.5, min(hgp) > 0.5
    return {(True, True): 'win', (False, False): 'loss', (True, False): 'both_fail', (False, True): 'both_ok'}[(hf, gs)]


def category(result):
    outs = [outcome(result['orders'][k]['hdbscan']['best'], result['orders'][k]['margin_r']['best']) for k in ORDERS]
    for name, cat in (('win', 'hgp_reussit_hdbscan_echoue'), ('loss', 'hgp_echoue_hdbscan_reussit'),
                      ('both_fail', 'hgp_echoue_hdbscan_echoue')):
        if name in outs:
            return cat
    return 'hgp_reussit_hdbscan_reussit'


def session_check(name, problems):
    session = HERE / 'sessions' / name
    receipt = json.loads((session / 'receipt.json').read_text())
    if receipt.get('targeted_shutdown_certified') is not True or \
            (receipt.get('observed_after') or {}).get('status') != 'TERMINATED':
        problems.append('arret cible non certifie : ' + name)
    print('session', name, receipt.get('status'), receipt.get('evidence_grade'), (receipt.get('commit') or '')[:12],
          {c['name']: (c['status'], c['exit_code']) for c in receipt.get('commands', [])})
    results = {}
    with tarfile.open(session / 'results.tar.gz') as tar:
        for member in tar.getmembers():
            if not member.isfile():
                continue
            data = tar.extractfile(member).read()
            if member.name.endswith('000_unpack/stdout'):
                text = data.decode()
                print(' ', text.strip())
                if 'ecarts 0' not in text:
                    problems.append('deballage avec ecarts : ' + name)
            elif member.name.endswith('/gate.json'):
                gate = json.loads(data)
                mutants = gate.get('mutants', {})
                print('  porte', gate['verdict'], 'nuages', gate['clouds'], 'fixtures',
                      sum(f['ok'] for f in gate['fixtures']), '/', len(gate['fixtures']),
                      'mutants', sum(m['killed'] for m in mutants.values()), '/', len(mutants))
                if gate['verdict'] != 'conforme' or gate['disagreements'] or \
                        not all(m['killed'] for m in mutants.values()) or len(mutants) < 4:
                    problems.append('porte non conforme : ' + name)
            elif '/files/lidar/' in member.name and member.name.endswith('.json'):
                result = json.loads(data)
                if result.get('status') != 'ok':
                    problems.append('scene en echec : %s/%s' % (name, result.get('name')))
                    continue
                results[result['name']] = result
    return results


def main():
    problems = []
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        if not (HERE / name).is_file() or sha256(HERE / name) != digest:
            problems.append('empreinte : ' + name)
    bouts = {e['name']: e for e in json.loads((HERE / 'bouts_lot1.json').read_text())['bouts']}
    first = session_check('claudebouts1', problems)
    counts = {}
    for name, result in first.items():
        cat, fam = category(result), bouts[name]['kind']
        counts.setdefault(cat, {})[fam] = counts.setdefault(cat, {}).get(fam, 0) + 1
    print('  scenes', len(first), 'categories', json.dumps(counts, sort_keys=True, ensure_ascii=False))
    if len(first) != len(bouts):
        problems.append('scenes %d pour %d bouts' % (len(first), len(bouts)))
    published = json.loads((ROOT / 'Zoltan' / 'demos' / 'bouts_evalues.json').read_text())
    if {c: v for c, v in published['comptes'].items() if v} != counts:
        problems.append('categories differentes de Zoltan/demos/bouts_evalues.json')
    second = session_check('claudebouts2', problems)
    lot2 = json.loads((HERE / 'selection_lot2.json').read_text())
    wanted = [e['name'] for e in lot2] + ['zoltan_01_velos_en_rang', 'zoltan_02_velos_contre_facade',
                                          'zoltan_03_pieton_contre_facade', 'zoltan_04_velos_en_rang_avec_sol',
                                          'zoltan_05_temoin_voitures_en_file']
    missing = [n for n in wanted if n not in second or any('members' not in second[n]['orders'][k] for k in ORDERS)]
    print('  lot 2 : %d scenes attendues, %d sans blocs publies' % (len(wanted), len(missing)))
    if missing:
        problems.append('blocs non publies : ' + ', '.join(missing[:5]))
    print('bouts_g4_verdict', 'conforme' if not problems else 'refus', problems)
    return 0 if not problems else 1


if __name__ == '__main__':
    sys.exit(main())
