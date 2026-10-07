#!/usr/bin/env python3
"""Oracle borne du catalogue (CONTRAT_CATALOGUE.md, paragraphe 6.2) : egalite de Cat_K avec l'etage B de la reference
exacte (reference/hgp12_ref/constructive.py, entiers Python, force brute) sur la suite rapide (n <= 14).

    oracle.py <mhgp12_catalogue_probe> [--min-clouds N] [--min-balls N]

Pour chaque nuage de families.fast_suite() : la sonde calcule Cat_K (K du nuage, feuille K+3, deux fils) et l'exporte
(MHGP12DP) ; la reference construit son catalogue (regle d'admission unique p + q_min <= K+1) dont on garde les boules
positives (q_min >= 2). Juge : bijection des boules par (centre exact, rayon carre) recalcules depuis S* ; p, m, q_min,
I et U egaux comme ensembles de POSITIONS ; S* egal au support minimal de plus petite liste triee de positions
(convention de la v12, CST-0113), calcule par la reference sur la coquille rangee par positions ; rangs egaux aux rangs
denses des niveaux de la reference ; ordre publie (rang, S* par positions) strictement croissant ; nombre de niveaux.
Un nuage a doublons doit etre refuse (code 2, multiplicity_unsupported, decision D8).
Codes : 0 conforme ; 1 ecart ; 2 refus (usage, sonde absente) ; 3 plancher non atteint. Python 3.10 nu, aucun assert.
"""
import os
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import catalogue_dump as dump  # noqa: E402  (lecteur MHGP12DP du meme dossier)
from hgp12_ref import families  # noqa: E402
from hgp12_ref import intgeom as G  # noqa: E402
from hgp12_ref.constructive import Reference  # noqa: E402

MAX_SITES = 14


def write_cloud(folder, points):
    xyz, ids = os.path.join(folder, 'in.u32le'), os.path.join(folder, 'in.ids.u32le')
    with open(xyz, 'wb') as handle:
        handle.write(b''.join(struct.pack('<3I', *p) for p in points))
    with open(ids, 'wb') as handle:
        handle.write(struct.pack('<%dI' % len(points), *range(len(points))))
    return xyz, ids


def run_probe(probe, folder, points, kmax, ordinal):
    xyz, ids = write_cloud(folder, points)
    out = os.path.join(folder, 'out%d' % ordinal)  # dossier transactionnel neuf : jamais un dossier existant
    cmd = [probe, xyz, ids, '--k=%d' % kmax, '--leaf=%d' % (kmax + 3), '--threads=2', '--out=%s' % out]
    done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    text = done.stdout.decode('ascii', 'replace')
    if done.returncode != 0:
        return done.returncode, text, None
    return 0, text, dump.Catalogue(dump.read_dump(os.path.join(out, 'cat.bin')))


def expected(points, kmax):
    """Boules positives de la reference : cle (centre, rayon carre) -> (p, m, q, I, U, S* v12) en positions."""
    ref = Reference(points, kmax, admission='single')
    balls = {}
    for b in ref.balls:
        if b.qmin < 2:
            continue
        shell = sorted(ref.sites[s] for s in b.shell_sites)
        found = G.canonical_support(shell, b.anchor, b.ctr)  # premier dans l'ordre des positions
        sstar = tuple(shell[i] for i in found[1]) if found is not None else None
        balls[(b.center, b.level)] = (b.p, b.m, b.qmin, frozenset(ref.sites[s] for s in b.inner_sites),
                                      frozenset(shell), sstar)
    return balls


def judge(points, kmax, cat, errors, name):
    want = expected(points, kmax)
    levels = sorted({key[1] for key in want})
    rank_of = {level: i + 1 for i, level in enumerate(levels)}
    seen = set()
    for b in range(cat.count):
        key = dump.ball_key(cat, cat.sstar[b])
        if key is None or key not in want or key in seen:
            errors.append('%s : boule %d (S* %r) absente de la reference ou en double' % (name, b, cat.sstar[b]))
            continue
        seen.add(key)
        p, m, q, inner, shell, sstar = want[key]
        population = list(cat.population(b))
        got = (cat.p[b], cat.m[b], cat.q[b], frozenset(cat.pos[s] for s in population[:cat.p[b]]),
               frozenset(cat.pos[s] for s in population[cat.p[b]:]), tuple(sorted(cat.pos[s] for s in cat.sstar[b])))
        if got != (p, m, q, inner, shell, sstar) or cat.rank[b] != rank_of[key[1]]:
            errors.append('%s : boule %d differente de la reference' % (name, b))
    if len(seen) != len(want):
        errors.append('%s : %d boules de la reference absentes' % (name, len(want) - len(seen)))
    if cat.levels != len(levels) + 1:
        errors.append('%s : %d niveaux, attendu %d' % (name, cat.levels, len(levels) + 1))
    dump.check_order(cat, errors)
    return len(want)


def main(argv):
    if len(argv) < 2 or not os.path.isfile(argv[1]):
        print('oracle_refus usage : oracle.py <mhgp12_catalogue_probe> [--min-clouds N] [--min-balls N]')
        return 2
    floors = {'--min-clouds': 300, '--min-balls': 1000}
    for item in argv[2:]:
        key, _, value = item.partition('=')
        if key not in floors or not value.isdigit():
            print('oracle_refus option %s' % item)
            return 2
        floors[key] = int(value)
    errors, clouds, balls, refused = [], 0, 0, 0
    with tempfile.TemporaryDirectory() as folder:
        for cloud in families.fast_suite():
            if len(cloud.points) > MAX_SITES:
                continue
            code, text, cat = run_probe(argv[1], folder, cloud.points, cloud.kmax, clouds)
            clouds += 1
            if len(set(cloud.points)) != len(cloud.points):
                if code != 2 or '"reason":"multiplicity_unsupported"' not in text:
                    errors.append('%s : doublons non refuses (code %d)' % (cloud.name, code))
                refused += 1
                continue
            if code != 0:
                errors.append('%s : sonde en refus (code %d)' % (cloud.name, code))
                continue
            balls += judge(cloud.points, cloud.kmax, cat, errors, cloud.name)
    for error in errors[:20]:
        print('ecart %s' % error)
    if errors:
        print('catalogue_oracle_ecart ecarts=%d' % len(errors))
        return 1
    if clouds < floors['--min-clouds'] or balls < floors['--min-balls']:
        print('catalogue_oracle_plancher nuages=%d boules=%d' % (clouds, balls))
        return 3
    print('catalogue_oracle_ok nuages=%d boules=%d doublons_refuses=%d' % (clouds, balls, refused))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
