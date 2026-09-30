import sys, warnings, statistics as st
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, baselines as b
seedsets = {'calib11-15': list(range(11,16)), 'plan': list(range(2026092800,2026092805)), 'zero0-4': list(range(0,5))}
which = sys.argv[1]
for fam in ('shells','hierarchical','spherical','filaments'):
    for n in (500, 2000, 8000):
        dv=[]
        for s in seedsets[which]:
            p,t,_ = d.generate(dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=s))
            dv.append(b.hdbscan_default(p,t)['ari'])
        print('%s %-12s n=%-5d default %.3f %s' % (which, fam, n, st.mean(dv), [round(x,2) for x in dv]), flush=True)
