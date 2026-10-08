#!/usr/bin/env python3
"""Temoin WIT-FORME-NIVEAU de bout en bout par l'export reel (contrat de la tour, paragraphe 1, corrige le 7 octobre
2026, 58d384721 ; fixture independante reference/test_witness_forme.py, e87896702).

Cinq sites (0,100,100), (10,100,100), (35,10,3), (27,14,3), (27,6,3), K = 2 : le niveau 25 est porte par la paire des
deux premiers (forme non reduite q2 100/4) et par le triangle aigu des trois autres (forme q3 409600/16384). Le
vidage FUL1 ecrit chaque niveau dans la forme de la PREMIERE boule de son rang selon l'ordre publie du catalogue qu'on
lui donne. Ordre de la v11 (S* par rangs de Morton) : le triangle ouvre le rang ; ordre de T1 (S* par positions) : la
paire. L'outil mhgp12_tower_forest_oracle exporte deux fois le meme registre, la table des niveaux suivant chacun des deux
ordres (memes boules, rangs et cibles, indices des deux boules echanges) ; attendus graves : octets differents, formes
409600/16384 et 100/4 au noeud de fusion de niveau 25 de l'ordre un, empreinte semantique identique (lecteur strict
full_reader.py), egale a bead653d..., et, au profil 21, les deux empreintes des octets.

    python3 forme_niveau.py <mhgp12_tower_forest_oracle> <bits>

Codes : 0 conforme ; 1 ecart ; 2 usage ; 3 refus de l'outil. Python 3.10 nu, aucun assert.
"""
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
sys.path.insert(0, HERE)
import full_reader  # noqa: E402
import oracle_tour  # noqa: E402

OK, DISAGREEMENT, USAGE, REFUSED = 0, 1, 2, 3
POINTS = [(0, 100, 100), (10, 100, 100), (35, 10, 3), (27, 14, 3), (27, 6, 3)]
K = 2
SEMANTIC = 'bead653db7ce5def183ca9c0694c111e8599bef699f33f8bbfa49c2bfe3442c0'
RAW_21 = {'morton': 'a7ab024fd140ee1f98e96eade139225d8a57b112873cd3edd1f26fe749ec5aa4',
          'positions': '11075c81623e7baa8a6f2889dfbe02edba7abe34fd909f7746dbe4a10ae65b8a'}
FORMS = {'morton': (409600, 16384), 'positions': (100, 4)}


def swapped(text):
    """Le meme cas, les deux boules de niveau 25 echangees (lignes b et indices dans les lignes n et c)."""
    lines = text.splitlines()
    b_lines = [i for i, line in enumerate(lines) if line.startswith('b ')]
    by_rank = {}
    for i in b_lines:
        by_rank.setdefault(lines[i].split()[1], []).append(i)
    pair = [v for v in by_rank.values() if len(v) == 2 and len(lines[v[0]].split()) != len(lines[v[1]].split())]
    if len(pair) != 1:
        return None
    first, second = pair[0]
    swap = {first - b_lines[0]: second - b_lines[0], second - b_lines[0]: first - b_lines[0]}
    out, order = [], 0
    for line in lines:
        words = line.split()
        if words[0] == 'ordre':
            order = int(words[1])
        if (words[0] == 'n' and order >= 2) or words[0] == 'c':
            words[1] = str(swap.get(int(words[1]), int(words[1])))
        out.append(' '.join(words))
    out[first], out[second] = lines[second], lines[first]
    return '\n'.join(out) + '\n'


def level_form_of_first_merge(path):
    """Numerateur et denominateur NON reduits ecrits pour la premiere fusion de niveau 25 de l'ordre un."""
    with open(path, 'rb') as handle:
        data = handle.read()
    words = [struct.unpack_from('<Q', data, at)[0] for at in range(10, len(data), 8)]
    at = 4 + 5 * len(POINTS)  # profil, K, sites, poids ; puis x, y, z, w, PointId par site
    births, count = words[at + 1], words[at + 2]
    at += 5
    for node in range(count):
        at += 3
        forms = []
        for _ in range(2):
            limbs = words[at + 1]
            forms.append(sum(words[at + 2 + i] << (64 * i) for i in range(limbs)) * (-1 if words[at] else 1))
            at += 2 + limbs
        if node >= births and forms[0] * 1 == 25 * forms[1]:
            return tuple(forms)
        if node < births:
            for _ in range(4):
                at += 2 + words[at + 1]
    return None


def main(argv):
    if len(argv) != 3 or argv[2] not in ('21', '24', '32'):
        print('usage : forme_niveau.py <mhgp12_tower_forest_oracle> <bits>', file=sys.stderr)
        return USAGE
    tool, bits = argv[1], int(argv[2])
    text, _expected, _levels = oracle_tour.case_text('forme25', POINTS, K, 'v12_indices')
    other = swapped(text)
    if other is None:
        print('cas : deux boules de meme rang et de supports differents introuvables', file=sys.stderr)
        return DISAGREEMENT
    gaps, seen = [], {}
    for name, body in (('morton', text), ('positions', other)):
        dumps = tempfile.mkdtemp(prefix='mhgp12_forme_')
        try:
            done = subprocess.run([tool, '--vidages', dumps], input=body, capture_output=True, text=True, check=False)
            path = os.path.join(dumps, '0', 'tour.ful1')
            if done.returncode != 0 or 'statut ok' not in done.stdout or not os.path.isfile(path):
                print('outil : %s %s' % (done.stdout.strip()[-200:], done.stderr.strip()[-200:]), file=sys.stderr)
                return REFUSED
            with open(path, 'rb') as handle:
                raw = hashlib.sha256(handle.read()).hexdigest()
            semantic = full_reader.inspect(path, bits, K, len(POINTS))['sha256']
            form = level_form_of_first_merge(path)
        finally:
            shutil.rmtree(dumps, ignore_errors=True)
        seen[name] = raw
        if semantic != SEMANTIC:
            gaps.append('%s : empreinte semantique %s, attendu %s' % (name, semantic, SEMANTIC))
        if form != FORMS[name]:
            gaps.append('%s : forme du niveau 25 %r, attendu %r' % (name, form, FORMS[name]))
        if bits == 21 and raw != RAW_21[name]:
            gaps.append('%s : octets %s, attendu %s' % (name, raw, RAW_21[name]))
    if seen['morton'] == seen['positions']:
        gaps.append('octets identiques dans les deux ordres')
    if gaps:
        for gap in gaps:
            print(gap, file=sys.stderr)
        return DISAGREEMENT
    print('forme_niveau_ok profil=%d semantique=%s octets_differents=oui' % (bits, SEMANTIC[:16]))
    return OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
