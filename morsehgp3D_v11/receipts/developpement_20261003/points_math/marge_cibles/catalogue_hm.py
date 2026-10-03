#!/usr/bin/env python3
"""Catalogue v2 (cibles condensees, 215 entrees hors fixtures de l'utilisateur) pour les pendaisons v11 calculees par
l'oracle exact (route A : hgp11_ref.Definition + points_reference.reference_rules), jugees par ver.juger (v10).

Budget : entrees de n <= --nmax sites et d'ordre K <= --kmax ; les autres sont sautees et COMPTEES.
Regles : core, cover (LCA des ex aequo), first_k1, H1 (margin1), Hk1 (margin, m = K + 1),
Hmcs (margin, m = max(K + 1, mcs)). Une hierarchie par variante (et par m pour Hmcs), jugee a chaque mcs de la plage.
Option --valider-er0h N : pour les N premieres entrees retenues, ER0h(1, 12) de la v10 (cellules.construire) est relue
par l'adaptateur et comparee entree par entree au recu publie catalogue_gamma_er0h_1_12.json.
Cadre : phase=exploration_v11_hors_registre, public_status=not_claimed. GCP non utilise. Aucun assert.
Usage : python3 -B catalogue_hm.py --nmax 16 --kmax 4 --out recus/catalogue_hm.json
"""
import argparse
import hashlib
import json
import sys
import time

sys.dont_write_bytecode = True
import adaptateur as AD  # noqa: E402

CL, ver = AD.CL, AD.ver
USER = ('cible_utilisateur_T0', 'question_Q1', 'question_Q2', 'question_Q3', 'question_Q4', 'famille_T1_L')
REGLES = ('core', 'cover', 'first_k1', 'H1', 'Hk1', 'Hmcs')
REGLES_B = ('H1r', 'Hk1r', 'P2r')
RECU_ER0H = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verdict/recus/catalogue_gamma_er0h_1_12.json'


def hierarchies_b(P, K):
    """Route B (arbre Gamma_K v10) : marge en rayon m = 1 et m = K + 1 ; famille H4 en rayon kappa = 2 (P_2 ?)."""
    T, _info = CL.full(P, K)
    out = {}
    for nom, m, kap in (('H1r', 1, 1), ('Hk1r', K + 1, 1), ('P2r', 1, 2)):
        _e, U = AD.route_b(T, m, 'r', CL.Fraction(kap))
        out[nom] = {0: AD.UHier(U, 'r', nom)}
    return out


