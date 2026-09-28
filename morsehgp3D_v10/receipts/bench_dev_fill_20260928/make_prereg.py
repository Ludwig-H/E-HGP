"""Construit le preenregistrement de la campagne de test du clustering v10 a partir de l'etape 2 sur DEV.

Regle de choix (identique pour la tour et pour sklearn, EVAL_v2 § 7.2) : J = moyenne ponderee par cellule
(famille, niveau, bruit, taille) de l'ARI_s sur les scenes dev (n = 2 000 et 8 000) ; argmax J ; egalite a 0,002
pres departagee par la configuration la plus simple (K ou min_samples croissant, alpha = 1, z = 1, politique de
bruit la plus simple : none < full < b1.5 < b2 < b2.5 < b3).
"""
import collections
import csv
import hashlib
import json
import os
import sys

SYN = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, SYN)
import run_test  # noqa: E402

BUILD = '/workspaces/E-HGP/build/v10-bench-4a3d09d8a/build'
STAGE2 = '/workspaces/E-HGP/build/v10-persist/bench/stage2_dev.csv'
FILL_ORDER = ('none', 'full', 'b1.5', 'b2', 'b2.5', 'b3')


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


def choose(rows, method):
    cells = collections.defaultdict(lambda: collections.defaultdict(list))
    for r in rows:
        if r['method'] != method:
            continue
        cells[(r['config'], r['fill'])][(r['family'], r['level'], r['noise'], r['n'])].append(float(r['ari_s']))
    J = {}
    for key, per_cell in cells.items():
        J[key] = sum(sum(v) / len(v) for v in per_cell.values()) / len(per_cell)
    best = max(J.values())

    def simplicity(key):
        cfg, fill = key
        parts = cfg.split('_')
        k = int(parts[0])
        if method == 'tower':
            z = 0 if parts[2] == '1' or parts[3] == 'leaf' else 1
            return (k, 0, z, FILL_ORDER.index(fill))
        alpha = float(parts[3])
        return (k, 0 if alpha == 1.0 else 1, 0, FILL_ORDER.index(fill))
    near = [k for k, v in J.items() if v >= best - 0.002]
    pick = min(near, key=simplicity)
    ranking = sorted(J.items(), key=lambda kv: -kv[1])[:12]
    return pick, J[pick], best, ranking, len(J)


