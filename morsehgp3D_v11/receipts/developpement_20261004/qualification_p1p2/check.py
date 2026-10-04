#!/usr/bin/env python3
"""Lecteur du recu de qualification G4 de P1/P2 (4 octobre 2026) : relit les deux sessions de la matrice.

    python3 morsehgp3D_v11/receipts/developpement_20261004/qualification_p1p2/check.py

claudequal1 (d597ed9ba) : echec attendu et explique, seule la porte de largeur (numpy absent du Python nu de la VM)
et le temoin u24 des mutants de la tour ; VM TERMINATED. claudequal2 (eb036dbe2) : empreintes du plan et de
results.tar.gz egales au recu, VM TERMINATED, matrice conforme et complete, chaque configuration demandee 'ok' sauf
clang absent de l'hote, aucun test en echec, planchers de mutants atteints par module, les deux mutants nouveaux
tues. Lit l'archive en memoire. Code 0 si tout concorde, 1 sinon. Aucune assertion : tient sous python3 -O.
"""
import hashlib
import json
import os
import re
import sys
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__))
NEW_MUTANTS = ('pipeline_abandon_apres_reveil', 'export_points_trois_mots')
MATRIX = 'results/cmd/000_matrice/files/matrix/'


def digest(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


def closed(receipt):
    return receipt.get('targeted_shutdown_certified') is True and \
        receipt.get('observed_after', {}).get('status') == 'TERMINATED'


def main():
    problems = []
    first = json.load(open(os.path.join(HERE, 'sessions', 'claudequal1', 'receipt.json')))
    if not closed(first) or first.get('status') != 'failed_remote' or not first.get('commit', '').startswith('d597ed9ba'):
        problems.append('claudequal1 : attendu failed_remote, d597ed9ba, VM TERMINATED')
    folder = os.path.join(HERE, 'sessions', 'claudequal2')
    receipt = json.load(open(os.path.join(folder, 'receipt.json')))
    if not closed(receipt) or receipt.get('status') != 'completed' or not receipt.get('commit', '').startswith('eb036dbe2'):
        problems.append('claudequal2 : attendu completed, eb036dbe2, VM TERMINATED')
    if digest(os.path.join(folder, 'plan.json')) != receipt.get('plan_sha256'):
        problems.append('claudequal2 : plan different du recu')
    archive = os.path.join(folder, 'results.tar.gz')
    if digest(archive) != receipt.get('results_sha256'):
        problems.append('claudequal2 : results.tar.gz different du recu')
    killed, floors, rows = set(), {}, {}
    with tarfile.open(archive) as tar:
        names = {m.name: m for m in tar.getmembers() if m.isfile()}
        summary = json.loads(tar.extractfile(names[MATRIX + 'summary.json']).read())
        for config in summary.get('requested', []):
            path = MATRIX + config + '/result.json'
            if path not in names:
                rows[config] = None
                continue
            result = json.loads(tar.extractfile(names[path]).read())
            rows[config] = (result.get('status'), result.get('tests', {}))
        for name, member in names.items():
            if name.startswith(MATRIX + 'mutants/') and name.endswith('.log'):
                text = tar.extractfile(member).read().decode('utf-8', 'replace')
                killed.update(m for m in NEW_MUTANTS if re.search(r'\b%s\s+TUE\b' % m, text))
                for module, count, kills, floor in re.findall(
                        r'mutants_ok module=(\w+) mutants=(\d+) tues=(\d+) .*? plancher=(\d+)', text):
                    floors[module] = (int(count), int(kills), int(floor))
    if summary.get('conforming') is not True or summary.get('complete') is not True:
        problems.append('matrice non conforme ou incomplete')
    for config, status in summary.get('statuses', {}).items():
        if status != 'ok' and not (config == 'clang_release' and status == 'absent'):
            problems.append('configuration %s : %s' % (config, status))
    for config, row in sorted(rows.items()):
        if row is None:
            print('%-15s absent de l hote' % config)
            continue
        status, tests = row
        print('%-15s %-6s %4s/%-4s en echec %s' % (config, status, tests.get('passed'), tests.get('selected'),
                                                tests.get('failed')))
        if config == 'clang_release' and status == 'absent':
            continue  # clang++ absent de l'hote G4 (summary.host) : configuration non jouee, pas un echec
        if status != 'ok' or tests.get('failed') != 0 or tests.get('passed') != tests.get('selected'):
            problems.append('configuration %s : %s' % (config, row))
    for module, (count, kills, floor) in sorted(floors.items()):
        print('mutants %-10s %3d/%-3d plancher %d' % (module, kills, count, floor))
        if kills != count or kills < floor:
            problems.append('mutants %s : %d/%d plancher %d' % (module, kills, count, floor))
    if set(NEW_MUTANTS) - killed:
        problems.append('mutants nouveaux non tues : %s' % sorted(set(NEW_MUTANTS) - killed))
    if len(floors) < 7:
        problems.append('modules de mutants : %d' % len(floors))
    for problem in problems:
        print('ecart : ' + problem)
    print('qualification_p1p2_verdict %s' % ('conforme' if not problems else 'ecart'))
    return 0 if not problems else 1


if __name__ == '__main__':
    sys.exit(main())
