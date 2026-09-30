#!/usr/bin/env python3
"""Petites scenes DEV (bhc : deux amas, halos, couloir, fond ; graines dev figees par la campagne frontiere).

Pour chaque scene et chaque K : export natif (arbre FULL + temoins du lemme de couverture), puis
  1. hierarchies dures : core, cover (A1), bande eta=1/8, unites de branche par marches (z=2) et progressives
     (z = 2, 3, 4) ;
  2. diagnostics avant la premiere fusion parasite r_sp (representants = plus petit D_K par classe, comme le panel
     'between' des fondations) : rappel par role (coeur/halo), masse differee, attaches croisees ;
  3. stabilite sous jitter apparie (delta1, delta4 des fondations) : ecarts de rayons de reunion sur un panel fixe de
     paires tire sur le nuage de base ; controle de la borne 2 eps de core (theoreme) ;
  4. selection sans hierarchie de points : condensation + EOM avec masses fractionnaires (marches, progressives),
     vote a l'antichaine ; comparee a EOM sur masses dures (core, cover, progressive) ; ARI exact contre les labels ;
     coherence (proposition S3) : tout point majoritaire sous un cluster choisi vote pour lui.
Decisions : Decimal a 110 chiffres, marge certifiee (Ambiguous sinon) ; comptes d'egalites douces publies.
Aucune graine test ; aucun moteur modifie ; GCP non utilise. Tient sous python3 -O.
"""
import argparse
import hashlib
import json
import math
import os
import random
import struct
import sys
import time
from fractions import Fraction as F

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, 'lib'))
from fullk import native_tree, FC, require, MathError  # noqa: E402
from scale import Phi, cmp, SOFT_TIES  # noqa: E402
import participation as PA  # noqa: E402
import selection as SE  # noqa: E402

FRONT = '/workspaces/E-HGP/build/v10-frontiere/work/bench/frontier'
EXE = '/workspaces/E-HGP/build/v10-frontiere/frontier-build/export_frontier'


