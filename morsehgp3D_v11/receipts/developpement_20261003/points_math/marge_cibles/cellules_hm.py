#!/usr/bin/env python3
"""Cellules de l'utilisateur (juge v10 inchange) pour les pendaisons v11 : H_m (m = 1, k + 1, max(k + 1, mcs)),
temoins core / cover / first, variantes « marge en rayon », et ER0h(1, 12) relue par l'adaptateur (validation).

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
public_status=not_claimed. GCP non utilise. Lecture seule hors de ce dossier ; aucun bytecode.
Usage : python3 -B cellules_hm.py --out recus/cellules_hm.json
"""
import argparse
import hashlib
import json
import sys
import time

sys.dont_write_bytecode = True
import adaptateur as AD  # noqa: E402

CL, ver, R = AD.CL, AD.ver, AD.R
_full_orig = CL.full
_construire_orig = CL.construire

REGLES = {
    # nom : (route, regle de la route, m)  m : 'un' | 'k1' | 'mcs'
    'ER0h_U': ('v10', 'ER0h', None),            # validation de l'adaptateur (ER0h de la v10, relue par UHier)
    'core': ('A', 'core', 'un'),
    'cover': ('A', 'cover', 'un'),               # premiere couverture, LCA des ex aequo (P2 / P4)
    'first_k1': ('A', 'first', 'k1'),            # premiere couverture qualifiee m = k + 1, LCA des ex aequo
    'H1': ('A', 'margin1', 'un'),                # H_m, m = 1 (niveau carre)
    'Hk1': ('A', 'margin', 'k1'),                # H_m, m = k + 1 (niveau carre)
    'Hmcs': ('A', 'margin', 'mcs'),              # H_m, m = max(k + 1, mcs) (choix retenu du developpeur)
    'H1r': ('Br', None, 'un'),                   # variante : marge en rayon (route B), m = 1
    'Hk1r': ('Br', None, 'k1'),                  # variante : marge en rayon, m = k + 1
    'P2r': ('Br', 2, 'un'),                      # famille H4 en rayon, kappa = 2, m = 1 (= P_2 de la v10 ?)
    'P2sq': ('Bs', 2, 'un'),                     # famille H4 en niveau carre, kappa = 2, m = 1
    'Hk1_k2r': ('Br', 2, 'k1'),                  # famille H4 en rayon, kappa = 2, m = k + 1
}
DEPEND = {'Hmcs': True}

STATS = {'recoupes_AB': 0, 'desaccords_AB': [], 'secondes_route': {}}


def full_memo(P, K):
    T, info = _full_orig(P, K)
    info['_P'] = tuple(tuple(p) for p in P)
    return T, info


def m_de(code, K, mcs):
    if code == 'un':
        return 1
    if code == 'k1':
        return K + 1
    return max(K + 1, mcs)


def construire(regle, T, info, prm, mcs):
    if regle not in REGLES:
        return _construire_orig(regle, T, info, prm, mcs)
    route, nom, mcode = REGLES[regle]
    t0 = time.time()
    if route == 'v10':
        h, det = _construire_orig(nom, T, info, prm, mcs)
        out = AD.UHier(AD.u_de_hier(h), 'r', regle), det
    else:
        P, K = list(info['_P']), info['K']
        m = m_de(mcode, K, mcs)
        if route == 'A':
            rules, _res, _tree = AD.route_a(P, K, m)
            ent, U = rules[nom]
            if nom in ('margin', 'margin1'):
                # recoupe exacte par la route B (H_m reecrite sur l'arbre Gamma_K de la v10)
                _entb, Ub = AD.route_b(T, m, 'sq')
                STATS['recoupes_AB'] += 1
                if not AD.egal_u(U, 'sq', Ub, 'sq'):
                    STATS['desaccords_AB'].append({'regle': regle, 'm': m, 'P': P, 'K': K})
            out = AD.UHier(U, 'sq', regle), {'entrees': [(str(e), o) for e, o in ent], 'm': m}
        elif route == 'Bs':
            ent, U = AD.route_b(T, m, 'sq', CL.Fraction(nom or 1))
            out = AD.UHier(U, 'sq', regle), {'entrees': [(str(e), o) for e, o in ent], 'm': m}
        else:
            ent, U = AD.route_b(T, m, 'r', CL.Fraction(nom or 1))
            out = AD.UHier(U, 'r', regle), {'entrees': [(e.texte(), o) for e, o in ent], 'm': m}
    STATS['secondes_route'][regle] = STATS['secondes_route'].get(regle, 0.0) + time.time() - t0
    return out


