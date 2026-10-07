"""Audit L06 : empreinte canonique (Merkle) d'un dump de mhgp10_tower, independante de la numerotation des noeuds et des
sites, donc comparable entre nombres de fils, entre isometries de la grille, et demain entre v10 et v11.

Par ordre k (lecture en flux, un ordre a la fois) :
  niveau canonique = fraction reduite « num/den » ;
  h(naissance) = sha256("B|" + niveau) ; h(fusion) = sha256("M|" + niveau + "|" + hachages hexadecimaux des enfants,
  tries, joints par "|") ;
  foret     = h(racine) ;
  verticale = sha256 de la liste triee des chaines h_k(v) + h_{k-1}(image(v)), jointes par "|" ("-" a l'ordre 1 ou
              si les images sont absentes) ;
  attaches  = sha256 de la liste triee des chaines niveau_d_entree + "@" + h(noeud d'attache), jointes par "|"
              (sans coordonnees : invariant par isometrie ; "-" sans attaches).
Usage : python3 tower_merkle.py DUMP [DUMP2]   (deux dumps : compare, code 1 si une empreinte differe)
Sortie : une ligne JSON par ordre.
"""
import hashlib
import json
import sys
from math import gcd


def canon(num, den):
    n, d = int(num), int(den)
    g = gcd(n, d)
    return '%d/%d' % (n // g, d // g)


def finish(k, o, hprev):
    nn = len(o['parent'])
    kids = [[] for _ in range(nn)]
    root = -1
    for v, p in enumerate(o['parent']):
        if p >= 0:
            kids[p].append(v)
        else:
            root = v
    h = [None] * nn
    for v in range(nn):  # les enfants sont crees avant leur parent (identifiants croissants)
        if kids[v]:
            hs = sorted(h[c] for c in kids[v])
            h[v] = hashlib.sha256(('M|' + o['level'][v] + '|' + '|'.join(hs)).encode()).hexdigest()
        else:
            h[v] = hashlib.sha256(('B|' + o['level'][v]).encode()).hexdigest()
    vert = '-'
    if hprev is not None and nn and all(x >= 0 for x in o['lower']):
        pairs = sorted(h[v] + hprev[o['lower'][v]] for v in range(nn))
        vert = hashlib.sha256('|'.join(pairs).encode()).hexdigest()
    att = '-'
    if o['points']:
        att = hashlib.sha256('|'.join(sorted(lv + '@' + h[v] for v, lv in o['points'])).encode()).hexdigest()
    arity = {}
    for v in range(nn):
        if kids[v]:
            arity[len(kids[v])] = arity.get(len(kids[v]), 0) + 1
    rec = dict(ordre=k, noeuds=nn, naissances=sum(1 for v in range(nn) if not kids[v]),
               fusions_par_arite={str(a): c for a, c in sorted(arity.items())}, niveaux_distincts=len(set(o['level'])),
               foret=h[root], verticale=vert, attaches=att)
    return rec, h


def digest(path):
    out = []
    cur = None
    k = None
    hprev = None
    with open(path) as fh:
        for line in fh:
            t = line.split()
            if t[0] == 'order':
                if cur is not None:
                    rec, hprev = finish(k, cur, hprev)
                    out.append(rec)
                cur = dict(parent=[], level=[], lower=[], points=[])
                k = int(t[1])
            elif t[0] == 'node':
                cur['parent'].append(int(t[2]))
                cur['level'].append(canon(t[3], t[4]))
                cur['lower'].append(int(t[5]))
            elif len(t) == 8:
                cur['points'].append((int(t[4]), canon(t[6], t[7])))
            else:
                cur['points'].append((int(t[4]), canon(t[5], 1)))
    if cur is not None:
        rec, hprev = finish(k, cur, hprev)
        out.append(rec)
    return out


def main():
    a = digest(sys.argv[1])
    for r in a:
        print(json.dumps(r, sort_keys=True), flush=True)
    if len(sys.argv) > 2:
        b = digest(sys.argv[2])
        bad = 0
        for ra, rb in zip(a, b):
            for key in ('foret', 'verticale', 'attaches'):
                same = ra[key] == rb[key]
                bad += not same
                print('ordre %d %s : %s' % (ra['ordre'], key, 'identique' if same else 'DIFFERENT'))
        return 1 if bad or len(a) != len(b) else 0
    return 0


if __name__ == '__main__':
    sys.exit(main())
