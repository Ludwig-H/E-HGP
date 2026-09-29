"""Preenregistrement du lot C (tete v10-b, entree par premiere couverture) et du lot B promis (tete C n X a
K = 5, 8, 10), a partir des CSV DEV. Espace de graines neuf `test_v10b` (EVAL_v2 D11 : la tete a change).

Regle de choix, identique pour chaque methode et chaque K : J = moyenne ponderee par cellule (famille, niveau,
bruit, taille) de l'ARI_s sur les 256 scenes dev (n = 2 000 et 8 000) ; argmax de J sur les candidats declares ;
egalite a 0,002 pres tranchee par la configuration la plus simple, dans cet ordre :
  - tour cover : entree cover avant cover1, puis z = 3 (echelle de densite K-NN ambiante) puis |z - 3| croissant
    (zhat compte comme son ecart median a 3, soit 0), puis politique de bruit (none < full < b1.5 < b2 < b2.5 < b3) ;
  - tour C n X (lot B, grille du lot A) et sklearn : regle du lot A (echelle par defaut, politique, EOM).

  python3 make_prereg_v10b.py --build BUILD --out PREREG.json [--dry]
"""
import argparse
import collections
import csv
import hashlib
import json
import os
import sys

SYN = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, SYN)
import run_test  # noqa: E402

DEV = '/workspaces/E-HGP/build/v10-persist/bench/'
FILL_ORDER = ('none', 'full', 'b1.5', 'b2', 'b2.5', 'b3')
KS = (1, 2, 3, 5, 8, 10)
KS_B = (5, 8, 10)
ALIAS = {'tower': 'cap', 'tower_cap': 'cap', 'tower_cover': 'cover', 'tower_cover1': 'cover1', 'hdb': 'sklearn'}
COVER_FILES = ('kcover_dev_zgrid.csv', 'kcover_dev_zgrid2.csv', 'kcover_dev_extra1.csv')
# Entree cover1 (alpha_{K+1}, semantique de HGP-old) mesuree sur dev : neutre des z >= 3 (+-0,0015), reçu
# bench_dev_z_samehead_20260929 ; exclue des candidats (entree la plus simple gardee, catalogue a l'ordre max(K)).
COVER_ENTRIES = {'cover'}
OBJ_FILES = ('samehead_border_dev.csv', 'samehead_border_dev_z56.csv')  # MR2-bord (hierarchie d'HDBSCAN, entree
                                                                       # bord, meme tete)
KS_OBJ = (2, 3, 5, 8, 10)  # a K = 1 les hierarchies coincident
CAP_SK_FILES = ('kmatch_dev_A.csv', 'kcover_dev_k5cap.csv', 'kcover_dev_k8cap.csv', 'kcover_dev_k8cap_missing.csv',
                'kcover_dev_k10cap.csv')


