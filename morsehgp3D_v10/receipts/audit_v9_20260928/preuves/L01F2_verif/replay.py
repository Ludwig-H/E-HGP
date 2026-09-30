import sys, os, argparse, importlib
rev=sys.argv[1]
base=os.path.join(os.path.dirname(os.path.abspath(__file__)),rev,'tower_clustering_20260928')
sys.path.insert(0,base)
import run_tower as RT
import plan as P
wanted={('spherical_n2000_g2_medium_noise0',2026092800),('spherical_n2000_g8_extreme_noise0',2026092800),('spherical_n500_g4_hard_noise0',2026092800)}
specs=[s for s in P.specifications(seeds=P.SEEDS, heavy=True) if (s['scene'],s['seed']) in wanted]
args=argparse.Namespace(export='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export',k=2,z=1,convention='gabriel',lambda_mode='radius',min_cluster_mass=0.0,workers=2)
import tempfile
with tempfile.TemporaryDirectory(dir=os.path.dirname(os.path.abspath(__file__))) as keep:
    for s in specs:
        row,detail=RT.run_one(s,args,keep)
        print(rev,s['scene'],s['seed'],'ARI=%.10f'%row['ari'],'clusters',row['clusters'],'cov',row['coverage'],detail)
