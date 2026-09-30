import sys, warnings, statistics as st
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, baselines as b
for fam in ('shells','hierarchical','spherical','filaments'):
    out=[]
    for n in (500, 2000, 8000):
        dv=[]
        for s in (11,12,13):
            p,t,_ = d.generate(dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=s))
            dv.append(b.hdbscan_default(p,t)['ari'])
        out.append('%.4f' % st.mean(dv))
    print('seeds11-13 %-12s 500/2000/8000 default %s' % (fam, ' / '.join(out)), flush=True)