TEXT = dict(
    status='public_status=not_claimed ; mode=benchmark_only ; une seule execution sur l espace test_v10b (EVAL_v2 D11)',
    design=('Lot C : tete v10-b (entree des points par premiere couverture, amas discrets du theoreme 2 ; EOM avec '
            'lambda = r^(-z), z choisi sur dev) contre sklearn HDBSCAN a min_samples = K (directive du 28 septembre '
            '2026), K = 1, 2, 3, 5, 8, 10. Famille secondaire lot B (promis par le preenregistrement du lot A) : tete '
            'C n X du 28 septembre a K = 5, 8, 10, meme grille que le lot A. Famille secondaire « objet » (audit du 29 '
            'septembre, OBJ-01) : la tour contre la hierarchie d atteignabilite mutuelle d HDBSCAN (alpha = 2) munie '
            'de la MEME entree (regle des points-bord, analogue de la couverture) et de la MEME tete ; elle mesure ce '
            'que l objet exact apporte au-dela de l entree et de la tete. Paires descriptives sans remplissage (audit '
            'IMP-11 : le remplissage borne fait l essentiel du score de sklearn en feuilles a alpha = 2). Espace de '
            'graines neuf test_v10b, puisque la tete a change.'),
    dev_rule=('a chaque K, argmax de J (moyenne ponderee par cellule de l ARI_s, sur les 128 scenes dev de 8 000 points : '
              'l optimum de z croit avec n et les tailles de test commencent a 8 000) sur les candidats '
              'declares ; egalite a 0,002 pres : tour cover : z = 3 d abord puis |z - 3| croissant, puis politique la '
              'plus simple ; tour C n X et sklearn : regle du lot A (echelle par defaut, politique, EOM). Candidats '
              'tour cover : EOM a z dans {1, zhat, 2, 3, 4, 5, 6, 8} x 6 politiques ; entree cover1 (alpha_{K+1}) '
              'mesuree neutre des z >= 3 et exclue.'),
    attribution_statement=(
        'Attribution, écrite avant le test : à K = 1, les hiérarchies de la tour et de sklearn coïncident (liaison '
        'simple) ; l’écart y vient entièrement de la tête (z, politique de bruit). À K ≥ 2, la paire principale '
        'mesure ensemble la hiérarchie, l’entrée des points et la tête ; la famille « objet » isole la hiérarchie '
        '(même entrée, même tête, même remplissage) : une parité y signifierait que l’avantage sur HDBSCAN vient de '
        'l’entrée et de la tête, applicables aussi à la hiérarchie d’HDBSCAN.'),
    deviations_from_EVAL_v2=[
        '8 familles du banc v9 au lieu de 16 familles a niveaux geometriques ; pas de temoins nuls',
        'comparaison appariee K = min_samples, au lieu de l IUT sur six adversaires',
        'garde AMI_nc au lieu de F1_H et AMI_s',
        'mcs = round(sqrt(n)) fixe des deux cotes',
        'remplissage borne a k = max(K, 5) au lieu de core_K (ecart deja declare au lot A)',
        'famille « objet » : hierarchie d HDBSCAN par le temoin mhgp10_mreach_cluster (egal a sklearn aux egalites '
        'de plateau pres), entree bord et tete v10 ; ce n est pas sklearn tel quel',
        'egalites de plus proche voisin du remplissage : ordre de scipy cKDTree'],
)


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


DEV_N = '8000'  # choix sur les scenes dev de 8 000 points : l'optimum de z croit avec n (contre-verification de
               # l'audit, EHO-01) et les tailles de test commencent a 8 000 ; meme restriction pour toutes les methodes


def load(files, methods):
    cells = collections.defaultdict(lambda: collections.defaultdict(list))
    units = collections.defaultdict(set)
    for f in files:
        path = os.path.join(DEV, f)
        if not os.path.exists(path):
            continue
        for r in csv.DictReader(open(path)):
            m = ALIAS.get(r['method'], r['method'])
            if m not in methods or r['n'] != DEV_N:
                continue
            key = (m, int(r['k']), r['head'], r['fill'])
            cells[key][(r['family'], r['level'], r['noise'], r['n'])].append(float(r['ari_s']))
            units[key].add((r['family'], r['level'], r['noise'], r['n'], r['seed']))
    J = {k: sum(sum(v) / len(v) for v in per.values()) / len(per) for k, per in cells.items()}
    return J, units


def z_of(head):
    return head.split('_', 1)[1][1:]  # eom_z3 -> '3', eom_zzhat -> 'zhat'


def simplicity_cover(key):
    m, _, head, fill = key
    z = z_of(head)
    dz = 0.0 if z == 'zhat' else abs(float(z) - 3.0)
    return (0 if m == 'cover' else 1, 0 if z == '3' else 1, dz, FILL_ORDER.index(fill), 0 if head.startswith('eom') else 1)


def simplicity_lotA(key):
    _, _, head, fill = key
    sel, scale = head.split('_')
    return (0 if scale in ('z1', 'a1') else 1, FILL_ORDER.index(fill), 0 if sel == 'eom' else 1)


