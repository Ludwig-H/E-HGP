#!/usr/bin/env python3
"""Validation de la bibliotheque des fixtures cibles contre les comportements DOCUMENTES (recus, memos).

Usage : python3 -B valide_lib.py [--out RECU.json]     (et python3 -B -O : semantique identique exigee)

Controles (lecture seule de toutes les sources citees) :
  V1 deux triangles (these 6.1), deux variantes : table documentee de
     audits/REPONSE_CLAUDE_AUDIT_GEANT_20260930.md section 3 (blocs entre 1,3 r et 1,7 r par regle), faits FULL ;
  V2 F1 {0,2,4} K2 : point du milieu reporte a la fusion (recu masses_selection/out/fixtures_gamma.json,
     F1_middle_point) ; couvertures {0,2} et {2,4} a r = 1, core vide, cover A1 = {0,2}|{4} ou {0}|{2,4}
     (CONTRE_AUDIT_FIXTURES_PROJECTION_20260929 section 2) ; P_kappa et bandes reportent (memo d'ancrage, section 8) ;
  V3 huit fixtures 0,1,L,L+1 : recu majority_boundary_geometry (blocs exacts aux coupes locale et core,
     groupes recuperes avant la premiere fusion, uniforme contre 1/beta) ; recu masses (paires avant la premiere
     fusion par regle) ; memo d'ancrage (A1, A5, P_2, bande_f forment les paires a alpha) ;
  V4 cinq sites : recu tower_math (W fixes, premier proprietaire de x0 a beta = 2/3 couvrant {0,2,3,4}, premiere
     reunion {0,1} a 5/4 pour les majorites, 1/4 pour l'ancrage unique) ; recu masses (rayon de x0 par regle,
     paire {0,1} avant sa fusion, proprietaire dans la lignee) ;
  V5 contact coquille/interieur M = 10, 100, 1024, 2048 : recu inverse_beta_shell (premiere reunion C/A exacte en
     1/beta avant et apres deplacement) ; recu masses (hauteurs rho(C, A) par regle) ;
  V6 recoupe complete du recu d'ancrage (recus/fixtures_normal.json) : dates et hauteurs des points d'interet pour
     core, cover_A1, A5, P_1, P_2, P_4, bande_f[1/4], bande_t[1/4], sur toutes les fixtures K >= 2 et variantes ;
  V7 semantique de conformite des cibles (tests unitaires, tolerance et points libres) et mutant tolerance_laxiste ;
  V8 mutants causaux : majorite non stricte, denominateur non fige, coupe ouverte, bande sans LCA ; chacun doit
     contredire un comportement documente (portes de non-vacuite) ;
  V9 compatibilite des cibles avec FULL (amas discrets distincts) : un cas compatible, deux incompatibles.
Codes : 0 tout est reproduit ; 1 un comportement documente n'est pas reproduit ; 3 invariant viole.
"""
import argparse
import ast
from fractions import Fraction
import hashlib
import json
import os
import sys
import time

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
if ICI not in sys.path:
    sys.path.insert(0, ICI)
import regles as RG  # noqa: E402
import run_target as RT  # noqa: E402
from regles import Rayon, Incoherence  # noqa: E402

WT = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10'
RECU_MASSES = '/workspaces/E-HGP/build/v10-verrou-points/masses_selection/out/fixtures_gamma.json'
RECU_ANCRAGE = '/workspaces/E-HGP/build/v10-verrou-points/ancrage_marges/recus/fixtures_normal.json'
RECU_TOWER = WT + '/receipts/audit_independant_20260930/tower_math/normal.stdout'
RECU_GEOM = WT + '/receipts/audit_continu_20260929/majority_boundary_geometry/normal/receipt.json'
RECU_SHELL = WT + '/receipts/audit_continu_20260929/inverse_beta_shell/normal.json'
REPONSE = WT + '/audits/REPONSE_CLAUDE_AUDIT_GEANT_20260930.md'
EXEMPLE = os.path.join(ICI, 'exemples', 'deux_triangles_K2.json')

# regles de la bibliotheque -> noms des recus
MASSES_NOM = {'core': 'core', 'cover_A1': 'cover_A1', 'A5_U1': 'band_eta0_A5', 'bande_t[1/8]': 'band_eta1/8',
              'maj_unif': 'cat_step_uniform_A3', 'maj_inv_beta': 'cat_step_invbeta_A4',
              'maj_prog[z=2]': 'branch_gradual_z2', 'maj_prog[z=3]': 'branch_gradual_z3'}
