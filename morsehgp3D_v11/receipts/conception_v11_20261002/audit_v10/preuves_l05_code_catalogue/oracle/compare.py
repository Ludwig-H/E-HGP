"""Compare le dump de mhgp10_catalogue a l'oracle brut independant (brute_oracle.cpp).
Controles : meme multiensemble (q_min, p, u, I, U) ; S* du dump == plus petit support de cardinal q_min dans l'ordre de
Morton (oracle) ; niveaux : rang dense depuis 0, egalite de rang <=> egalite de niveau, croissance stricte ; drapeaux
(etendue : |U| > q ; ponderee : un site de coquille de poids > 1).
usage : python3 compare.py CATALOGUE_EXE ORACLE_EXE IN.u32le K [options catalogue...]   -> code 0 conforme, 1 ecart, 2 refus"""
import collections
import os
import subprocess
import sys
import tempfile
from fractions import Fraction


def main():
    exe, orc, src, K = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
    extra = sys.argv[5:]
    raw = open(src, 'rb').read()
    pts = [tuple(int.from_bytes(raw[12 * i + 4 * j:12 * i + 4 * j + 4], 'little') for j in range(3)) for i in range(len(raw) // 12)]
    w = collections.Counter(pts)
    with tempfile.TemporaryDirectory() as tmp:
        dump = os.path.join(tmp, 'd.txt')
        r = subprocess.run([exe, src, '--k=%d' % K, '--dump=' + dump] + extra, capture_output=True, text=True)
        if r.returncode != 0:
            print('REFUS catalogue', r.stdout.strip()[:200])
            return 2
        o = subprocess.run([orc, src, str(K)], capture_output=True, text=True)
        if o.returncode != 0:
            print('REFUS oracle', o.stderr.strip()[:200])
            return 2
        conv = lambda s: [tuple(int(v) for v in t.split(',')) for t in s.split()]  # noqa: E731
        want = {}
        for line in o.stdout.splitlines():
            head, I, U, S, lev = line.split('|')
            q, p, u = (int(t) for t in head.split())
            num, den = lev.split('/')
            key = (q, p, u, tuple(sorted(conv(I))), tuple(sorted(conv(U))))
            if key in want:
                print('ORACLE : cle en double')
                return 1
            want[key] = (tuple(conv(S)), Fraction(int(num), int(den)))
        got = {}
        rows = []
        for line in open(dump):
            head, S, I, U = line.rstrip('\n').split('|')
            rank, q, p, u, flags = (int(t) for t in head.split())
            Ic, Uc, Sc = conv(I), conv(U), conv(S)
            key = (q, p, u, tuple(sorted(Ic)), tuple(sorted(Uc)))
            if key in got:
                print('ECART : boule en double', key[:3])
                return 1
            got[key] = (rank, tuple(Sc), flags)
            rows.append((rank, key))
            ext = 1 if len(Uc) > q else 0
            wsh = 2 if any(w[c] > 1 for c in Uc) else 0
            if flags != ext | wsh:
                print('ECART : drapeaux', flags, ext | wsh, key[:3])
                return 1
            if p != sum(w[c] for c in Ic) or u != sum(w[c] for c in Uc):
                print('ECART : poids', key[:3])
                return 1
    miss, extra_ = set(want) - set(got), set(got) - set(want)
    if miss or extra_:
        print('ECART : manquantes %d, en trop %d ; ex. %s %s' % (len(miss), len(extra_), [k[:3] for k in list(miss)[:3]], [k[:3] for k in list(extra_)[:3]]))
        return 1
    bad_sup = sum(1 for k in got if got[k][1] != want[k][0])
    if bad_sup:
        k = next(k for k in got if got[k][1] != want[k][0])
        print('ECART : S* non canonique sur %d boules ; ex. dump %s oracle %s' % (bad_sup, got[k][1], want[k][0]))
        return 1
    rows.sort(key=lambda t: t[0])
    levels = sorted(set(v[1] for v in want.values()))
    rank_of = {l: i for i, l in enumerate(levels)}
    bad_rank = sum(1 for rank, key in rows if rank != rank_of[want[key][1]])
    if bad_rank:
        print('ECART : rangs (dense, depuis 0) faux sur %d boules' % bad_rank)
        return 1
    # ordre du dump : (niveau, S*) croissant -> verifie sur le fichier par les rangs deja controles
    next_ = sum(1 for k in got if got[k][2] & 1)
    nw = sum(1 for k in got if got[k][2] & 2)
    print('CONFORME K=%d sites=%d poids=%d boules=%d niveaux=%d etendues=%d ponderees=%d | %s' % (
        K, len(w), len(pts), len(got), len(levels), next_, nw, r.stdout.splitlines()[0][-150:]))
    return 0


if __name__ == '__main__':
    sys.exit(main())