def choose(J, units, methods, k, simplicity, need=128, fill=None):
    cands = {key: v for key, v in J.items() if key[0] in methods and key[1] == k and len(units[key]) == need
             and (fill is None or key[3] == fill)}
    best = max(cands.values())
    pick = min((key for key, v in cands.items() if v >= best - 0.002), key=simplicity)
    top = sorted(cands.items(), key=lambda kv: -kv[1])[:6]
    return pick, cands[pick], best, top, len(cands)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--dry', action='store_true')
    a = ap.parse_args()
    Jc, Uc = load(COVER_FILES, COVER_ENTRIES)
    Jo, Uo = load(CAP_SK_FILES, {'cap', 'sklearn'})
    methods, pairs, pairs_b, reported, basis, nofill = [], [], [], [], {}, {}
    for k in KS:
        (tm, _, th, tf), tj, tbest, trank, tn = choose(Jc, Uc, COVER_ENTRIES, k, simplicity_cover)
        (_, _, hh, hf), hj, hbest, hrank, hn = choose(Jo, Uo, {'sklearn'}, k, simplicity_lotA)
        z = z_of(th)
        hsel, ha = hh.split('_')
        methods.append(dict(name='tw_K%d' % k, kind='tower', entry=tm, k=k, mcs='sqrt', selection='eom',
                            z='zhat' if z == 'zhat' else z, fill=tf))
        methods.append(dict(name='hdb_ms%d' % k, kind='sklearn', min_samples=k, mcs='sqrt', selection=hsel,
                            alpha=float(ha[1:]), fill=hf))
        pairs.append(dict(name='K=%d' % k, method='tw_K%d' % k, adversary='hdb_ms%d' % k))
        # paire descriptive sans remplissage (audit IMP-11) : meilleure tete dev de chaque methode parmi fill = none
        (tm0, _, th0, _), tj0, _, _, _ = choose(Jc, Uc, COVER_ENTRIES, k, simplicity_cover, fill='none')
        (_, _, hh0, _), hj0, _, _, _ = choose(Jo, Uo, {'sklearn'}, k, simplicity_lotA, fill='none')
        z0 = z_of(th0)
        hsel0, ha0 = hh0.split('_')
        methods.append(dict(name='tw_K%d_nf' % k, kind='tower', entry=tm0, k=k, mcs='sqrt', selection='eom',
                            z='zhat' if z0 == 'zhat' else z0, fill='none'))
        methods.append(dict(name='hdb_ms%d_nf' % k, kind='sklearn', min_samples=k, mcs='sqrt', selection=hsel0,
                            alpha=float(ha0[1:]), fill='none'))
        reported.append(dict(name='K=%d sans remplissage' % k, method='tw_K%d_nf' % k, adversary='hdb_ms%d_nf' % k))
        nofill['K%d' % k] = dict(tower=dict(entry=tm0, head=th0, J=round(tj0, 4)), hdb=dict(head=hh0, J=round(hj0, 4)),
                                 dev_delta=round(tj0 - hj0, 4))
        print('     sans remplissage : tour %s %s J=%.4f | sklearn %s J=%.4f | delta %+.4f'
              % (tm0, th0, tj0, hh0, hj0, tj0 - hj0))
        basis['K%d' % k] = dict(tower=dict(entry=tm, head=th, fill=tf, J=round(tj, 4), max=round(tbest, 4),
                                           candidates=tn),
                                hdb=dict(head=hh, fill=hf, J=round(hj, 4), max=round(hbest, 4), candidates=hn),
                                dev_delta=round(tj - hj, 4))
        print('K=%-2d tour %s %s %s J=%.4f (max %.4f, %d cand.) | sklearn %s %s J=%.4f | delta %+.4f'
              % (k, tm, th, tf, tj, tbest, tn, hh, hf, hj, tj - hj))
        for name, rank in (('tour', trank), ('sklearn', hrank)):
            print('     ', name, ' '.join('%s/%s/%s=%.4f' % (kk[0], kk[2], kk[3], v) for kk, v in rank))
    # famille « objet » : meme entree (bord) et meme tete (z, remplissage) que la tour, hierarchie d'HDBSCAN
    Jm, Um = load(OBJ_FILES, {'mr2b'})
    pairs_obj = []
    for k in KS_OBJ:
        tw = next(m for m in methods if m['name'] == 'tw_K%d' % k)
        head = 'eom_z%s' % tw['z']
        methods.append(dict(name='mrb_K%d' % k, kind='mreach', alpha=2, entry='border', k=k, mcs='sqrt',
                            selection='eom', z=tw['z'], fill=tw['fill']))
        pairs_obj.append(dict(name='objet, K=%d' % k, method='tw_K%d' % k, adversary='mrb_K%d' % k))
        key = ('mr2b', k, head, tw['fill'])
        jm = Jm.get(key) if len(Um.get(key, ())) == 128 else None  # base dev complete seulement
        basis['OBJ_K%d' % k] = dict(mrb=dict(head=head, fill=tw['fill'], J=round(jm, 4) if jm is not None else None),
                                    dev_delta=round(basis['K%d' % k]['tower']['J'] - jm, 4) if jm is not None else None)
        print('objet K=%-2d MR2-bord %s %s J=%s | delta tour - MR2-bord %s'
              % (k, head, tw['fill'], '%.4f' % jm if jm is not None else '-',
                 '%+.4f' % (basis['K%d' % k]['tower']['J'] - jm) if jm is not None else '-'))
    for k in KS_B:
        (_, _, ch, cf), cj, cbest, crank, cn = choose(Jo, Uo, {'cap'}, k, simplicity_lotA)
        csel, cz = ch.split('_')
        methods.append(dict(name='cap_K%d' % k, kind='tower', entry='core', k=k, mcs='sqrt', selection=csel,
                            z='zhat' if cz == 'zzhat' else cz[1:], fill=cf))
        pairs_b.append(dict(name='lot B, K=%d' % k, method='cap_K%d' % k, adversary='hdb_ms%d' % k))
        basis['B_K%d' % k] = dict(cap=dict(head=ch, fill=cf, J=round(cj, 4), max=round(cbest, 4), candidates=cn),
                                  dev_delta=round(cj - basis['K%d' % k]['hdb']['J'], 4))
        print('lot B K=%-2d C n X %s %s J=%.4f | delta %+.4f' % (k, ch, cf, cj, cj - basis['K%d' % k]['hdb']['J']))
    if a.dry:
        return 0
    env = run_test.environment()
    prereg = dict(
        id=os.path.splitext(os.path.basename(a.out))[0],
        methods=methods,
        decision=dict(alpha=0.05, delta_min=0.02, refusal_cap=0.01, permutations=100000, bootstrap=10000,
                      pairs=pairs, reported=reported,
                      secondary=[dict(name='lot B, tête C∩X du 28 septembre', pairs=pairs_b,
                                      method_label='la tour (C∩X)', adversary_label='HDBSCAN'),
                                 dict(name='objet exact contre hiérarchie d’HDBSCAN, même entrée et même tête',
                                      pairs=pairs_obj, method_label='la tour',
                                      adversary_label='MR2-bord')]),
        dev_basis=dict(choices=basis, nofill=nofill, cover_files=list(COVER_FILES),
                       cap_sklearn_files=list(CAP_SK_FILES)),
    )
    prereg['pins'] = dict(mhgp10_cluster_sha256=sha(os.path.join(a.build, 'mhgp10_cluster')),
                          mhgp10_mreach_cluster_sha256=sha(os.path.join(a.build, 'mhgp10_mreach_cluster')),
                          scripts_sha256={n: sha(os.path.join(SYN, n)) for n in run_test.SCRIPTS},
                          versions={k: env[k] for k in ('numpy', 'scipy', 'sklearn')}, python=env['python'])
    prereg.update(TEXT)
    prereg['engine'] = dict(commit='c764e121a', build='git archive c764e121a, CMake Release, g++ 13.3 (codespace)',
                            note='binaires mhgp10_cluster et mhgp10_mreach_cluster epingles par leur sha256 ; scripts '
                                 'du banc identiques entre c764e121a et le commit du preenregistrement')
    prereg['plan'] = dict(split='test_v10b', families=list(run_test.scenes.FAMILIES), levels=list(run_test.scenes.LEVELS),
                          noises=[0.0, 0.1], sizes=[8000, 16000, 32000], replicates=5)
    prereg['prediction'] = ('Ecrite avant le test, d apres les 128 scenes dev de 8 000 points : ecarts tour - sklearn = '
                            + ', '.join('%+.3f (K = %d)' % (basis['K%d' % k]['dev_delta'], k) for k in KS)
                            + ' ; sans remplissage : '
                            + ', '.join('%+.3f' % nofill['K%d' % k]['dev_delta'] for k in KS)
                            + ' ; tour - MR2-bord a meme entree et meme tete : '
                            + ', '.join('%s (K = %d)' % ('%+.3f' % basis['OBJ_K%d' % k]['dev_delta']
                                                         if basis['OBJ_K%d' % k]['dev_delta'] is not None else '-', k)
                                        for k in KS_OBJ)
                            + '. On attend : victoire de la tour sur sklearn la ou l ecart dev depasse 0,02 (et '
                              'probablement au-dela, l ecart du lot A ayant cru avec n) ; parite de la tour et de '
                              'MR2-bord (audit OBJ-01) ; lot B perdant ou a parite.')
    run_test.plan_manifest(prereg)
    prereg['plan']['manifest_sha256'] = run_test.plan_manifest(prereg)[1]
    json.dump(prereg, open(a.out, 'w'), indent=1, ensure_ascii=False, sort_keys=True)
    open(a.out, 'a').write('\n')
    print('ecrit', a.out, sha(a.out))
    return 0


if __name__ == '__main__':
    sys.exit(main())
