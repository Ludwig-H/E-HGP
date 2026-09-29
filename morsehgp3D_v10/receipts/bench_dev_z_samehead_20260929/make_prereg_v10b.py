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
CAP_SK_FILES = ('kmatch_dev_A.csv', 'kcover_dev_k5cap.csv', 'kcover_dev_k8cap.csv', 'kcover_dev_k8cap_missing.csv',
                'kcover_dev_k10cap.csv')


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


def load(files, methods):
    cells = collections.defaultdict(lambda: collections.defaultdict(list))
    units = collections.defaultdict(set)
    for f in files:
        path = os.path.join(DEV, f)
        if not os.path.exists(path):
            continue
        for r in csv.DictReader(open(path)):
            m = ALIAS.get(r['method'], r['method'])
            if m not in methods:
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


def choose(J, units, methods, k, simplicity, need=256, fill=None):
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
    Jc, Uc = load(COVER_FILES, {'cover', 'cover1'})
    Jo, Uo = load(CAP_SK_FILES, {'cap', 'sklearn'})
    methods, pairs, pairs_b, reported, basis, nofill = [], [], [], [], {}, {}
    for k in KS:
        (tm, _, th, tf), tj, tbest, trank, tn = choose(Jc, Uc, {'cover', 'cover1'}, k, simplicity_cover)
        (_, _, hh, hf), hj, hbest, hrank, hn = choose(Jo, Uo, {'sklearn'}, k, simplicity_lotA)
        z = z_of(th)
        hsel, ha = hh.split('_')
        methods.append(dict(name='tw_K%d' % k, kind='tower', entry=tm, k=k, mcs='sqrt', selection='eom',
                            z='zhat' if z == 'zhat' else z, fill=tf))
        methods.append(dict(name='hdb_ms%d' % k, kind='sklearn', min_samples=k, mcs='sqrt', selection=hsel,
                            alpha=float(ha[1:]), fill=hf))
        pairs.append(dict(name='K=%d' % k, method='tw_K%d' % k, adversary='hdb_ms%d' % k))
        # paire descriptive sans remplissage (audit IMP-11) : meilleure tete dev de chaque methode parmi fill = none
        (tm0, _, th0, _), tj0, _, _, _ = choose(Jc, Uc, {'cover', 'cover1'}, k, simplicity_cover, fill='none')
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
                      pairs=pairs, secondary_pairs=pairs_b, reported=reported),
        dev_basis=dict(choices=basis, nofill=nofill, cover_files=list(COVER_FILES),
                       cap_sklearn_files=list(CAP_SK_FILES)),
    )
    prereg['pins'] = dict(mhgp10_cluster_sha256=sha(os.path.join(a.build, 'mhgp10_cluster')),
                          scripts_sha256={n: sha(os.path.join(SYN, n)) for n in run_test.SCRIPTS},
                          versions={k: env[k] for k in ('numpy', 'scipy', 'sklearn')}, python=env['python'])
    json.dump(prereg, open(a.out, 'w'), indent=1, ensure_ascii=False, sort_keys=True)
    print('ecrit (brouillon, a completer : plan, texte, predictions)', a.out)
    return 0


if __name__ == '__main__':
    sys.exit(main())