ANCRAGE_NOM = {'core': 'core', 'cover_A1': 'cover_A1', 'A5_U1': 'A5', 'P_1': 'P_1', 'P_2': 'P_2', 'P_4': 'P_4',
               'bande_f[1/4]': 'bande_f[1/4]', 'bande_t[1/4]': 'bande_t[1/4]'}
# table documentee (REPONSE_CLAUDE_AUDIT_GEANT_20260930.md, section 3) : blocs entre 1,3 r et 1,7 r
TABLE_TRIANGLES = {
    'core': ('(singletons)', '(singletons)'),
    'cover_A1': ('ABC | DEF', 'AB | CD | EF'), 'A5_U1': ('ABC | DEF', 'AB | CD | EF'),
    'P_1': ('AB | EF', 'AB | EF'), 'P_2': ('AB | EF', 'AB | EF'), 'P_4': ('AB | EF', 'AB | EF'),
    'bande_f[1/8]': ('AB | EF', 'AB | EF'), 'bande_f[1/4]': ('AB | EF', 'AB | EF'),
    'bande_t[1/8]': ('AB | EF', 'AB | EF'), 'bande_t[1/4]': ('AB | EF', 'AB | EF'),
    'maj_unif': ('ABC | DEF', 'ABC | DEF'), 'maj_inv_beta': ('ABC | DEF', 'ABC | DEF'),
}
for _e in ('1/8', '1/4', '1/2'):
    TABLE_TRIANGLES['maj_bande_unif[%s]' % _e] = ('ABC | DEF', 'ABC | DEF')
    TABLE_TRIANGLES['maj_bande_inv[%s]' % _e] = ('ABC | DEF', 'ABC | DEF')


class Juge:
    def __init__(self):
        self.lignes = []

    def check(self, groupe, nom, ok, attendu, observe, source):
        self.lignes.append({'groupe': groupe, 'controle': nom, 'ok': bool(ok), 'attendu': attendu,
                            'observe': observe, 'source': source})
        return ok

    def proche(self, groupe, nom, a, b, source, rel=1e-12):
        ok = a is not None and b is not None and abs(a - b) <= rel * max(1.0, abs(a), abs(b))
        return self.check(groupe, nom, ok, b, a, source)


def ids(P):
    return [('p%d' % i, tuple(p)) for i, p in enumerate(P)]


def blocs_sets(part):
    return sorted(sorted(b) for b in part)


# ------------------------------------------------------------------ V1 deux triangles

def v1(J, cnt):
    with open(EXEMPLE) as f:
        brut = json.load(f)
    fx = RT.normaliser(brut, EXEMPLE)
    res = RT.juger_fixture(fx, RG.NOMS_REGLES, 'auto', RG.GAMMA_BUDGET, cnt)
    src = 'REPONSE_CLAUDE_AUDIT_GEANT_20260930.md section 3'
    for k, v in enumerate(res['variantes']):
        J.check('V1', '%s : oracle Gamma' % v['nom'], v['gamma'].get('statut') == 'verifie', 'verifie',
                v['gamma'].get('statut'), 'hgp10_ref.gamma_cuts')
        J.check('V1', '%s : cible compatible avec FULL' % v['nom'], v['compat_full'][0]['compatible'], True,
                v['compat_full'][0], 'amas discrets (definition 8)')
        for r, (e1, e2) in TABLE_TRIANGLES.items():
            obs = [o['texte'] for o in v['regles'][r]['cibles'][0]['observes']]
            want = (e1, e2)[k]
            J.check('V1', '%s : %s entre 1,3 r et 1,7 r' % (v['nom'], r), obs == [want], want, obs, src)
        for r in ('hdbscan_these', 'hdbscan_eps_r', 'maj_prog[z=2]', 'maj_prog[z=3]'):
            obs = [o['texte'] for o in v['regles'][r]['cibles'][0]['observes']]
            J.check('V1', '%s : %s (non documente, rapporte)' % (v['nom'], r), True, None, obs, 'information')
        fus = v['full']['fusions_r']
        J.check('V1', '%s : triangles formes vers 1,155 r, fusion globale vers 1,932 r' % v['nom'],
                len(fus) == 2 and abs(fus[0] / 1000 - 1.155) < 0.002 and abs(fus[1] / 1000 - 1.932) < 0.002,
                [1155, 1932], fus, src)
    # amas discrets a 1,5 r : trois composantes (triangle ABC, pont CD, triangle DEF)
    sc = RG.Scene(fx['variantes'][0]['points'], 2, nom='deux_triangles/base')
    amas = sorted(''.join(a['points']) for a in sc.amas_discrets(r=1500))
    J.check('V1', 'base : trois amas discrets ABC, CD, DEF a 1,5 r', amas == ['ABC', 'CD', 'DEF'],
            ['ABC', 'CD', 'DEF'], amas, 'these 6.1, FULL_2')
    return res


