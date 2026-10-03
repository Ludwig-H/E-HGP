#!/usr/bin/env python3
"""Catalogue v2, sous-ensemble n <= 9 (19 entrees, K <= 3), avec MON FULL et MES regles ; deux juges (v10, moi).
Meme protocole que verdict/catalogue.py v10 : familles des fixtures de l'utilisateur ecartees, une hierarchie par
variante (m = max(K+1, mcs) pour Hmcs), jugee a chaque mcs de la plage et chaque cible."""
import sys, json, time, hashlib
sys.dont_write_bytecode = True
import indep_full as I
import cells as C
ver = C.ver
V2 = C.V2
SKIP = ('cible_utilisateur_T0', 'question_Q1', 'question_Q2', 'question_Q3', 'question_Q4', 'famille_T1_L')

def run(rules, nmax, builder):
    v2 = json.load(open(V2))
    tot = {r: [0, 0] for r in rules}
    per = {r: [] for r in rules}
    dis = 0
    t0 = time.time()
    for e in v2['fixtures']:
        if e['famille'] in SKIP or len(e['points']) > nmax:
            continue
        K = e['K']; lo, hi = e['mcs']
        vars_ = [('base', e['points'], e['target'])] + [(v['name'], v['points'], v['target']) for v in e.get('variants', [])]
        res = {r: [0, 0] for r in rules}
        for vn, pts, cibles in vars_:
            noms = list(pts); P = [tuple(pts[x]) for x in noms]
            T = builder(P, K)
            ix = {x: i for i, x in enumerate(noms)}
            for r in rules:
                cache = {}
                for mcs in range(lo, hi + 1):
                    k = mcs if r in C.DEPEND_MCS else 0
                    if k not in cache:
                        U, kind = C.build(r, T, mcs)
                        cache[k] = I.Hier(U, kind)
                    h = cache[k]
                    for c in cibles:
                        ok, _v = ver.juger(h, c, ix, mcs)
                        okm, _r = C.mon_juger(h, c, ix, mcs)
                        dis += ok != okm
                        res[r][0] += ok; res[r][1] += 1
        for r in rules:
            tot[r][0] += res[r][0]; tot[r][1] += res[r][1]
            per[r].append((e['name'], res[r][0], res[r][1]))
    fp = {r: hashlib.sha256(json.dumps(per[r]).encode()).hexdigest()[:16] for r in rules}
    return {'total': tot, 'par_entree': per, 'desaccords_juges': dis, 'empreintes': fp, 'entrees': len(per[rules[0]]),
            'secondes': round(time.time() - t0, 1)}

if __name__ == '__main__':
    rules = sys.argv[1].split(',')
    out = run(rules, 9, I.Full)
    json.dump(out, open('recus/catalogue9_%s.json' % '_'.join(rules), 'w'), indent=1)
    print(json.dumps({k: v for k, v in out.items() if k != 'par_entree'}))