def read_scene(sdir):
    with open(os.path.join(sdir, 'scene.json')) as f:
        meta = json.load(f)
    require(meta.get('split') == 'dev' and meta.get('kind') == 'dev', 'scene non dev refusee')
    raw = open(os.path.join(sdir, 'points.u32le'), 'rb').read()
    pts = [struct.unpack_from('<3I', raw, 12 * i) for i in range(len(raw) // 12)]
    lab = list(struct.unpack('<%di' % len(pts), open(os.path.join(sdir, 'labels.i32le'), 'rb').read()))
    rol = list(open(os.path.join(sdir, 'roles.u8'), 'rb').read())
    return meta, pts, lab, rol


def read_points(path):
    raw = open(path, 'rb').read()
    return [struct.unpack_from('<3I', raw, 12 * i) for i in range(len(raw) // 12)]


def export(points, K, tag, tmp):
    fc = FC()
    cloud = os.path.join(tmp, tag + '.u32le')
    out = os.path.join(tmp, tag + '_k%d.json' % K)
    fc.write_cloud(points, cloud)
    code, summ, so, se, argv = fc.run_exporter(EXE, cloud, out, K, threads=1, timeout=600)
    require(code == 0, 'export natif en echec : %s %s' % (tag, se))
    return fc.load_export(out), fc.file_sha256(out)


def band_attach(T, e, K, phi, eta):
    """cover_band_lca (auditeur) : LCA des temoins forts de niveau <= (1+eta)^2 alpha, attache a max(alpha, LCA)."""
    fc = FC()
    Wu = fc.witness_universe(e, K, strong=True)
    o = e.order(K)
    dates, nodes = [], []
    for s in range(len(e.sites)):
        alpha = o.cover_level(s)
        thr = (1 + F(eta)) ** 2 * alpha
        J = None
        for _b, lvl, v in Wu[s]:
            if lvl <= thr:
                J = v if J is None else T.lca(J, v)
        date = max(alpha, T.birth[J])
        dates.append(date)
        nodes.append(T.anc(J, date))
    return PA.fixed_attach(T, dates, nodes, phi)


def hard_rules(T, cover, e, K, zs):
    o = e.order(K)
    n = len(e.sites)
    phi2 = Phi(1, force_decimal=True)
    out = {
        'core': (PA.fixed_attach(T, [F(o.core_level[s]) for s in range(n)], o.core_node, phi2), phi2),
        'cover_A1': (PA.fixed_attach(T, [o.cover_level(s) for s in range(n)], o.cover_node, phi2), phi2),
        'band_eta1/8': (band_attach(T, e, K, phi2, F(1, 8)), phi2),
        'branch_step_z2': (PA.majority_step_fast(T, cover, phi2), phi2),
    }
    for z in zs:
        phi = Phi(F(z, 2), force_decimal=True)
        out['branch_gradual_z%d' % z] = (PA.majority_gradual_fast(T, cover, phi), phi)
    return out


def open_block(T, a, phi, beta):
    """Bloc de l'attache a juste avant beta (coupe ouverte) ; None si non attache avant beta."""
    if a.owner is None or cmp(a.phi_date, phi(beta), soft=True) <= 0:
        return None
    # vivant juste avant beta : naissance < beta <= mort ; rang du plus grand niveau < beta
    from bisect import bisect_left
    r = bisect_left(T.level_list, beta) - 1
    return T.anc_rank(a.owner, r)


def spurious_reference(T, e, K, labels_site):
    o = e.order(K)
    classes = sorted({l for l in labels_site if l >= 0})
    reps = {}
    for g in classes:
        members = [s for s in range(len(labels_site)) if labels_site[s] == g]
        reps[g] = min(members, key=lambda s: (o.core_level[s], e.point_id[s]))
    best = None
    for g in classes:
        for h in classes:
            if g < h:
                a, b = reps[g], reps[h]
                lv = max(F(o.core_level[a]), F(o.core_level[b]), T.birth[T.lca(o.core_node[a], o.core_node[b])])
                best = lv if best is None or lv < best else best
    return reps, best


def recall_metrics(T, atts, phi, labels_site, roles_site, reps, r_sp):
    blocks = [open_block(T, a, phi, r_sp) for a in atts]
    rep_block = {g: blocks[s] for g, s in reps.items()}
    res = {}
    for g in reps:
        for role_name, role in (('coeur', 0), ('halo', 1), ('tous', None)):
            mem = [s for s in range(len(atts)) if labels_site[s] == g and (role is None or roles_site[s] == role)]
            if not mem:
                continue
            ok = sum(1 for s in mem if blocks[s] is not None and blocks[s] == rep_block[g])
            wrong = sum(1 for s in mem if blocks[s] is not None and any(blocks[s] == rep_block[h]
                                                                          for h in reps if h != g))
            res['classe%d_%s' % (g, role_name)] = {'n': len(mem), 'rappel': ok / len(mem), 'croise': wrong / len(mem)}
    res['differes_avant_r_sp'] = sum(1 for b in blocks if b is None) / len(atts)
    res['representants_attaches'] = all(rep_block[g] is not None for g in reps)
    return res


def merge_radius(T, atts, phi, s, t):
    m = PA.merge_phi(T, atts, s, t, phi)
    return None if m is None else math.sqrt(phi.inverse(m))


def selection_block(T, cover, e, K, hard, labels_site, zs_eom, mcs):
    out = {}
    for z in zs_eom:
        phi = Phi(F(z, 2), force_decimal=True)
        for mode in ('step', 'gradual'):
            st = SE.fractional_stats_fast(T, cover, phi, mode)
            cl = SE.condense(T, st, mcs)
            chosen, _S = SE.eom(T, st, cl)
            lab, margins = SE.vote_labels_fast(T, cover, phi, cl, chosen)
            key = 'fractional_%s_z%d' % (mode, z)
            coh = None
            gname = 'branch_gradual_z%d' % z
            if gname in hard:
                atts = hard[gname][0]
                hl = SE.hard_labels(T, atts, cl, chosen)
                coh = sum(1 for s in range(len(lab)) if hl[s] >= 0 and lab[s] != hl[s])
            out[key] = {'clusters_condenses': len(cl), 'choisis': len(chosen),
                        'ARI': float(SE.ari(labels_site, lab)), 'bruit': lab.count(-1) / len(lab),
                        'violations_coherence_S3': coh,
                        'marge_vote_mediane': sorted(m for m in margins if m is not None)[len(lab) // 2]
                        if any(m is not None for m in margins) else None}
        for rname, (atts, _phi) in hard.items():
            st = SE.hard_stats(T, atts, phi)
            cl = SE.condense(T, st, mcs)
            chosen, _S = SE.eom(T, st, cl)
            lab = SE.hard_labels(T, atts, cl, chosen)
            out['hard_%s_eom_z%d' % (rname, z)] = {'choisis': len(chosen), 'ARI': float(SE.ari(labels_site, lab)),
                                                    'bruit': lab.count(-1) / len(lab)}
    return out


def pair_panel(n, m, seed):
    rng = random.Random(seed)
    pairs = set()
    while len(pairs) < min(m, n * (n - 1) // 2):
        a, b = rng.randrange(n), rng.randrange(n)
        if a != b:
            pairs.add((min(a, b), max(a, b)))
    return sorted(pairs)


def run_scene(sid, K, zs, zs_eom, tmp, with_jitter=True):
    sdir = os.path.join(FRONT, 'scenes', sid)
    meta, pts, lab, rol = read_scene(sdir)
    t0 = time.time()
    e, sha = export(pts, K, sid, tmp)
    T, cover, info = native_tree(e, K)
    n = len(e.sites)
    labels_site = [lab[e.point_id[s]] for s in range(n)]
    roles_site = [rol[e.point_id[s]] for s in range(n)]
    hard = hard_rules(T, cover, e, K, zs)
    reps, r_sp = spurious_reference(T, e, K, labels_site)
    res = {'scene': sid, 'K': K, 'n': n, 'export_sha256': sha, 'nodes': len(T),
           'witness_incidences': info['witness_incidences'], 'coverage_units': sum(len(c) for c in cover),
           'r_sp': str(r_sp), 'r_sp_radius': math.sqrt(r_sp), 'rules': {}}
    for rname, (atts, phi) in hard.items():
        lam = PA.check_laminar(T, atts, phi, max_cuts=120)
        res['rules'][rname] = {'avant_fusion_parasite': recall_metrics(T, atts, phi, labels_site, roles_site,
                                                                       reps, r_sp),
                               'transitions_laminaires': lam}
    mcs = round(math.sqrt(n))
    res['mcs'] = mcs
    res['selection'] = selection_block(T, cover, e, K, hard, labels_site, zs_eom, mcs)
    if with_jitter:
        panel = pair_panel(n, 2000, 'dev-verrou-masses-%s' % sid)
        site_of = [None] * n
        for s in range(n):
            site_of[e.point_id[s]] = s
        base_rho = {r: [merge_radius(T, hard[r][0], hard[r][1], site_of[a], site_of[b]) for a, b in panel]
                    for r in hard}
        jit = {}
        for tag in ('delta1', 'delta4'):
            pdir = os.path.join(FRONT, 'perturbations', sid, tag)
            with open(os.path.join(pdir, 'perturb.json')) as f:
                pm = json.load(f)
            require(pm.get('split') == 'dev', 'perturbation non dev')
            ppts = read_points(os.path.join(pdir, 'points.u32le'))
            e2, sha2 = export(ppts, K, sid + '_' + tag, tmp)
            T2, cover2, _i2 = native_tree(e2, K)
            hard2 = hard_rules(T2, cover2, e2, K, zs)
            site2 = [None] * n
            for s in range(n):
                site2[e2.point_id[s]] = s
            eps = math.sqrt(pm['eps2'])
            row = {'eps': eps, 'export_sha256': sha2}
            for r in hard:
                diffs = []
                for (a, b), u0 in zip(panel, base_rho[r]):
                    u1 = merge_radius(T2, hard2[r][0], hard2[r][1], site2[a], site2[b])
                    if u0 is not None and u1 is not None:
                        diffs.append(abs(u0 - u1))
                diffs.sort()
                row[r] = {'max': diffs[-1], 'mediane': diffs[len(diffs) // 2], 'q99': diffs[int(0.99 * len(diffs))],
                          'part_au_dela_2eps': sum(1 for d in diffs if d > 2 * eps + 1e-9) / len(diffs)}
            jit[tag] = row
        res['jitter'] = jit
    res['seconds'] = time.time() - t0
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--scenes', default='bhc_n600_base,bhc_n600_couloir_dense,bhc_n600_halo_rare,bhc_n600_loin')
    ap.add_argument('--orders', default='2,5')
    ap.add_argument('--tmp', default='/tmp/mhgp10-verrou-points/masses_selection/dev')
    ap.add_argument('--no-jitter', action='store_true')
    a = ap.parse_args()
    os.makedirs(a.tmp, exist_ok=True)
    zs = (2, 3, 4)
    zs_eom = (2, 3)
    report = {'schema': 'verrou_points_masses_dev_v1', 'python_optimize': sys.flags.optimize,
              'decision_arithmetic': 'Decimal 110 chiffres, marge relative 1e-80, egalites douces comptees',
              'runs': []}
    for K in map(int, a.orders.split(',')):
        for sid in a.scenes.split(','):
            r = run_scene(sid, K, zs, zs_eom, a.tmp, with_jitter=not a.no_jitter)
            report['runs'].append(r)
            print(json.dumps({'scene': sid, 'K': K, 'seconds': round(r['seconds'], 1)}), flush=True)
            with open(a.out + '.partial', 'w') as f:
                f.write(json.dumps(report, indent=1, sort_keys=True) + '\n')
    report['soft_ties'] = SOFT_TIES[0]
    data = json.dumps(report, indent=1, sort_keys=True)
    with open(a.out, 'w') as f:
        f.write(data + '\n')
    print(json.dumps({'status': 'ok', 'sha256': hashlib.sha256(data.encode()).hexdigest(),
                      'soft_ties': SOFT_TIES[0]}))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except MathError as err:
        print(json.dumps({'status': 'math_error', 'reason': str(err)}))
        sys.exit(1)