# ------------------------------------------------------------------ V2 F1

def v2(J, masses):
    P = [(0, 0, 0), (2, 0, 0), (4, 0, 0)]
    sc = RG.Scene(ids(P), 2, nom='F1')
    root = sc.foret.root
    b_root = sc.niveau(root)
    src_m = 'masses_selection/out/fixtures_gamma.json : F1_middle_point'
    for r, mnom in MASSES_NOM.items():
        h = sc.regle(r)
        deferred = h.proprietaires[1] == root and h.dates[1].b2 == b_root
        attendu = masses['F1_middle_point'][mnom]['middle_deferred_to_merge']
        J.check('V2', 'F1 : %s reporte le milieu a la fusion' % r, deferred == attendu, attendu, deferred, src_m)
    for r in ('P_1', 'P_2', 'P_4', 'bande_f[1/8]', 'bande_f[1/4]', 'bande_t[1/4]'):
        h = sc.regle(r)
        deferred = h.proprietaires[1] == root and h.dates[1].b2 == b_root
        J.check('V2', 'F1 : %s reporte le milieu (LCA des ex aequo)' % r, deferred, True, deferred,
                'MEMO_ANCRAGE_MARGES section 8 (plateaux), mutant departage tue par F1')
    src_a = 'CONTRE_AUDIT_FIXTURES_PROJECTION_20260929 section 2'
    amas = sorted(a['points'] for a in sc.amas_discrets(r=1))
    J.check('V2', 'F1 : couvertures {0,2} et {2,4} a r = 1', amas == [['p0', 'p1'], ['p1', 'p2']],
            [['p0', 'p1'], ['p1', 'p2']], amas, src_a)
    core = sc.regle('core').blocs(r=1)
    J.check('V2', 'F1 : core vide a r = 1', all(len(b) == 1 for b in core), '(singletons)', core, src_a)
    cov = blocs_sets(sc.regle('cover_A1').blocs(r=1))
    J.check('V2', 'F1 : cover A1 immediat a r = 1', cov in ([['p0'], ['p1', 'p2']], [['p0', 'p1'], ['p2']]),
            "{0,2}|{4} ou {0}|{2,4}", cov, src_a)


# ------------------------------------------------------------------ V3 huit fixtures 0, 1, L, L+1

