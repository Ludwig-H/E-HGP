#!/usr/bin/env python3
"""Porte de la sonde FULL residente (bench/full_probe.cpp), voie CPU, sans donnees reelles.

Deux petits nuages deterministes ecrits au format u32le (coordonnees de 0 a 2^14, positions distinctes) :
  1. une trame, trois passes, --digest : trois lignes "full" conformes (statut ok, K, fils, sites), une ligne
     "liberation" par passe, la ligne de sortie conforme ; empreinte FUL1 identique d'une passe a l'autre et EGALE a
     celle de mhgp12_tower_chain sur la meme entree (meme chaine, memes octets) ; etages du mur disjoints (somme au plus
     le mur) ; memoire par etage presente pour P, C, G, raccord et TMVR, usage au plus le pic, plus haut pic egal a
     pic_octets ;
  2. deux trames en alternance, quatre passes : empreintes alternees (passe 0 = passe 2, passe 1 = passe 3), differentes
     d'une trame a l'autre.
Voie par defaut : aucun champ du schema recouvert (etapes_schema, recouvrement, fenetres_ns). Avec --recouvert, la
sonde joue build_tower (Session recouverte, decision D-F2) et chaque ligne "full" doit suivre son schema (en-tete de
bench/full_probe.cpp) : etapes_schema = "recouvert" ; partition murale P, C, G, raccord (nul), TMVR, de somme au plus le
mur ; fenetres_ns (sommes de fenetres murales des taches) avec T + M + V + R et foret_apres_g au plus foret ;
memoire_octets P, C, tour, usage au plus le pic, plus haut pic egal a pic_octets ; recouvrement coherent (fin_g_ns = G,
queue_ns = TMVR = fin_ns - fin_g_ns, fin_ns au plus tour_ns, arrets au plus reprises) ; fins par ordre dans [0, fin_ns],
celles de G au plus fin_g_ns, V nulle a l'ordre 1 ; et la MEME empreinte FUL1 que mhgp12_tower_chain (voie sequentielle).
Usage : full_probe_check.py <mhgp12_full_probe> <mhgp12_tower_chain> [--recouvert]. Codes : 0 conforme ; 1 ecart ;
2 usage. Python 3.10 nu, aucun assert (tient sous -O).
"""
import json
import os
import struct
import subprocess
import sys
import tempfile

K = 3
THREADS = 3
MEM_STAGES = ('P', 'C', 'G', 'raccord', 'TMVR')
WALL_STAGES = ('P', 'C', 'G', 'raccord', 'TMVR')
OVERLAP_MEM = ('P', 'C', 'tour')
WINDOWS = ('G', 'foret', 'foret_apres_g', 'T', 'M', 'V', 'R')
OVERLAP = ('tour_ns', 'ouverture_ns', 'fin_g_ns', 'fin_ns', 'queue_ns', 'noyau_reprises', 'noyau_arrets', 'admis_octets')


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


def ints(block, keys):
    """Bloc objet aux cles exactes `keys`, valeurs entieres positives ou nulles."""
    return type(block) is dict and set(block) == set(keys) and \
        all(type(block[k]) is int and block[k] >= 0 for k in keys)


def overlapped_errors(r):
    """Ecarts d'une ligne "full" au schema recouvert (liste vide si conforme)."""
    out = []
    st, win, rec, mem, g = r.get('etapes_ns'), r.get('fenetres_ns'), r.get('recouvrement'), r.get('memoire_octets'), \
        r.get('g_ns')
    if r.get('etapes_schema') != 'recouvert' or not ints(st, WALL_STAGES) or st['raccord'] != 0 or \
            sum(st.values()) > r.get('wall_ns', 0):
        return ['partition murale']
    if not ints(win, WINDOWS) or win['T'] + win['M'] + win['V'] + win['R'] > win['foret'] or \
            win['foret_apres_g'] > win['foret']:
        out.append('fenetres murales')
    if type(mem) is not dict or set(mem) != set(OVERLAP_MEM) or \
            any(type(mem[k]) is not list or len(mem[k]) != 2 or mem[k][0] > mem[k][1] for k in OVERLAP_MEM) or \
            max(mem[k][1] for k in OVERLAP_MEM) != r.get('pic_octets'):
        out.append('memoire')
    if not ints(g, ('ouverture', 'tables')) or g['tables'] > g['ouverture'] or g['ouverture'] > st['G']:
        out.append('ouverture')
    if not ints(rec, OVERLAP) or rec['fin_g_ns'] != st['G'] or rec['queue_ns'] != st['TMVR'] or \
            rec['queue_ns'] != rec['fin_ns'] - rec['fin_g_ns'] or rec['fin_ns'] > rec['tour_ns'] or \
            rec['noyau_arrets'] > rec['noyau_reprises'] or rec['admis_octets'] == 0:
        return out + ['recouvrement']
    ends = r.get('fins_par_ordre_ns')
    if type(ends) is not list or len(ends) != K or \
            any(type(e) is not list or len(e) != 5 or any(type(x) is not int or not 0 <= x <= rec['fin_ns'] for x in e)
                for e in ends) or any(e[0] > rec['fin_g_ns'] for e in ends) or ends[0][3] != 0:
        out.append('fins par ordre')
    return out