CL.full = full_memo
CL.construire = construire
for _r in REGLES:
    CL.DEPEND_MCS[_r] = DEPEND.get(_r, False)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--regles', default=','.join(REGLES))
    a = ap.parse_args(argv)
    with open(CL.V2, 'rb') as f:
        brut = f.read()
    v2 = json.loads(brut)
    prm = {'lam': None, 'eta': CL.Fraction(1), 'kappa': CL.Fraction(12)}  # parametres de ER0h_U seulement
    regles = [r for r in a.regles.split(',') if r]
    out = {'cadre': 'phase=exploration_v11_hors_registre backend=cpu_reference profile=quantized_u18_input_only '
                    'public_status=not_claimed ; GCP non utilise',
           'sha_v2': hashlib.sha256(brut).hexdigest(), 'regles': regles, 'resultats': {}}
    t0 = time.time()
    for regle in regles:
        bilan = {}
        cels = []
        controles = {}
        for cel in CL.cellules(v2):
            r = CL.juger_cellule(cel, regle, prm, controles)
            st = cel['statut']
            b = bilan.setdefault(st, [0, 0, 0, 0])
            if r['jugements']:
                b[0] += 1
                b[1] += 1 if r['passe'] else 0
                b[2] += r['jugements']
                b[3] += r['passes']
            cels.append({'nom': cel['nom'], 'statut': st, 'mcs': cel['mcs'], 'passes': r['passes'],
                         'jugements': r['jugements'], 'passe': r['passe'], 'premier_viol': r['premier_viol'],
                         'rapport': r['rapport'], 'dates': r['dates']})
        anc = [bilan.get('ancree', [0, 0, 0, 0]), bilan.get('ancree_1oct', [0, 0, 0, 0])]
        tot = (anc[0][3] + anc[1][3], anc[0][2] + anc[1][2])
        sem = json.dumps([(c['nom'], c['passes'], c['jugements'], c['rapport']) for c in cels], sort_keys=True,
                         default=str)
        out['resultats'][regle] = {'bilan': bilan, 'ancrees': '%d/%d' % tot, 'cellules': cels,
                                   'controles': controles,
                                   'empreinte': hashlib.sha256(sem.encode()).hexdigest()}
        print('%-8s ancrees %s  bilan %s  empreinte %s' % (regle, '%d/%d' % tot, json.dumps(bilan),
                                                          out['resultats'][regle]['empreinte'][:16]), flush=True)
        for c in cels:
            if c['jugements'] and not c['passe'] and c['statut'] in ('ancree', 'ancree_1oct'):
                print('    X %-24s %d/%d %s' % (c['nom'], c['passes'], c['jugements'],
                                               json.dumps(c['premier_viol'], default=str)[:230]), flush=True)
    out['stats'] = STATS
    out['secondes'] = round(time.time() - t0, 1)
    with open(a.out, 'w') as f:
        json.dump(out, f, indent=1, sort_keys=True, default=str)
        f.write('\n')
    print(json.dumps({'recoupes_AB': STATS['recoupes_AB'], 'desaccords_AB': len(STATS['desaccords_AB']),
                      'secondes': out['secondes'],
                      'secondes_route': {k: round(v, 1) for k, v in STATS['secondes_route'].items()}}))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