def v3(J, masses, geom):
    src_g = 'majority_boundary_geometry/normal/receipt.json'
    src_m = 'masses_selection/out/fixtures_gamma.json : two_pairs'
    for L in (4, 8, 32, 100):
        for fam, P in (('line', [(0, 0, 0), (1, 0, 0), (L, 0, 0), (L + 1, 0, 0)]),
                       ('tetra', [(0, 0, 0), (1, 0, 0), (L, 1, 1), (L + 1, 1, 2)])):
            nom = '%s%d' % (fam, L)
            sc = RG.Scene(ids(P), 2, nom=nom)
            row = geom['rows'][nom]
            J.check('V3', '%s : memes points que le recu' % nom, [tuple(p) for p in row['points']] == P, P,
                    row['points'], src_g)
            first = min(sc.fusions_full())
            J.check('V3', '%s : premiere fusion FULL' % nom, str(first) == row['first_FULL_fusion_beta'],
                    row['first_FULL_fusion_beta'], str(first), src_g)
            lb, cb = Fraction(row['local_beta']), Fraction(row['core_beta'])
            for r, mode in (('maj_unif', 'uniform'), ('maj_inv_beta', 'inverse_beta')):
                h = sc.regle(r)
                m = row['modes'][mode]
                a = blocs_sets(h.blocs_ids(Rayon.niveau(lb)))
                b = blocs_sets(h.blocs_ids(Rayon.niveau(cb)))
                J.check('V3', '%s : %s blocs a la coupe locale' % (nom, r), a == m['blocks_at_local_beta'],
                        m['blocks_at_local_beta'], a, src_g)
                J.check('V3', '%s : %s blocs a la coupe core' % (nom, r), b == m['blocks_at_core_beta'],
                        m['blocks_at_core_beta'], b, src_g)
                rec = 0
                for g in row['expected_groups']:
                    if any(tuple(g) in part for rr, part in h.evenements_ids() if rr.b2 < first):
                        rec += 1
                J.check('V3', '%s : %s groupes recuperes avant la premiere fusion' % (nom, r),
                        rec == m['recovered_groups_before_first_FULL_fusion'],
                        m['recovered_groups_before_first_FULL_fusion'], rec, src_g)
            mr = masses['two_pairs']['pairs_%s' % nom]
            J.check('V3', '%s : premiere fusion (recu masses)' % nom, str(first) == mr['first_fusion'],
                    mr['first_fusion'], str(first), src_m)
            for r, mnom in MASSES_NOM.items():
                part = sc.regle(r).blocs_ids(Rayon(b2=first), ferme=False)
                obs = [(0, 1) in part, (2, 3) in part]
                rr = mr['rules'][mnom]
                want = [rr['pair01_before_first_fusion'], rr['pair23_before_first_fusion']]
                J.check('V3', '%s : %s paires juste avant la premiere fusion' % (nom, r), obs == want, want, obs, src_m)
            for r in ('cover_A1', 'A5_U1', 'P_2', 'P_4', 'bande_f[1/8]', 'bande_f[1/4]'):
                part = blocs_sets(sc.regle(r).blocs_ids(Rayon.niveau(lb)))
                J.check('V3', '%s : %s forme les deux paires a alpha' % (nom, r), part == [[0, 1], [2, 3]],
                        [[0, 1], [2, 3]], part, 'MEMO_ANCRAGE_MARGES section 10 (portes G1/G2 ; P_4 par T3 d)')


# ------------------------------------------------------------------ V4 cinq sites

def lire_tower():
    out = {}
    with open(RECU_TOWER) as f:
        for line in f:
            for key in ('fixed W ', 'first_owner ', 'first_join_0_1 '):
                if line.startswith(key):
                    out[key.strip()] = ast.literal_eval(line[len(key):].strip())
    return out


def v4(J, masses):
    P = [(1, 1, 0), (2, 1, 0), (0, 2, 0), (0, 0, 0), (0, 1, 1)]
    sc = RG.Scene(ids(P), 2, nom='cinq_sites')
    tw = lire_tower()
    src_t = 'audit_independant_20260930/tower_math/normal.stdout'
    for r, mode in (('maj_unif', 'uniform'), ('maj_inv_beta', 'inverse_beta')):
        h = sc.regle(r)
        W = [h.info['W']['p%d' % i] for i in range(5)]
        J.check('V4', 'cinq sites : %s denominateurs fixes' % r, W == tw['fixed W'][mode], tw['fixed W'][mode], W,
                src_t)
        beta, cov = tw['first_owner'][mode][0]
        date = h.dates[0].b2
        amas = sorted(sc.couvertures(date).get(sc.foret.ancestor(h.proprietaires[0], date, True), ()))
        J.check('V4', 'cinq sites : %s premier proprietaire de x0' % r,
                str(date) == beta and tuple(amas) == tuple(cov), [beta, list(cov)], [str(date), amas], src_t)
        hh = h.hauteur(0, 1)
        J.check('V4', 'cinq sites : %s premiere reunion {0,1}' % r, str(hh.b2) == tw['first_join_0_1'][mode],
                tw['first_join_0_1'][mode], str(hh.b2), src_t)
    for r, mode in (('A5_U1', 'k2_lca_eta0'), ('cover_A1', 'hybrid_unique_first')):
        hh = sc.regle(r).hauteur(0, 1)
        J.check('V4', 'cinq sites : %s premiere reunion {0,1} (%s)' % (r, mode),
                str(hh.b2) == tw['first_join_0_1'][mode], tw['first_join_0_1'][mode], str(hh.b2), src_t)
    src_m = 'masses_selection/out/fixtures_gamma.json : five_sites'
    pair = next(v for v in range(len(sc.foret)) if sc.niveau(v) == Fraction(1, 4) and sc.foret.birth[v] != RG.NONE)
    death = sc.niveau(sc.foret.parent[pair])
    for r, mnom in MASSES_NOM.items():
        h = sc.regle(r)
        m = masses['five_sites'][mnom]
        J.proche('V4', 'cinq sites : %s rayon de x0' % r, float(h.dates[0]), m['x0_radius'], src_m)
        part = h.blocs_ids(Rayon(b2=death), ferme=False)
        J.check('V4', 'cinq sites : %s paire {0,1} avant sa fusion' % r, ((0, 1) in part) ==
                m['pair01_block_before_its_fusion'], m['pair01_block_before_its_fusion'], (0, 1) in part, src_m)
        lin = sc.foret.is_ancestor(h.proprietaires[0], pair)
        J.check('V4', 'cinq sites : %s proprietaire de x0 dans la lignee de la paire' % r,
                lin == m['x0_owner_in_pair_lineage'], m['x0_owner_in_pair_lineage'], lin, src_m)
    J.check('V4', 'cinq sites : P_2 attache x0 a 0,704', abs(float(sc.regle('P_2').dates[0]) - 0.704) < 5e-4,
            0.704, float(sc.regle('P_2').dates[0]), 'MEMO_ANCRAGE_MARGES section 10')