def check_passes(rows, passes, errors, label, overlapped=False):
    fulls = [r for r in rows if r.get('phase') == 'full']
    frees = [r for r in rows if r.get('phase') == 'liberation']
    if len(fulls) != passes or len(frees) != passes or rows[-1] != {'phase': 'exit', 'status': 'ok', 'reason': 'none'}:
        errors.append('%s : %d passes, %d liberations, fin %s' % (label, len(fulls), len(frees), rows[-1]))
        return []
    for i, r in enumerate(fulls):
        if overlapped:
            errors += ['%s : passe %d, schema recouvert : %s' % (label, i, e) for e in overlapped_errors(r)]
            if r.get('pass') != i or r.get('status') != 'ok' or r.get('kmax') != K or \
                    r.get('threads') != THREADS or r.get('voie') != 'cpu' or len(r.get('full_sha256', '')) != 64:
                errors.append('%s : passe %d non conforme' % (label, i))
            continue
        if any(key in r for key in ('etapes_schema', 'recouvrement', 'fenetres_ns', 'fins_par_ordre_ns')):
            errors.append('%s : passe %d, champ du schema recouvert dans la voie par defaut' % (label, i))
        stages = r.get('etapes_ns', {})
        disjoint = sum(stages.get(key, 0) for key in ('P', 'C', 'G', 'raccord', 'TMVR'))
        mem = r.get('memoire_octets')
        if type(mem) is not dict or sorted(mem) != sorted(MEM_STAGES) or \
                any(type(mem[k]) is not list or len(mem[k]) != 2 or mem[k][0] > mem[k][1] for k in MEM_STAGES) or \
                max(mem[k][1] for k in MEM_STAGES) != r.get('pic_octets'):
            errors.append('%s : passe %d, memoire par etage absente ou incoherente avec pic_octets' % (label, i))
        if r.get('pass') != i or r.get('status') != 'ok' or r.get('kmax') != K or r.get('threads') != THREADS or \
                r.get('voie') != 'cpu' or disjoint > r.get('wall_ns', 0) or \
                len(r.get('full_sha256', '')) != 64:
            errors.append('%s : passe %d non conforme' % (label, i))
    return [r.get('full_sha256') for r in fulls]


def main(argv):
    overlapped = len(argv) == 4 and argv[3] == '--recouvert'
    if len(argv) not in (3, 4) or (len(argv) == 4 and not overlapped) or \
            not all(os.path.isfile(p) for p in argv[1:3]):
        print('full_probe_check : usage', file=sys.stderr)
        return 2
    probe, chain = argv[1], argv[2]
    mode = ['--recouvert'] if overlapped else []
    errors = []
    with tempfile.TemporaryDirectory() as folder:
        xyz_a, ids_a = write_cloud(folder, 'a', 600, 7)
        xyz_b, ids_b = write_cloud(folder, 'b', 500, 11)
        code, rows = run([probe, '--trame=%s,%s,a' % (xyz_a, ids_a), '--k=%d' % K, '--threads=%d' % THREADS,
                          '--passes=3', '--digest'] + mode)
        digests = check_passes(rows or [{}], 3, errors, 'une trame', overlapped) if code == 0 and rows else []
        if code != 0 or len(set(digests)) != 1:
            errors.append('une trame : code %s, empreintes %s' % (code, digests))
        code_c, rows_c = run([chain, xyz_a, ids_a, str(K), '--fils', str(THREADS)])
        chain_sha = rows_c[0].get('sha256') if code_c == 0 and rows_c else None
        if not digests or chain_sha != digests[0]:
            errors.append('empreinte de la sonde differente de mhgp12_tower_chain : %s / %s' % (digests[:1], chain_sha))
        code, rows = run([probe, '--trame=%s,%s,a' % (xyz_a, ids_a), '--trame=%s,%s,b' % (xyz_b, ids_b),
                          '--k=%d' % K, '--threads=%d' % THREADS, '--passes=4', '--digest'] + mode)
        alt = check_passes(rows or [{}], 4, errors, 'deux trames', overlapped) if code == 0 and rows else []
        if code != 0 or len(alt) != 4 or alt[0] != alt[2] or alt[1] != alt[3] or alt[0] == alt[1] or \
                (digests and alt[0] != digests[0]):
            errors.append('deux trames : code %s, empreintes %s' % (code, alt))
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('full_probe_ok passes=7 trames=2 identite_chaine=oui' + (' schema=recouvert' if overlapped else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