def hierarchies(P, K, mcs_range):
    """{regle: {mcs ou 0: UHier}} par l'oracle v11."""
    out = {r: {} for r in REGLES}
    a1, _res, _tree = AD.route_a(P, K, 1)
    ak, _res, _tree = AD.route_a(P, K, K + 1)
    out['core'][0] = AD.UHier(a1['core'][1], 'sq', 'core')
    out['cover'][0] = AD.UHier(a1['cover'][1], 'sq', 'cover')
    out['H1'][0] = AD.UHier(a1['margin1'][1], 'sq', 'H1')
    out['first_k1'][0] = AD.UHier(ak['first'][1], 'sq', 'first_k1')
    out['Hk1'][0] = AD.UHier(ak['margin'][1], 'sq', 'Hk1')
    par_m = {K + 1: out['Hk1'][0]}
    for mcs in mcs_range:
        m = max(K + 1, mcs)
        if m not in par_m:
            if m > len(P):
                par_m[m] = None
            else:
                am, _r, _t = AD.route_a(P, K, m)
                par_m[m] = AD.UHier(am['margin'][1], 'sq', 'Hmcs')
        out['Hmcs'][mcs] = par_m[m]
    return out


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('--nmax', type=int, default=16)
    ap.add_argument('--kmax', type=int, default=4)
    ap.add_argument('--valider-er0h', type=int, default=0)
    ap.add_argument('--route-b', action='store_true')
    ap.add_argument('--out', required=True)
    a = ap.parse_args(argv)
    with open(CL.V2, 'rb') as f:
        brut = f.read()
    v2 = json.loads(brut)
    recu = {x['name']: x for x in json.load(open(RECU_ER0H))['entrees']}
    out = {'cadre': 'phase=exploration_v11_hors_registre backend=cpu_reference profile=quantized_u18_input_only '
                    'public_status=not_claimed ; GCP non utilise',
           'sha_v2': hashlib.sha256(brut).hexdigest(), 'nmax': a.nmax, 'kmax': a.kmax, 'entrees': [],
           'sautees': {'n_trop_grand': 0, 'K_trop_grand': 0, 'm_superieur_a_n': 0}, 'validation_er0h': []}
    t0 = time.time()
    nval = 0
    regles = REGLES + (REGLES_B if a.route_b else ())
    for e in v2['fixtures']:
        if e['famille'] in USER:
            continue
        n, K = len(e['points']), e['K']
        if n > a.nmax:
            out['sautees']['n_trop_grand'] += 1
            continue
        if K > a.kmax:
            out['sautees']['K_trop_grand'] += 1
            continue
        t1 = time.time()
        lo, hi = e['mcs']
        mcs_range = list(range(lo, hi + 1))
        vars_ = [('base', e['points'], e['target'])] + [(v['name'], v['points'], v['target'])
                                                       for v in e.get('variants', [])]
        res = {r: {'jugements': 0, 'passes': 0, 'premier_viol': None, 'retard': 0, 'structure': 0}
               for r in regles}
        val = None
        if nval < a.valider_er0h and e['name'] in recu:
            val = {'jugements': 0, 'passes': 0}
        for vn, pts, cibles in vars_:
            noms = list(pts)
            P = [tuple(pts[x]) for x in noms]
            index = {x: i for i, x in enumerate(noms)}
            hs = hierarchies(P, K, mcs_range)
            if a.route_b:
                hs.update(hierarchies_b(P, K))
            for r in regles:
                for mcs in mcs_range:
                    h = hs[r].get(mcs, hs[r].get(0))
                    for ic, c in enumerate(cibles):
                        if h is None:
                            continue
                        ok, viol = ver.juger(h, c, index, mcs)
                        res[r]['jugements'] += 1
                        res[r]['passes'] += 1 if ok else 0
                        if not ok:
                            rr, part, why = viol
                            if 'alternative' in why:
                                genre = 'alternative'
                            elif 'reunis' in why or 'hors bloc' in why or 'plusieurs' in why:
                                genre = 'structure'
                            else:
                                genre = 'retard'
                            res[r][genre] = res[r].get(genre, 0) + 1
                            if res[r]['premier_viol'] is None:
                                cl, _b = ver.condenser(part, mcs)
                                res[r]['premier_viol'] = {'variante': vn, 'mcs': mcs, 'cible': ic,
                                                          'rayon': round(float(rr), 3), 'raison': why,
                                                          'clusters': [sorted(noms[y] for y in b) for b in cl]}
            if val is not None:
                T, info = CL.full(P, K)
                h10, _d = CL.construire('ER0h', T, info, {'lam': None, 'eta': CL.Fraction(1),
                                                          'kappa': CL.Fraction(12)}, lo)
                hu = AD.UHier(AD.u_de_hier(h10), 'r', 'ER0h_U')
                for mcs in mcs_range:
                    for c in cibles:
                        ok, _v = ver.juger(hu, c, index, mcs)
                        val['jugements'] += 1
                        val['passes'] += 1 if ok else 0
        if val is not None:
            nval += 1
            ref = recu[e['name']]
            val.update({'name': e['name'], 'recu_passes': ref['passes'], 'recu_jugements': ref['jugements'],
                        'accord': (val['passes'], val['jugements']) == (ref['passes'], ref['jugements'])})
            out['validation_er0h'].append(val)
        ligne = {'name': e['name'], 'code': e.get('code'), 'famille': e['famille'], 'statut': e['statut_cible'],
                 'K': K, 'n': n, 'mcs': [lo, hi], 'variantes': len(vars_), 'resultats': res,
                 'er0h_recu': ({'passes': recu[e['name']]['passes'], 'jugements': recu[e['name']]['jugements']}
                               if e['name'] in recu else None),
                 'secondes': round(time.time() - t1, 2)}
        out['entrees'].append(ligne)
        print('%-44s K%d n%-2d %s %s (%.1fs)' % (e['name'], K, n, ' '.join(
            '%s=%d/%d' % (r, res[r]['passes'], res[r]['jugements']) for r in regles),
            '' if ligne['er0h_recu'] is None else 'ER0h=%d/%d' % (ligne['er0h_recu']['passes'],
                                                                  ligne['er0h_recu']['jugements']),
            time.time() - t1), flush=True)
    bilan = {}
    for r in regles + ('ER0h_recu',):
        ent = pas = jug = epas = 0
        for x in out['entrees']:
            if r == 'ER0h_recu':
                if x['er0h_recu'] is None:
                    continue
                p, j = x['er0h_recu']['passes'], x['er0h_recu']['jugements']
            else:
                p, j = x['resultats'][r]['passes'], x['resultats'][r]['jugements']
            ent += 1
            jug += j
            pas += p
            epas += 1 if p == j else 0
        bilan[r] = {'entrees': ent, 'entrees_passees': epas, 'jugements': jug, 'jugements_passes': pas}
    out['bilan'] = bilan
    out['secondes'] = round(time.time() - t0, 1)
    sem = json.dumps([(x['name'], {r: (x['resultats'][r]['passes'], x['resultats'][r]['jugements'])
                                   for r in regles}) for x in out['entrees']], sort_keys=True)
    out['empreinte'] = hashlib.sha256(sem.encode()).hexdigest()
    with open(a.out, 'w') as f:
        json.dump(out, f, indent=1, sort_keys=True, default=str)
        f.write('\n')
    print(json.dumps({'bilan': bilan, 'sautees': out['sautees'], 'validation_er0h': [
        (v['name'], v['passes'], v['jugements'], v['recu_passes'], v['recu_jugements'], v['accord'])
        for v in out['validation_er0h']], 'secondes': out['secondes'], 'empreinte': out['empreinte'][:16]},
        indent=0))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
