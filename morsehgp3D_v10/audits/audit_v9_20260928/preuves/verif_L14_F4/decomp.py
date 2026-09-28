"""Verif L14-F4 : decomposition couverture / qualite, tour (cda636b5e) contre HDBSCAN apparie."""
import csv, math, sys, time
import numpy as np
S = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/verif_L14_F4'
sys.path.insert(0, S + '/pre'); sys.path.insert(0, S + '/bench'); sys.path.insert(0, S)
import run_tower as RT, bench_datasets as bd, plan as bench_plan, heads as H
from sklearn.metrics import adjusted_rand_score as ari, adjusted_mutual_info_score as ami
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'

def metrics(truth, lab, pts):
    lab = np.asarray(lab); cov = lab >= 0
    a = ari(truth, lab); m = ami(truth, lab)
    ac = ari(truth[cov], lab[cov]) if cov.sum() > 1 else float('nan')
    f = H.fill_noise(pts, lab); af = ari(truth, f)
    # vs truth restricted to truth-labelled points (bench truth -1 = noise/bridge)
    tl = truth >= 0
    return dict(ari=a, ami=m, cov=float(cov.mean()), ari_cov=ac, ari_fill=af)

def main():
    out = sys.argv[1]; nmax = int(sys.argv[2]); sel = sys.argv[3].split(',') if len(sys.argv) > 3 else None
    specs = [s for s in bench_plan.specifications(heavy=False) if s['family'] == 'spherical' and s['n'] == nmax]
    if sel: specs = [s for s in specs if s['scene'] in sel]
    with open(out, 'w', newline='') as h:
        w = None
        for i, s in enumerate(specs):
            t0 = time.time()
            pts, truth, meta = bd.generate({k: s[k] for k in bd.SPEC_KEYS})
            grid, _ = bd.quantize(pts)
            rep = RT.export(BIN, grid, 2, 2, S)
            n = len(pts); sq = int(round(math.sqrt(n)))
            tl, det = RT.tower_labels(rep, n, 2, 1, 'gabriel', math.sqrt(n), 'radius')
            rows = [('tower_k2_z1', metrics(truth, tl, pts))]
            from sklearn.cluster import HDBSCAN
            d20 = HDBSCAN(min_cluster_size=20, copy=True).fit(pts).labels_
            rows.append(('hdbscan_default', metrics(truth, d20, pts)))
            for ms in (2, 3):
                tree = H.slt(pts, ms); cl = H.condense(tree, n, sq)
                for z in (1, 3):
                    lab = H.labels_from(cl, H.select(cl, H.stabilities(cl, z), 'eom'), n)
                    rows.append(('eom_z%d_ms%d_sqrt' % (z, ms), metrics(truth, lab, pts)))
            for name, m in rows:
                r = dict(scene=s['scene'], seed=s['seed'], n=n, noise=s['noise_fraction'], method=name, **m)
                if w is None:
                    w = csv.DictWriter(h, fieldnames=list(r)); w.writeheader()
                w.writerow(r)
            h.flush()
            print('%d/%d %s %d tower=%.3f cov=%.2f %.1fs' % (i + 1, len(specs), s['scene'], s['seed'], rows[0][1]['ari'], rows[0][1]['cov'], time.time() - t0), flush=True)

main()
