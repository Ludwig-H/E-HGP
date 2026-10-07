#!/usr/bin/env python3
"""Catalogues amputes (audit L02) : pour chaque boule admissible du produit (p + q <= K + 1), on la retire du catalogue
et on regarde ce que fait la tour, puis ce que dit l'invariant d'Euler.

Issues par retrait :
  refus      : la tour refuse (raison publiee) ;
  sans_effet : la tour publie une foret identique a l'arbre exact de Gamma_k (la boule n'etait pas necessaire) ;
  FAUX       : la tour publie, sans refus, une foret differente de l'arbre exact (zone aveugle).
Euler est calcule sur le meme catalogue ampute, construit a K + 2 : detecte si un chi_K' (K' <= K) differe de 1.
"""
import json
import os
import random
import sys
import tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from l02_judge import FAMILIES, judge_cloud


def main():
    exe = sys.argv[1]
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 16
    K = int(sys.argv[3]) if len(sys.argv) > 3 else 4
    fams = 'generic,grid,plane,clusters,sphere,circle,line,cube'.split(',')
    tot = dict(nuages=0, retraits=0, refus=0, sans_effet=0, FAUX=0, euler_detecte=0, FAUX_et_euler_detecte=0,
               FAUX_et_euler_muet=0, sans_effet_et_euler_detecte=0)
    reasons = {}
    by_kind = {}
    examples = []
    with tempfile.TemporaryDirectory(dir=os.environ.get('L02_TMP')) as tmp:
        for t in range(count):
            fam = fams[t % len(fams)]
            rnd = random.Random(424242 + t)
            n = rnd.randint(8, 10)
            P = FAMILIES[fam](rnd, n)
            rnd.shuffle(P)
            err, cnt, code = judge_cloud(exe, P, K, tmp, extra=('--list-balls',), cache=True)
            if err:
                print('nuage %d : temoin non conforme (%s)' % (t, err))
                return 1
            balls = json.loads(cnt['_stdout'])['ball_list']
            tot['nuages'] += 1
            for i, (p, q, m, ext) in enumerate(balls):
                if p + q > K + 1:
                    continue  # hors du catalogue du produit a K
                err, cnt, code = judge_cloud(exe, P, K, tmp, extra=('--drop-ball=%d' % i,), euler_strict=False, cache=True)
                tot['retraits'] += 1
                eul = cnt['euler_bad'] > 0
                tot['euler_detecte'] += eul
                kind = '%s q=%d' % ('etendue' if ext else 'reguliere', q)
                d = by_kind.setdefault(kind, dict(retraits=0, refus=0, sans_effet=0, FAUX=0))
                d['retraits'] += 1
                if code == 2:
                    tot['refus'] += 1
                    d['refus'] += 1
                    try:
                        r = json.loads(cnt['_stdout']).get('reason', '?')
                    except ValueError:
                        r = '?'
                    reasons[r] = reasons.get(r, 0) + 1
                elif err is None:
                    tot['sans_effet'] += 1
                    d['sans_effet'] += 1
                    tot['sans_effet_et_euler_detecte'] += eul
                else:
                    tot['FAUX'] += 1
                    d['FAUX'] += 1
                    tot['FAUX_et_euler_detecte' if eul else 'FAUX_et_euler_muet'] += 1
                    if len(examples) < 4:
                        examples.append(dict(famille=fam, P=P, K=K, boule=i, p=p, q=q, m=m, etendue=ext, ecart=err[:160]))
    print(json.dumps(dict(bilan=tot, raisons_de_refus=reasons, par_type=by_kind), sort_keys=True))
    for e in examples:
        print('EXEMPLE', json.dumps(e, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
