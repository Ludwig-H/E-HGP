#!/usr/bin/env python3
"""Catalogue v2, n <= 16 et K <= kmax, MON FULL (aretes de Johnson) et MES regles, juge v10 + mon juge ;
comparaison entree par entree au recu du rapport (marge_cibles/recus/catalogue_hm_n16_b.json, donnees seulement)
et aux recus natifs v10 (cat_natif_lib.json : core, A5_U1, P_2)."""
import sys, json, time
sys.dont_write_bytecode = True
import indep_full as I
import indep_full_j as J
import hier_fast as HF
import cells as C
ver = C.ver
SKIP = ('cible_utilisateur_T0', 'question_Q1', 'question_Q2', 'question_Q3', 'question_Q4', 'famille_T1_L')
kmax = int(sys.argv[2]); rules = sys.argv[1].split(","); FAST = len(sys.argv) > 3
rep = {e['name']: e for e in json.load(open('/workspaces/E-HGP/build/v11-points-math/marge_cibles/recus/catalogue_hm_n16_b.json'))['entrees']}
nat = {e['name']: e for e in json.load(open('/workspaces/E-HGP/build/v10-verrou-points/juge_final/verdict/recus/cat_natif_lib.json'))['entrees']}
REP = {'first': 'first_k1'}
NAT = {'core': 'core', 'cover': 'A5_U1', 'P2r': 'P_2'}
v2 = json.load(open(C.V2))
tot = {r: [0, 0] for r in rules}; diff_rep = []; diff_nat = []; dis = 0; t0 = time.time(); per = []
for e in v2['fixtures']:
    if e['famille'] in SKIP or len(e['points']) > 16 or e['K'] > kmax:
        continue
    K = e['K']; lo, hi = e['mcs']; t1 = time.time()
    vars_ = [('base', e['points'], e['target'])] + [(v['name'], v['points'], v['target']) for v in e.get('variants', [])]
    res = {r: [0, 0] for r in rules}
    for vn, pts, cibles in vars_:
        noms = list(pts); P = [tuple(pts[x]) for x in noms]
        T = J.FullJ(P, K); ix = {x: i for i, x in enumerate(noms)}
        for r in rules:
            cache = {}
            for mcs in range(lo, hi + 1):
                k = mcs if r in C.DEPEND_MCS else 0
                if k not in cache:
                    U, kind = C.build(r, T, mcs); cache[k] = HF.HierFast(U, kind)
                h = cache[k]
                for c in cibles:
                    ok, _v = ver.juger(h, c, ix, mcs); okm = ok if FAST else C.mon_juger(h, c, ix, mcs)[0]
                    dis += ok != okm; res[r][0] += ok; res[r][1] += 1
    for r in rules:
        tot[r][0] += res[r][0]; tot[r][1] += res[r][1]
        rr = rep[e['name']]['resultats'][REP.get(r, r)]
        if (rr['passes'], rr['jugements']) != tuple(res[r]):
            diff_rep.append((e['name'], r, res[r], (rr['passes'], rr['jugements'])))
        if r in NAT:
            nn = nat[e['name']]['resultats'][NAT[r]]
            if (nn['passes'], nn['jugements']) != tuple(res[r]):
                diff_nat.append((e['name'], r, res[r], (nn['passes'], nn['jugements'])))
    per.append((e['name'], K, len(e['points']), {r: res[r] for r in rules}))
    print('%-46s K%d n%d %5.1fs %s' % (e['name'], K, len(e['points']), time.time() - t1,
                                       ' '.join('%s=%d/%d' % (r, res[r][0], res[r][1]) for r in rules)), flush=True)
out = {'kmax': kmax, 'entrees': len(per), 'total': tot, 'ecarts_au_recu_du_rapport': diff_rep,
       'ecarts_aux_recus_natifs_v10': diff_nat, 'desaccords_juges': dis, 'par_entree': per, 'secondes': round(time.time() - t0, 1)}
json.dump(out, open('recus/catalogue_kf%d_%s.json' % (kmax, '_'.join(rules)), 'w'), indent=1)
print(json.dumps({k: v for k, v in out.items() if k != 'par_entree'}))
