#!/usr/bin/env python3
"""Voies de feuille de la sonde FULL sur un nuage moyen : CPU (16379), feuille source unique sur l'hote (32763), lot
de feuilles sur le Pool avec reservoir chaine (49147) et sans (180219 = 49147 + 131072, voie du 5 octobre) rendent le
meme dump et le meme registre du catalogue.

    python3 full_leaf_lanes.py MHGP11_FULL_BENCH BITS

Nuage uniforme de 3 000 sites (random.Random(20261004), cube de cote 2^16, PointId = rang), quatre fils : K = 5 avec
des feuilles de 16 (trois voies), puis K = 10 avec des feuilles de 24 (CPU et lot). Les feuilles se recouvrent : la
porte exige un lot plus nombreux que les sites (le regime qu'une garde fausse « feuilles <= sites » refusait le
4 octobre 2026), au moins une boule par voie de lot et aucune feuille du lot hors des voies 49147 ; a K = 10, des
feuilles debordent de leur case : avec le reservoir, elles continuent dans ses blocs (blocs pris > 0, aucune feuille
rejouee) ; sans lui, elles sont rejouees par la seconde passe (0 < rejouees < lot). Les trois chemins d'ecriture
(copie des cases, copie des chaines, rejeu) servent donc. Bibliotheque standard seule ; aucune assertion Python. Code 0 :
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
CONFIGS = (('5', '16', {'cpu': 16379, 'feuille_hote': 32763, 'lot_hote': 49147}),
           ('10', '24', {'cpu': 16379, 'lot_hote': 49147, 'lot_rejoue': 180219}))


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
                need(name.startswith('lot_') or seen[name]['batch']['jobs'] == 0, 'lot hors voie de lot')
            batch = seen['lot_hote']['batch']
            need(batch['jobs'] > SITES, 'lot de feuilles pas plus nombreux que les sites (%d)' % batch['jobs'])
            need(batch['records'] > 0 and batch['population'] > 0 and batch['unresolved'] <= batch['jobs'], 'lot vide')
            # Branche copiee observable (audit du 4 octobre, ee2b48b4c) : des feuilles emettrices tiennent dans leur case.
            need(batch['copied_jobs'] > 0, 'aucune case copiee')
            line.append('k%s boules%d lot%d non_resolues%d rejouees%d copiees%d' % (
                kmax, reference['balls'], batch['jobs'], batch['unresolved'], batch['fill_jobs'], batch['copied_jobs']))
            last = batch
        # A K = 10 : chaines du reservoir avec lui, seconde passe sans lui (memes dump et registre, controles plus haut).
        need(last['fill_jobs'] == 0 and last['spare_record_chunks'] > 0 and last['spare_population_chunks'] > 0,
             'reservoir non exerce (rejouees %d, blocs %d et %d)' % (last['fill_jobs'], last['spare_record_chunks'],
                                                                     last['spare_population_chunks']))
        replay = seen['lot_rejoue']['batch']
        need(0 < replay['fill_jobs'] < replay['jobs'] and replay['spare_record_chunks'] == 0,
             'seconde passe non exercee sans reservoir (%d)' % replay['fill_jobs'])
        line.append('rejouees_sans_reservoir%d' % replay['fill_jobs'])
        print('full_leaf_lanes_verdict conforme sites%d %s' % (SITES, ' '.join(line)))


if __name__ == '__main__':
    main()