# ------------------------------------------------------------------ V5 contact coquille / interieur

def v5(J, masses, shell):
    src_s = 'inverse_beta_shell/normal.json'
    src_m = 'masses_selection/out/fixtures_gamma.json : shell_interior_contact'
    rows = {r['M']: r for r in shell['rows']}
    for M in (10, 100, 1024, 2048):
        base = [(0, 0, 0), (4 * M, 0, 0), (0, 5 * M, 0), (0, 0, 100 * M)]
        moved = [(1, 1, 1)] + base[1:]
        scs = {'original': RG.Scene(ids(base), 2, nom='contact_M%d' % M),
               'perturbed': RG.Scene(ids(moved), 2, nom='contact_M%d_C111' % M)}
        for key, sc in scs.items():
            hh = sc.regle('maj_inv_beta').hauteur(0, 1)
            want = rows[M][key]['CA_first_beta']
            J.check('V5', 'contact M=%d %s : maj_inv_beta premiere reunion C/A' % (M, key), str(hh.b2) == want,
                    want, str(hh.b2), src_s)
        want_o, want_p = Fraction(41 * M * M, 4), Fraction((4 * M - 1) ** 2 + 2, 4)
        J.check('V5', 'contact M=%d : formules 41 M^2/4 et ((4M-1)^2+2)/4' % M,
                scs['original'].regle('maj_inv_beta').hauteur(0, 1).b2 == want_o and
                scs['perturbed'].regle('maj_inv_beta').hauteur(0, 1).b2 == want_p,
                [str(want_o), str(want_p)], None, src_s)
        mrow = masses['shell_interior_contact']['M%d' % M]
        for r, mnom in MASSES_NOM.items():
            ub = float(scs['original'].regle(r).hauteur(0, 1))
            um = float(scs['perturbed'].regle(r).hauteur(0, 1))
            J.proche('V5', 'contact M=%d : %s rho(C,A) base' % (M, r), ub, mrow[mnom]['rho_CA_base'], src_m)
            J.proche('V5', 'contact M=%d : %s rho(C,A) deplace' % (M, r), um, mrow[mnom]['rho_CA_moved'], src_m)
        if M == 2048:
            for key, sc in scs.items():
                d = float(sc.regle('P_2').dates[0])
                J.check('V5', 'contact M=2048 %s : P_2 date de C = 4508,799' % key, abs(d - 4508.799) < 5e-4,
                        4508.799, d, 'MEMO_ANCRAGE_MARGES section 10')


# ------------------------------------------------------------------ V6 recu d'ancrage complet

