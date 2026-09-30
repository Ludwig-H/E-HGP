"""Campagne L14 : tetes candidates sur le banc calibre du 28 septembre (n <= 8000).

Sortie CSV longue : scene, seed, family, level, n, groups, noise, method, ari, coverage, clusters.
Aucune methode ne voit la verite, sauf les lignes 'oracle_*' declarees comme telles.
"""
import csv
import math
import sys
import time

import numpy as np
from sklearn.metrics import adjusted_rand_score

sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as bd  # noqa: E402
import plan as bench_plan  # noqa: E402
import heads as H  # noqa: E402

MS_GRID = (2, 3, 5, 8, 12, 20, 32)
MCS_ORACLE = (5, 10, 15, 20, 30, 50, 75, 100)


def score(truth, lab):
    return float(adjusted_rand_score(truth, lab)), float((lab >= 0).mean()), int(len(set(lab.tolist()) - {-1}))


def run_scene(spec, writer, verify):
    pts, truth, meta = bd.generate({k: spec[k] for k in bd.SPEC_KEYS})
    n = len(pts)
    sq = int(round(math.sqrt(n)))
    rows = []

    def emit(method, lab, extra=''):
        a, c, k = score(truth, lab)
        rows.append(dict(scene=spec['scene'], seed=spec['seed'], family=spec['family'], level=spec['level'], n=n,
                         groups=spec['groups'], noise=spec['noise_fraction'], method=method, ari=a, coverage=c,
                         clusters=k, extra=extra))
        return a

    trees = {ms: H.slt(pts, ms) for ms in MS_GRID}
    cores = {ms: H.core_distances(pts, ms) for ms in MS_GRID}
    eom_labels = {}
    oracle_best = (-2, None)
    oracle_diag = (-2, None)
    for ms in MS_GRID:
        for mcs in sorted(set(MCS_ORACLE + (sq,))):
            if 2 * mcs > n:
                continue
            cl = H.condense(trees[ms], n, mcs)
            st1 = H.stabilities(cl, 1)
            lab = H.labels_from(cl, H.select(cl, st1, 'eom'), n)
            a = score(truth, lab)[0]
            if a > oracle_best[0]:
                oracle_best = (a, (ms, mcs, lab))
            if ms == 20 and mcs in MCS_ORACLE and a > oracle_diag[0]:
                pass
            eom_labels[(ms, mcs)] = lab
            if mcs == sq or mcs == 20:
                tag = 'mcs%s' % ('sqrt' if mcs == sq else '20')
                emit('eom_z1_ms%d_%s' % (ms, tag), lab)
                emit('eom_z1_ms%d_%s_fill' % (ms, tag), H.fill_noise(pts, lab))
                for z in (2, 3, 'log'):
                    lz = H.labels_from(cl, H.select(cl, H.stabilities(cl, z), 'eom'), n)
                    emit('eom_z%s_ms%d_%s' % (z, ms, tag), lz)
                    emit('eom_z%s_ms%d_%s_fill' % (z, ms, tag), H.fill_noise(pts, lz))
                ll = H.labels_from(cl, H.select(cl, st1, 'leaf'), n)
                emit('leaf_ms%d_%s' % (ms, tag), ll)
                emit('leaf_ms%d_%s_fill' % (ms, tag), H.fill_noise(pts, ll))
                tl, pers, gap, k = H.tomato(trees[ms], cores[ms], n, mcs)
                emit('tomato_ms%d_%s' % (ms, tag), tl, extra='%.4f' % gap)
    a, (ms, mcs, lab) = oracle_best
    emit('oracle2d_eom_z1', lab, extra='ms%d_mcs%d' % (ms, mcs))
    emit('oracle2d_eom_z1_fill', H.fill_noise(pts, lab), extra='ms%d_mcs%d' % (ms, mcs))
    # automatic K choices (no truth)
    for tag, mcsv in (('sqrt', sq), ('20', 20)):
        # (a) consensus across K: ms whose EOM partition agrees most with its grid neighbours
        labs = [eom_labels[(ms, mcsv)] for ms in MS_GRID]
        agree = []
        for i in range(len(MS_GRID)):
            nb = [j for j in (i - 1, i + 1) if 0 <= j < len(MS_GRID)]
            agree.append(np.mean([adjusted_rand_score(labs[i], labs[j]) for j in nb]))
        i = int(np.argmax(agree))
        emit('autoK_consensus_%s' % tag, labs[i], extra='ms%d' % MS_GRID[i])
        emit('autoK_consensus_%s_fill' % tag, H.fill_noise(pts, labs[i]), extra='ms%d' % MS_GRID[i])
        # (b) ToMATo: K whose prominence diagram has the largest log-gap
        best = None
        for ms in MS_GRID:
            tl, pers, gap, k = H.tomato(trees[ms], cores[ms], n, mcsv)
            if best is None or gap > best[0]:
                best = (gap, ms, tl)
        emit('autoK_tomato_gap_%s' % tag, best[2], extra='ms%d' % best[1])
    if verify:
        from sklearn.cluster import HDBSCAN
        ref = HDBSCAN(min_cluster_size=sq, min_samples=3, copy=True).fit(pts).labels_
        mine = eom_labels[(3, sq)]
        same = adjusted_rand_score(ref, mine)
        rows.append(dict(scene=spec['scene'], seed=spec['seed'], family=spec['family'], level=spec['level'], n=n,
                         groups=spec['groups'], noise=spec['noise_fraction'], method='verify_vs_sklearn',
                         ari=float(same), coverage=float((ref >= 0).mean()), clusters=int(len(set(ref)) - (1 if -1 in ref else 0)),
                         extra=''))
        ref20 = HDBSCAN(min_cluster_size=20, copy=True).fit(pts).labels_
        emit('sklearn_default', ref20)
        emit('sklearn_default_fill', H.fill_noise(pts, ref20))
    for r in rows:
        writer.writerow(r)


def main():
    out = sys.argv[1]
    only = sys.argv[2] if len(sys.argv) > 2 else None
    specs = [s for s in bench_plan.specifications(heavy=False) if s['n'] <= 8000]
    if only:
        specs = [s for s in specs if s['family'] in only.split(',')]
    fields = ('scene', 'seed', 'family', 'level', 'n', 'groups', 'noise', 'method', 'ari', 'coverage', 'clusters', 'extra')
    with open(out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=fields)
        w.writeheader()
        for i, s in enumerate(specs):
            t0 = time.time()
            run_scene(s, w, verify=True)
            h.flush()
            print('%d/%d %s %d %.1fs' % (i + 1, len(specs), s['scene'], s['seed'], time.time() - t0), flush=True)


if __name__ == '__main__':
    main()
