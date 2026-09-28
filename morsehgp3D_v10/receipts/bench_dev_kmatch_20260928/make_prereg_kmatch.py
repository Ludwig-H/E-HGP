"""Preenregistrement du panneau apparie K = min_samples, lot A (K = 1, 2, 3), a partir de kmatch_dev_A.csv (DEV).

Regle de choix, identique pour la tour et pour sklearn, a chaque K (EVAL_v2 § 7.2 restreint a K fixe) :
J = moyenne ponderee par cellule (famille, niveau, bruit, taille) de l'ARI_s sur les 256 scenes dev (n = 2 000 et
8 000) ; argmax J sur (tete, politique de bruit) ; egalite a 0,002 pres tranchee par la configuration la plus
simple : parametre d'echelle par defaut (z = 1 pour la tour, alpha = 1 pour sklearn), puis politique de bruit
(none < full < b1.5 < b2 < b2.5 < b3), puis EOM avant feuilles.
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
DEV = '/workspaces/E-HGP/build/v10-persist/bench/kmatch_dev_A.csv'
FILL_ORDER = ('none', 'full', 'b1.5', 'b2', 'b2.5', 'b3')
KS = (1, 2, 3)


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


def choose(rows, method, k):
    cells = collections.defaultdict(lambda: collections.defaultdict(list))
    for r in rows:
        if r['method'] == method and int(r['k']) == k:
            cells[(r['head'], r['fill'])][(r['family'], r['level'], r['noise'], r['n'])].append(float(r['ari_s']))
    J = {key: sum(sum(v) / len(v) for v in per.values()) / len(per) for key, per in cells.items()}
    best = max(J.values())

    def simplicity(key):
        head, fill = key
        sel, scale = head.split('_')
        default = scale in ('z1', 'a1')
        return (0 if default else 1, FILL_ORDER.index(fill), 0 if sel == 'eom' else 1)
    pick = min((key for key, v in J.items() if v >= best - 0.002), key=simplicity)
    return pick, J[pick], best, sorted(J.items(), key=lambda kv: -kv[1])[:6], len(J)


def main():
    rows = list(csv.DictReader(open(DEV)))
    units = len({(r['family'], r['level'], r['noise'], r['n'], r['seed']) for r in rows})
    methods, pairs, basis = [], [], {}
    for k in KS:
        (th, tf), tj, tbest, trank, tn = choose(rows, 'tower', k)
        (hh, hf), hj, hbest, hrank, hn = choose(rows, 'hdb', k)
        tsel, tz = th.split('_')
        hsel, ha = hh.split('_')
        methods.append(dict(name='tw_K%d' % k, kind='tower', k=k, mcs='sqrt', selection=tsel,
                            z='zhat' if tz == 'zzhat' else '1', fill=tf))
        methods.append(dict(name='hdb_ms%d' % k, kind='sklearn', min_samples=k, mcs='sqrt', selection=hsel,
                            alpha=float(ha[1:]), fill=hf))
        pairs.append(dict(name='K=%d' % k, method='tw_K%d' % k, adversary='hdb_ms%d' % k))
        basis['K%d' % k] = dict(tower=dict(head=th, fill=tf, J=round(tj, 4), max=round(tbest, 4), candidates=tn),
                                hdb=dict(head=hh, fill=hf, J=round(hj, 4), max=round(hbest, 4), candidates=hn),
                                dev_delta=round(tj - hj, 4))
        print('K=%d tour %s %s J=%.4f (max %.4f) | sklearn %s %s J=%.4f (max %.4f) | dev delta %+.4f'
              % (k, th, tf, tj, tbest, hh, hf, hj, hbest, tj - hj))
        for name, rank in (('tour', trank), ('sklearn', hrank)):
            print('    ', name, ' '.join('%s/%s=%.4f' % (a, b, v) for (a, b), v in rank))
    methods.append(dict(name='hdb_lib', kind='sklearn_default', fill='none'))
    methods.append(dict(name='hdb_these_K1', kind='sklearn', min_samples=3, mcs='sqrt', selection='eom', alpha=1.0,
                        fill='none'))
    best_k = max(KS, key=lambda k: basis['K%d' % k]['tower']['J'])
    reported = [dict(name='défaut sklearn', method='tw_K%d' % best_k, adversary='hdb_lib'),
                dict(name='protocole de la thèse', method='tw_K%d' % best_k, adversary='hdb_these_K1')]
    env = run_test.environment()
    prereg = dict(
        id='PREREG_V10_KMATCH_A_20260928',
        status='public_status=not_claimed ; mode=benchmark_only ; une seule execution sur l espace test (EVAL_v2 D11)',
        engine=dict(commit='4a3d09d8a', build='git archive 4a3d09d8a, CMake Release, g++ 13.3 (codespace)',
                    note='sources C++ de morsehgp3D_v10 identiques entre 4a3d09d8a et le commit du '
                         'preenregistrement ; binaire epingle par son sha256'),
        design=('Panneau apparie K = min_samples (directive utilisateur du 28 septembre 2026). Lot A : K = 1, 2, 3, '
                'teste maintenant en local. Lot B : K = 5, 8, 10, preenregistre separement apres l acceleration de '
                'la tour (cout actuel super-lineaire), avec la meme regle ; les deux lots sont publies quel que soit '
                'leur resultat. Aucun autre K n est teste sur l espace test pour ce panneau.'),
        plan=dict(split='test', families=list(run_test.scenes.FAMILIES), levels=list(run_test.scenes.LEVELS),
                  noises=[0.0, 0.1], sizes=[8000, 16000, 32000], replicates=5),
        methods=methods,
        decision=dict(alpha=0.05, delta_min=0.02, refusal_cap=0.01, permutations=100000, bootstrap=10000,
                      pairs=pairs, reported=reported),
        dev_basis=dict(source='receipts/bench_dev_kmatch_20260928 (kmatch_dev_A.csv, %d scenes dev, n = 2 000 et '
                              '8 000)' % units,
                       rule='a chaque K, argmax de J (moyenne ponderee par cellule de l ARI_s) sur (tete, politique '
                            'de bruit) ; egalite a 0,002 pres : echelle par defaut, puis politique la plus simple, '
                            'puis EOM',
                       grids=dict(tower='EOM z = 1, EOM z = zhat, feuilles', sklearn='EOM ou feuilles x alpha 1 ou 2',
                                  mcs='round(sqrt(n)) des deux cotes',
                                  fills='none, full, b1.5, b2, b2.5, b3 (coeur au max(K, 5)-ieme voisin)'),
                       choices=basis),
        prediction=('Ecrite avant le test, d apres dev : ecarts dev tour - sklearn = '
                    + ', '.join('%+.3f (K = %d)' % (basis['K%d' % k]['dev_delta'], k) for k in KS)
                    + ' ; on attend une victoire de la tour la ou l ecart dev depasse 0,02 et la parite ailleurs.'),
        attribution_statement=(
            "Attribution, écrite avant le test : à K = 1, la hiérarchie de la tour est exactement celle de sklearn à "
            "min_samples = 1 (liaison simple, à un facteur d'échelle près selon alpha) ; tout écart à K = 1 vient "
            "donc de la tête (échelle ẑ, sélection, politique de bruit), pas de la géométrie. À K ≥ 2, les deux "
            "hiérarchies diffèrent (multicouverture exacte contre atteignabilité mutuelle) et les têtes aussi ; "
            "l'écart mesure le couple hiérarchie + tête."),
        deviations_from_EVAL_v2=[
            '8 familles du banc v9 (niveaux calibres par HDBSCAN(20, 20) en v9, biais connu) au lieu de 16 familles a '
            'niveaux geometriques ; pas de temoins nuls ni de suites publiques',
            'comparaison appariee K = min_samples par lot, au lieu de l IUT sur six adversaires',
            'garde AMI_nc au lieu de F1_H et AMI_s',
            'mcs = round(sqrt(n)) fixe des deux cotes',
            'egalites de plus proche voisin du remplissage : ordre de scipy cKDTree (identique pour toutes les '
            'methodes)',
            'hdb_lib appelle HDBSCAN() tel quel (algorithm="auto") ; les autres appels sklearn sont epingles a '
            'algorithm="kd_tree"'],
    )
    _, digest = run_test.plan_manifest(prereg)
    prereg['plan']['manifest_sha256'] = digest
    prereg['pins'] = dict(mhgp10_cluster_sha256=sha(os.path.join(BUILD, 'mhgp10_cluster')),
                          scripts_sha256={n: sha(os.path.join(SYN, n)) for n in run_test.SCRIPTS},
                          versions={k: env[k] for k in ('numpy', 'scipy', 'sklearn')}, python=env['python'])
    out = os.path.join(SYN, 'prereg', prereg['id'] + '.json')
    with open(out, 'w') as f:
        json.dump(prereg, f, indent=1, ensure_ascii=False, sort_keys=True)
        f.write('\n')
    print('ecrit', out, sha(out))


if __name__ == '__main__':
    main()