def v6(J, anc):
    sys.path.insert(0, RG.ANCRAGE)
    import fixtures as fxa  # module fixtures de l'approche d'ancrage (lecture seule)
    src = 'ancrage_marges/recus/fixtures_normal.json'
    n = 0
    for F in fxa.fixtures():
        if F['K'] < 2:
            continue
        entry = next(e for e in anc['fixtures'] if e['name'] == F['name'])
        for vname, pts in [('base', F['base'])] + list(F['variants']):
            sc = RG.Scene(ids(pts), F['K'], nom='%s/%s' % (F['name'], vname), gamma='non')
            rows = entry['clouds'][vname]['points']
            for r, anom in ANCRAGE_NOM.items():
                h = sc.regle(r)
                for p in F['focus']:
                    want = rows[anom][str(p)]['t_float']
                    ok = abs(float(h.dates[p]) - want) <= 1e-12 * max(1.0, abs(want))
                    for q in range(sc.n):
                        if q == p:
                            continue
                        hw = rows[anom]['hauteurs']['%d-%d' % (p, q)]
                        ok = ok and abs(float(h.hauteur(p, q)) - hw) <= 1e-12 * max(1.0, abs(hw))
                    if not ok:
                        J.check('V6', '%s/%s : %s point %d' % (F['name'], vname, r, p), False, want,
                                float(h.dates[p]), src)
                    n += 1
    J.check('V6', 'recu d ancrage : dates et hauteurs des points d interet (%d cas)' % n,
            all(ln['ok'] for ln in J.lignes if ln['groupe'] == 'V6'), True, n, src)


# ------------------------------------------------------------------ V7 semantique de conformite (tests unitaires)

class _Stub:
    def __init__(self, noms):
        self.noms = list(noms)
        self.index = {x: i for i, x in enumerate(self.noms)}


def _part(sc, texte):
    """'AB|CD' -> partition d'identifiants (les points absents sont des singletons)."""
    vus = set()
    blocs = []
    for b in texte.split('|'):
        b = b.strip()
        if b:
            blocs.append(tuple(sorted(sc.index[x] for x in b)))
            vus |= set(b)
    blocs += [(sc.index[x],) for x in sc.noms if x not in vus]
    return tuple(sorted(blocs))


def _cible(blocs, tol=(), lib=()):
    return {'blocs': [list(b) for b in blocs], 'tolerance': list(tol), 'libres': list(lib)}


CAS_CONFORMITE = [
    # (points, blocs attendus, tolerance, libres, partition observee, verdict attendu)
    ('ABCDEF', ['ABC', 'DEF'], '', '', 'ABC|DEF', True),
    ('ABCDEF', ['ABC', 'DEF'], '', '', 'AB|EF', False),
    ('ABCDEF', ['ABC', 'DEF'], 'CD', '', 'AB|EF', True),
    ('ABCDEF', ['ABC', 'DEF'], 'CD', '', 'AB|CD|EF', False),
    ('ABCDEF', ['ABC', 'DEF'], 'CD', '', 'ABCDEF', False),
    ('ABCDEF', ['ABC', 'DEF'], 'F', '', 'ABC|DE', True),
    ('ABCDEF', ['ABC', 'DEF'], 'F', '', 'ABC|DF', False),
    ('ABCDEFG', ['ABC', 'DEF'], '', 'G', 'ABCG|DEF', True),
    ('ABCDEFG', ['ABC', 'DEF'], '', 'G', 'AG|BC|DEF', False),
    ('ABCDEFX', ['ABC', 'DEF'], '', '', 'ABCX|DEF', False),
    ('ABCD', ['ABCD'], 'CD', '', 'AB|CD', False),
    ('ABCD', ['ABCD'], 'CD', '', 'AB', True),
    ('ABCD', ['CD'], 'CD', '', 'CD', True),
    ('ABCD', [], '', '', '', True),
    ('ABCD', [], '', '', 'AB', False),
]


def conforme_laxiste(part, cible, sc):
    """MUTANT : les points toleres sont ignores partout (ils peuvent rejoindre n'importe quel bloc)."""
    tol = set(cible['tolerance'])
    c2 = dict(cible)
    c2['libres'] = sorted(set(cible['libres']) | tol, key=sc.noms.index)
    c2['tolerance'] = []
    return RT.conforme(part, c2, sc)


def v7(J):
    tue = False
    for pts, blocs, tol, lib, obs, attendu in CAS_CONFORMITE:
        sc = _Stub(pts)
        cible = _cible(blocs, tol, lib)
        ok, raison = RT.conforme(_part(sc, obs), cible, sc)
        J.check('V7', 'conformite %s / attendu %s tol=%s libres=%s' % (obs, '|'.join(blocs), tol, lib),
                ok == attendu, attendu, [ok, raison], 'README, semantique des cibles')
        okm, _r = conforme_laxiste(_part(sc, obs), cible, sc)
        if okm != attendu:
            tue = True
    J.check('V7', 'mutant tolerance_laxiste tue', tue, True, tue, 'porte de non-vacuite')