def main():
    rows = list(csv.DictReader(open(STAGE2)))
    units = len({(r['family'], r['level'], r['noise'], r['n'], r['seed']) for r in rows})
    tw, tw_j, tw_best, tw_rank, tw_n = choose(rows, 'tower')
    hd, hd_j, hd_best, hd_rank, hd_n = choose(rows, 'hdb')
    print('unites', units)
    print('tour', tw, round(tw_j, 4), 'max', round(tw_best, 4))
    print('hdb ', hd, round(hd_j, 4), 'max', round(hd_best, 4))
    for name, rank in (('tour', tw_rank), ('hdb', hd_rank)):
        for k, v in rank:
            print('  ', name, k, round(v, 4))
    tk, tm, tz, tsel = tw[0].split('_')
    hk, hm, hsel, ha = hd[0].split('_')
    P = dict(name='tw_P', kind='tower', k=int(tk), mcs=tm, z=tz if tsel == 'eom' else '1', selection=tsel, fill=tw[1])
    methods = [
        P,
        dict(P, name='tw_P_none', fill='none'),
        dict(P, name='tw_P_z1', z='1'),
        dict(name='hdb_dev_sk', kind='sklearn', min_samples=int(hk), mcs=hm, selection=hsel, alpha=float(ha),
             fill=hd[1]),
        dict(name='hdb_match', kind='sklearn', min_samples=int(tk), mcs=tm, selection=tsel, alpha=2.0, fill='none'),
        dict(name='hdb_match_fill', kind='sklearn', min_samples=int(tk), mcs=tm, selection=tsel, alpha=2.0,
             fill=tw[1]),
        dict(name='hdb_these_K1', kind='sklearn', min_samples=3, mcs='sqrt', selection='eom', alpha=1.0, fill='none'),
        dict(name='hdb_lib', kind='sklearn_default', fill='none'),
        dict(name='hdb_lib_fill', kind='sklearn_default', fill=tw[1]),
    ]
    env = run_test.environment()
    prereg = dict(
        id='PREREG_V10_CLUSTER_20260928',
        status='public_status=not_claimed ; mode=benchmark_only ; une seule execution sur l espace test (EVAL_v2 D11)',
        engine=dict(commit='4a3d09d8a', build='git archive 4a3d09d8a, CMake Release, g++ 13.3 (codespace)',
                    note='les sources C++ de morsehgp3D_v10 sont identiques entre 4a3d09d8a et le commit du '
                         'preenregistrement ; le binaire est epingle par son sha256'),
        plan=dict(split='test', families=list(run_test.scenes.FAMILIES), levels=list(run_test.scenes.LEVELS),
                  noises=[0.0, 0.1], sizes=[8000, 16000, 32000], replicates=5),
        primary='tw_P',
        methods=methods,
        decision=dict(alpha=0.05, delta_min=0.02, refusal_cap=0.01, permutations=100000, bootstrap=10000,
                      iut_adversaries=['hdb_dev_sk', 'hdb_match', 'hdb_match_fill', 'hdb_lib', 'hdb_these_K1'],
                      decisive_adversary='hdb_dev_sk',
                      reported_adversaries=['hdb_lib_fill', 'tw_P_none', 'tw_P_z1']),
        dev_basis=dict(stage1='receipts/bench_dev_20260928 (dev_r2 : grille tour K <= 5 et sklearn ms <= 20, '
                              'politiques none/full, n = 2 000 et 8 000, 256 scenes)',
                       stage2='8 meilleures configurations distinctes de chaque methode x politiques none, full, '
                              'b1.5, b2, b2.5, b3 ; memes 256 scenes dev',
                       rule='argmax de J (moyenne ponderee par cellule de l ARI_s) ; egalite a 0,002 pres : '
                            'configuration la plus simple',
                       tower=dict(config=tw[0], fill=tw[1], J=round(tw_j, 4), max=round(tw_best, 4),
                                  candidates=tw_n),
                       hdb=dict(config=hd[0], fill=hd[1], J=round(hd_j, 4), max=round(hd_best, 4), candidates=hd_n)),
        attribution_statement=(
            "Attribution, écrite avant le test : si K_P = 1, la hiérarchie de la tour est exactement celle de "
            "l'atteignabilité mutuelle à alpha = 2 (union de boules), le témoin géométrique E1 est nul par "
            "construction, et tout écart avec HDBSCAN vient de la tête (échelle ẑ, sélection, mcs = √n, politique "
            "de bruit), pas de la géométrie exacte de la tour. Si K_P > 1, aucune attribution n'est revendiquée : "
            "sur dev, la tour et l'atteignabilité alpha = 1 clusterisent de la même façon à K égal."),
        deviations_from_EVAL_v2=[
            '8 familles du banc v9 (niveaux calibres par HDBSCAN(20, 20) en v9, biais connu) au lieu des 16 familles '
            'a niveaux geometriques ; pas de temoins nuls ni de suites publiques',
            'garde AMI_nc (bruit en classe) au lieu de F1_H et AMI_s',
            'hdb_dev (tete v10 sur les sources mr) omis : a K = 1 la tour est egale a mr2 (E1 nul) ; les '
            'adversaires sont sklearn appele tel quel (directive utilisateur du 28 septembre 2026)',
            'selection par J en deux etapes (grille dev_r2, puis 8 meilleures x politiques de bruit), '
            'identique pour les deux methodes',
            'egalites de plus proche voisin du remplissage : ordre de scipy cKDTree (identique pour toutes les '
            'methodes), au lieu de l ordre lexicographique',
            'hdb_lib appelle HDBSCAN() tel quel (algorithm="auto"), comme un utilisateur naif ; les autres appels '
            'sklearn sont epingles a algorithm="kd_tree"'],
    )
    _, digest = run_test.plan_manifest(prereg)
    prereg['plan']['manifest_sha256'] = digest
    prereg['pins'] = dict(mhgp10_cluster_sha256=sha(os.path.join(BUILD, 'mhgp10_cluster')),
                          scripts_sha256={n: sha(os.path.join(SYN, n)) for n in run_test.SCRIPTS},
                          versions={k: env[k] for k in ('numpy', 'scipy', 'sklearn')},
                          python=env['python'])
    out = os.path.join(SYN, 'prereg', 'PREREG_V10_CLUSTER_20260928.json')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w') as f:
        json.dump(prereg, f, indent=1, ensure_ascii=False, sort_keys=True)
        f.write('\n')
    print('ecrit', out, sha(out))


if __name__ == '__main__':
    main()
