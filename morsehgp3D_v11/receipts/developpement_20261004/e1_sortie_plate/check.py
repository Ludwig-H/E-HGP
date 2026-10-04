#!/usr/bin/env python3
"""Lecteur du recu E1 (sortie plate) : relit les sessions G4 et recalcule les verdicts et le choix du dev.

    python3 morsehgp3D_v11/receipts/developpement_20261004/e1_sortie_plate/check.py

Pour chaque session : statut, arret cible certifie (TERMINATED), empreinte de results.tar.gz quand l'archive est
versionnee ; portes points et sortie plate conformes ; scenes du dev toutes 'ok'. Puis, sur le dev (8 000 et, si
present, 16 000 points), recalcule par bench/points_flat_summary.py la moyenne equiponderee des cellules du mIoU
un-a-un de chaque regle de la tour et verifie la regle retenue. Lit les archives en memoire (rien n'est ecrit).
Code 0 si tout concorde, 1 sinon. Aucune assertion : tient sous python3 -O.
"""
import hashlib
import io
import json
import os
import sys
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', '..', 'bench'))
import points_flat_summary as S  # noqa: E402

EXPECTED_CHOICE = 'eom2'


def archive_members(path, prefix):
    out = {}
    with tarfile.open(path) as tar:
        for member in tar.getmembers():
            if member.isfile() and prefix in member.name:
                out[member.name] = json.loads(tar.extractfile(member).read())
    return out


def main():
    problems = []
    sessions = os.path.join(HERE, 'sessions')
    dev = []
    for name in sorted(os.listdir(sessions)):
        folder = os.path.join(sessions, name)
        receipt = json.load(open(os.path.join(folder, 'receipt.json')))
        if receipt.get('status') != 'completed' or not receipt.get('targeted_shutdown_certified') or \
                receipt.get('observed_after', {}).get('status') != 'TERMINATED':
            problems.append('%s : session non close ou arret non certifie' % name)
        if any(c.get('status') != 'ok' for c in receipt.get('commands', [])):
            problems.append('%s : commande en echec' % name)
        archive = os.path.join(folder, 'results.tar.gz')
        if not os.path.isfile(archive):
            print('%-12s statut %s, TERMINATED, archive non versionnee (empreinte dans receipt.json)' % (
                name, receipt.get('status')))
            continue
        digest = hashlib.sha256(open(archive, 'rb').read()).hexdigest()
        recorded = json.dumps(receipt)
        if digest not in recorded:
            problems.append('%s : empreinte de results.tar.gz absente du recu' % name)
        gates = archive_members(archive, 'gate.json')
        verdicts = {key.split('/')[-1]: value.get('verdict') for key, value in gates.items()}
        if any(v != 'conforme' for v in verdicts.values()) or len(verdicts) < 2:
            problems.append('%s : portes %s' % (name, verdicts))
        runs = [v for k, v in archive_members(archive, '/files/dev').items() if k.endswith('.json')]
        bad = [r.get('name') for r in runs if r.get('status') != 'ok']
        if bad:
            problems.append('%s : scenes en echec %s' % (name, bad[:3]))
        dev.extend(r for r in runs if r.get('status') == 'ok')
        print('%-12s statut %s, TERMINATED, portes %s, scenes dev %d' % (name, receipt.get('status'), verdicts, len(runs)))
    if dev:
        table = {}
        for rule in ('eom1', 'eom2', 'eom3', 'leaf'):
            cells = {}
            for r in dev:
                for k in (2, 3, 5, 10):
                    o = r['orders'].get(str(k))
                    if o is None:
                        continue
                    v = S.line_value(o, 'T_' + rule, S.mcs_keys_of(o), 'miou_h')
                    cells.setdefault(S.cell_of(r), []).append(v)
            table[rule] = sum(sum(v) / len(v) for v in cells.values()) / len(cells)
        best = max(table, key=table.get)
        print('dev : %d scenes ; mIoU_h moyen par regle %s ; meilleure %s' % (
            len(dev), {k: round(v, 4) for k, v in table.items()}, best))
        if best != EXPECTED_CHOICE:
            problems.append('regle du dev %s au lieu de %s' % (best, EXPECTED_CHOICE))
    print('verdict', 'conforme' if not problems else 'ecarts : ' + ' ; '.join(problems))
    return 0 if not problems else 1


if __name__ == '__main__':
    raise SystemExit(main())
