#!/usr/bin/env python3
"""Voies de feuille de la sonde FULL sur un nuage moyen : CPU (16379), feuille source unique sur l'hote (32763), lot
de feuilles sur le Pool (49147), et leurs variantes a feuille cooperative (163835, 180219) rendent le meme dump et le
meme registre du catalogue.

    python3 full_leaf_lanes.py MHGP11_FULL_BENCH BITS

Nuage uniforme de 3 000 sites (random.Random(20261004), cube de cote 2^16, PointId = rang), quatre fils : K = 5 avec
des feuilles de 16 (trois voies), puis K = 10 avec des feuilles de 24 (CPU et lot). Les feuilles se recouvrent : la
porte exige un lot plus nombreux que les sites (le regime qu'une garde fausse « feuilles <= sites » refusait le
4 octobre 2026), au moins une boule par voie de lot et aucune feuille du lot hors des voies 49147 ; a K = 10, des
feuilles debordent de leur case, si bien que les deux chemins d'ecriture du lot (copie des cases du comptage,
seconde passe des feuilles qui debordent) servent. Bibliotheque standard seule ; aucune assertion Python. Code 0 :
conforme.
"""
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

SITES = 3000
# feuille_coop (163835 = 32763 + 131072) et lot_coop (180219 = 49147 + 131072) : feuille cooperative emulee sur
# l'hote (paires de la profondeur 1 comptees dans l'ordre inverse, puis emises dans l'ordre du parcours).
CONFIGS = (('5', '16', {'cpu': 16379, 'feuille_hote': 32763, 'lot_hote': 49147, 'feuille_coop': 163835,
                        'lot_coop': 180219}),
           ('10', '24', {'cpu': 16379, 'lot_hote': 49147, 'lot_coop': 180219}))


def need(value, reason):
    if not value:
        print('full_leaf_lanes_verdict refus : ' + reason)
        sys.exit(1)


def phases(text):
    out = {}
    for line in text.splitlines():
        try:
            event = json.loads(line)
        except ValueError:
            continue
        if isinstance(event, dict) and 'phase' in event:
            out[event['phase']] = event
    return out


def main():
    executable, bits = Path(sys.argv[1]), int(sys.argv[2])
    need(bits in (18, 21, 24) and executable.is_file(), 'arguments')
    rng = random.Random(20261004)
    points = sorted({(rng.getrandbits(16), rng.getrandbits(16), rng.getrandbits(16)) for _ in range(SITES)})
    need(len(points) == SITES, 'sites distincts')
    with tempfile.TemporaryDirectory(prefix='mhgp11-leaf-lanes-') as temporary:
        root = Path(temporary)
        xyz, ids, dump = root / 'xyz', root / 'ids', root / 'dump'
        xyz.write_bytes(b''.join(struct.pack('<III', *p) for p in points))
        ids.write_bytes(b''.join(struct.pack('<I', i) for i in range(SITES)))
        line = []
        for kmax, leaf, modes in CONFIGS:
            seen = {}
            for name, mode in modes.items():
                dump.unlink(missing_ok=True)
                argv = [str(executable), str(xyz), str(ids), str(dump), kmax, leaf, '256', '0', str(2**32 - 1),
                        str(1 << 32), '4', str(mode)]
                run = subprocess.run(argv, stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=300)
                got = phases(run.stdout)
                need(run.returncode == 0 and got.get('exit', {}).get('status') == 'ok', name + ' K' + kmax +
                     ' : sortie ' + str(run.returncode) + ' ' + json.dumps(got.get('exit')))
                need(dump.is_file(), name + ' : dump absent')
                domain = got['domain']
                seen[name] = dict(digest=hashlib.sha256(dump.read_bytes()).hexdigest(), work=domain['catalogue_work'],
                                  batch=domain['leaf_batch'], balls=domain['catalogue_balls'])
            reference = seen['cpu']
            for name in modes:
                need(seen[name]['digest'] == reference['digest'], name + ' K' + kmax + ' : dump different du CPU')
                need(seen[name]['work'] == reference['work'], name + ' K' + kmax + ' : registre different')
                need(name in ('lot_hote', 'lot_coop') or seen[name]['batch']['jobs'] == 0, 'lot hors voie de lot')
            batch = seen['lot_hote']['batch']
            coop = seen['lot_coop']['batch']
            need(all(coop[key] == batch[key] for key in ('jobs', 'records', 'population', 'unresolved')),
                 'lot cooperatif different du lot sequentiel')
            need(batch['jobs'] > SITES, 'lot de feuilles pas plus nombreux que les sites (%d)' % batch['jobs'])
            need(batch['records'] > 0 and batch['population'] > 0 and batch['unresolved'] <= batch['jobs'], 'lot vide')
            # Branche copiee observable (audit du 4 octobre, ee2b48b4c) : des feuilles emettrices tiennent dans leur case.
            need(batch['copied_jobs'] > 0, 'aucune case copiee')
            line.append('k%s boules%d lot%d non_resolues%d rejouees%d copiees%d' % (
                kmax, reference['balls'], batch['jobs'], batch['unresolved'], batch['fill_jobs'], batch['copied_jobs']))
            last = batch
        # Les deux chemins d'ecriture servent a K = 10 : copie des cases et seconde passe des feuilles qui debordent.
        need(0 < last['fill_jobs'] < last['jobs'], 'chemins d ecriture non exerces (%d)' % last['fill_jobs'])
        print('full_leaf_lanes_verdict conforme sites%d %s' % (SITES, ' '.join(line)))


if __name__ == '__main__':
    main()