# ------------------------------------------------------------------ V8 mutants causaux des majorites et des coupes

def majorite_mutante(sc, poids, stricte=True, denominateur='fixe'):
    """Majorite par balayage de la definition, avec mutations : seuil 2m >= W (non strict), ou denominateur
    recalcule sur les seuls temoins actifs a chaque coupe. Rend {point: (date b2, proprietaire)}."""
    f = sc.foret
    out = {}
    for s in range(sc.ctx.n):
        p = sc.ctx.point_id[s]
        rows = sorted(sc._temoins_ponderes(s, poids, None), key=lambda r: r[0])
        W = sum(w for _l, _v, w in rows)
        lvls = sorted({Fraction(lw) for lw, _v, _w in rows} | {sc.niveau(v) for v in range(len(f))})
        for beta in lvls:
            t = f.threshold(beta, True)
            mass = {}
            Wb = 0
            for lvl, v, w in rows:
                if lvl <= beta:
                    a = f.ancestor_lv(v, t)
                    mass[a] = mass.get(a, 0) + w
                    Wb += w
            den = W if denominateur == 'fixe' else Wb
            win = sorted(a for a, m in mass.items() if (2 * m > den if stricte else 2 * m >= den))
            if win:
                out[p] = (beta, win[0])
                break
    return out


def v8(J):
    # M1 majorite non stricte : F1, le milieu est attache a r = 1 (egalite 1/1), au lieu d'etre reporte
    sc = RG.Scene(ids([(0, 0, 0), (2, 0, 0), (4, 0, 0)]), 2, nom='F1', gamma='non')
    ref = majorite_mutante(sc, 'unif')
    mut = majorite_mutante(sc, 'unif', stricte=False)
    h = sc.regle('maj_unif')
    J.check('V8', 'balayage de reference = regle maj_unif (F1)',
            all(ref[p] == (h.dates[p].b2, sc.foret.ancestor(h.proprietaires[p], h.dates[p].b2, True))
                for p in range(3)), True, None, 'definition')
    J.check('V8', 'mutant majorite_non_stricte tue (F1 : milieu reporte a la fusion)',
            mut[1][0] != sc.niveau(sc.foret.root), True, str(mut[1][0]), 'recu masses F1_middle_point')
    # M2 denominateur non fige : cinq sites, x0 attache des 1/4 a {0,1} au lieu de 2/3 dans {0,2,3,4}
    sc5 = RG.Scene(ids([(1, 1, 0), (2, 1, 0), (0, 2, 0), (0, 0, 0), (0, 1, 1)]), 2, nom='cinq_sites', gamma='non')
    mut = majorite_mutante(sc5, 'unif', denominateur='actif')
    J.check('V8', 'mutant denominateur_actif tue (cinq sites : premier proprietaire de x0 a 2/3)',
            str(mut[0][0]) != '2/3', True, str(mut[0][0]), 'recu tower_math')
    # M3 coupe ouverte : F1 a r = 1, cover A1 ne montre plus son attache immediate
    part = sc.regle('cover_A1').blocs_ids(Rayon.rationnel(1), ferme=False)
    J.check('V8', 'mutant coupe_ouverte tue (F1 : cover A1 immediat a r = 1)', all(len(b) == 1 for b in part),
            True, [list(b) for b in part], 'CONTRE_AUDIT_FIXTURES_PROJECTION section 2')
    # M4 bande sans LCA (proprietaire = premiere feuille, date alpha) = cover : deux triangles, pont plus court
    with open(EXEMPLE) as f:
        fx = RT.normaliser(json.load(f), EXEMPLE)
    sct = RG.Scene(fx['variantes'][1]['points'], 2, nom='deux_triangles/pont_plus_court', gamma='non')
    cov = RT.format_blocs(sct.regle('cover_A1').blocs(r=1500))
    bande = RT.format_blocs(sct.regle('bande_t[1/8]').blocs(r=1500))
    J.check('V8', 'mutant bande_sans_lca tue (deux triangles : la bande ne suit pas le pont)', cov != bande,
            True, [cov, bande], 'table documentee')


# ------------------------------------------------------------------ V9 compatibilite des cibles avec FULL

