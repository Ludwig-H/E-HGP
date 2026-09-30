"""Rejoue HDBSCAN(mcs=20) aux graines de calibration (11..15) et aux graines du plan (2026092800..04)
pour les cellules contestees ; lecture seule."""
import sys, warnings, statistics as st
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, baselines as b
cells = [('spherical','medium'),('spherical','hard'),('filaments','medium'),('filaments','hard'),('shells','medium'),('shells','hard'),('spherical','extreme')]
for fam, lv in cells:
    for label, seeds in (('calib', range(11,16)), ('plan', range(2026092800, 2026092805)), ('fresh', range(100,110))):
        v=[]
        for s in seeds:
            p,t,_=d.generate(dict(family=fam,n=2000,groups=8,level=lv,noise_fraction=0.0,seed=s))
            v.append(b.hdbscan_default(p,t)['ari'])
        print('%-10s %-8s %-6s mean=%.3f sd=%.3f pub=%.2f vals=%s' % (fam, lv, label, st.mean(v), st.stdev(v), d.CALIBRATION[fam][lv], [round(x,2) for x in v]), flush=True)
