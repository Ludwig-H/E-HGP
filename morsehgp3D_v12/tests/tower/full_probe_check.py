#!/usr/bin/env python3
"""Porte de la sonde FULL residente (bench/full_probe.cpp), voie CPU, sans donnees reelles.

Deux petits nuages deterministes ecrits au format u32le (coordonnees de 0 a 2^14, positions distinctes) :
  1. une trame, trois passes, --digest : trois lignes "full" conformes (statut ok, K, fils, sites), une ligne
     "liberation" par passe, la ligne de sortie conforme ; empreinte FUL1 identique d'une passe a l'autre et EGALE a
     celle de mhgp12_tower_chain sur la meme entree (meme chaine, memes octets) ; etages du mur disjoints (somme au plus
     le mur) ;
  2. deux trames en alternance, quatre passes : empreintes alternees (passe 0 = passe 2, passe 1 = passe 3), differentes
     d'une trame a l'autre.
Usage : full_probe_check.py <mhgp12_full_probe> <mhgp12_tower_chain>. Codes : 0 conforme ; 1 ecart ; 2 usage.
Python 3.10 nu, aucun assert (tient sous -O).
"""
import json
import os
import struct
import subprocess
import sys
import tempfile

K = 3
THREADS = 3


def write_cloud(folder, name, count, seed):
    """Nuage deterministe (generateur congruentiel), positions distinctes ; rend (xyz, ids)."""
    state, seen, points = seed, set(), []
    while len(points) < count:
        coords = []
        for _ in range(3):
            state = (state * 6364136223846793005 + 1442695040888963407) % (1 << 64)
            coords.append((state >> 33) % (1 << 14))
        key = tuple(coords)
        if key not in seen:
            seen.add(key)
            points.append(key)
    xyz, ids = os.path.join(folder, name + '.u32le'), os.path.join(folder, name + '.ids.u32le')
    with open(xyz, 'wb') as out:
        out.write(b''.join(struct.pack('<3I', *p) for p in points))
    with open(ids, 'wb') as out:
        out.write(b''.join(struct.pack('<I', 1000 + i) for i in range(count)))
    return xyz, ids


def run(argv):
    done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=600)
    rows = []
    for raw in done.stdout.decode('utf-8', 'replace').splitlines():
        try:
            row = json.loads(raw)
        except ValueError:
            return done.returncode, None
        if not isinstance(row, dict):
            return done.returncode, None
        rows.append(row)
    return done.returncode, rows


def check_passes(rows, passes, errors, label):
    fulls = [r for r in rows if r.get('phase') == 'full']
    frees = [r for r in rows if r.get('phase') == 'liberation']
    if len(fulls) != passes or len(frees) != passes or rows[-1] != {'phase': 'exit', 'status': 'ok', 'reason': 'none'}:
        errors.append('%s : %d passes, %d liberations, fin %s' % (label, len(fulls), len(frees), rows[-1]))
        return []
    for i, r in enumerate(fulls):
        stages = r.get('etapes_ns', {})
        disjoint = sum(stages.get(key, 0) for key in ('P', 'C', 'G', 'raccord', 'TMVR'))
        if r.get('pass') != i or r.get('status') != 'ok' or r.get('kmax') != K or r.get('threads') != THREADS or \
                r.get('voie') != 'cpu' or disjoint > r.get('wall_ns', 0) or \
                len(r.get('full_sha256', '')) != 64:
            errors.append('%s : passe %d non conforme' % (label, i))
    return [r.get('full_sha256') for r in fulls]


def main(argv):
    if len(argv) != 3 or not all(os.path.isfile(p) for p in argv[1:]):
        print('full_probe_check : usage', file=sys.stderr)
        return 2
    probe, chain = argv[1], argv[2]
    errors = []
    with tempfile.TemporaryDirectory() as folder:
        xyz_a, ids_a = write_cloud(folder, 'a', 600, 7)
        xyz_b, ids_b = write_cloud(folder, 'b', 500, 11)
        code, rows = run([probe, '--trame=%s,%s,a' % (xyz_a, ids_a), '--k=%d' % K, '--threads=%d' % THREADS,
                          '--passes=3', '--digest'])
        digests = check_passes(rows or [{}], 3, errors, 'une trame') if code == 0 and rows else []
        if code != 0 or len(set(digests)) != 1:
            errors.append('une trame : code %s, empreintes %s' % (code, digests))
        code_c, rows_c = run([chain, xyz_a, ids_a, str(K), '--fils', str(THREADS)])
        chain_sha = rows_c[0].get('sha256') if code_c == 0 and rows_c else None
        if not digests or chain_sha != digests[0]:
            errors.append('empreinte de la sonde differente de mhgp12_tower_chain : %s / %s' % (digests[:1], chain_sha))
        code, rows = run([probe, '--trame=%s,%s,a' % (xyz_a, ids_a), '--trame=%s,%s,b' % (xyz_b, ids_b),
                          '--k=%d' % K, '--threads=%d' % THREADS, '--passes=4', '--digest'])
        alt = check_passes(rows or [{}], 4, errors, 'deux trames') if code == 0 and rows else []
        if code != 0 or len(alt) != 4 or alt[0] != alt[2] or alt[1] != alt[3] or alt[0] == alt[1] or \
                (digests and alt[0] != digests[0]):
            errors.append('deux trames : code %s, empreintes %s' % (code, alt))
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('full_probe_ok passes=7 trames=2 identite_chaine=oui')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