def v9(J):
    with open(EXEMPLE) as f:
        fx = RT.normaliser(json.load(f), EXEMPLE)
    sc = RG.Scene(fx['variantes'][0]['points'], 2, nom='deux_triangles/base', gamma='non')
    src = 'definition 8 et theoreme T2 (amas discrets distincts)'
    cas = [
        ({'r': [1300, 1700]}, True, 'triangles formes, fusion globale apres'),
        ({'r': [1000, 1100]}, False, 'avant 2r/sqrt(3) aucun amas ne contient tout ABC'),
        ({'r': [1950, 2000]}, False, 'apres la fusion globale les deux blocs sont dans une meme composante'),
    ]
    for iv, attendu, pourquoi in cas:
        cible = RT.lire_cible(dict(iv, blocks=[['A', 'B', 'C'], ['D', 'E', 'F']]), sc.noms, [], [])
        res = RT.compat_full(sc, cible)
        J.check('V9', 'deux triangles %s : compatible = %s (%s)' % (iv, attendu, pourquoi),
                res['compatible'] == attendu, attendu, res, src)
    tol = RT.lire_cible({'r': [1000, 1100], 'blocks': [['A', 'B', 'C'], ['D', 'E', 'F']], 'tolerance': ['A', 'B']},
                        sc.noms, [], [])
    res = RT.compat_full(sc, tol)
    J.check('V9', 'deux triangles [1000, 1100], A et B toleres : bloc {C} seul, E et F encore separes',
            not res['compatible'], False, res, src)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=None)
    a = ap.parse_args(argv)
    t0 = time.time()
    avant = RG.empreintes()
    recus = [RECU_MASSES, RECU_ANCRAGE, RECU_TOWER, RECU_GEOM, RECU_SHELL, REPONSE, EXEMPLE]
    h_recus = {p: RG.sha_fichier(p) for p in recus}
    with open(RECU_MASSES) as f:
        masses = json.load(f)['findings']
    with open(RECU_ANCRAGE) as f:
        anc = json.load(f)
    with open(RECU_GEOM) as f:
        geom = json.load(f)
    with open(RECU_SHELL) as f:
        shell = json.load(f)
    J = Juge()
    cnt = {'jugements': 0, 'compat': 0}
    code = 0
    try:
        v1(J, cnt)
        v2(J, masses)
        v3(J, masses, geom)
        v4(J, masses)
        v5(J, masses, shell)
        v6(J, anc)
        v7(J)
        v8(J)
        v9(J)
    except Incoherence as err:
        J.check('invariant', 'incoherence', False, None, str(err), 'regles.py')
        code = 3
    echecs = [ln for ln in J.lignes if not ln['ok']]
    if echecs and code == 0:
        code = 1
    apres = RG.empreintes()
    par_groupe = {}
    for ln in J.lignes:
        g = par_groupe.setdefault(ln['groupe'], [0, 0])
        g[0] += 1
        g[1] += 0 if ln['ok'] else 1
    recu = {'cadre': RG.CADRE, 'optimize': sys.flags.optimize, 'code': code,
            'controles': len(J.lignes), 'echecs': len(echecs), 'par_groupe': {k: {'controles': v[0], 'echecs': v[1]}
                                                                              for k, v in sorted(par_groupe.items())},
            'lignes': J.lignes, 'recus_lus': h_recus, 'sources_avant': avant, 'sources_apres': apres,
            'sources_stables': avant == apres, 'secondes': round(time.time() - t0, 1)}
    sem = json.dumps(J.lignes, sort_keys=True, default=str)
    recu['empreinte_semantique'] = hashlib.sha256(sem.encode()).hexdigest()
    out = a.out or os.path.join(RG.SCRATCH, 'validation_%s.json' % ('O' if sys.flags.optimize else 'normal'))
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, 'w') as f:
        json.dump(recu, f, indent=1, ensure_ascii=False, sort_keys=True, default=str)
        f.write('\n')
    for ln in echecs[:30]:
        print('ECHEC %s %s : attendu %s, observe %s (%s)' % (ln['groupe'], ln['controle'], ln['attendu'],
                                                             ln['observe'], ln['source']))
    print(json.dumps({'code': code, 'controles': len(J.lignes), 'echecs': len(echecs), 'par_groupe': recu['par_groupe'],
                      'empreinte_semantique': recu['empreinte_semantique'], 'secondes': recu['secondes'],
                      'recu': out}, ensure_ascii=False))
    return code


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
