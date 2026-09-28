import sys, json, time, math
sys.dont_write_bytecode=True
sys.path.insert(0,'.'); sys.path.insert(0,'../synthetic_bench_20260928')
import cluster as C, measure as M
OUT=sys.argv[1]
for f in sys.argv[2:]:
    t0=time.monotonic(); rep=json.load(open('%s/g_%s_k2.json'%(OUT,f))); t1=time.monotonic()
    print(f,'json_load %.2f'%(t1-t0),flush=True)
    cofaces,gab,size=M.read_export(rep); t2=time.monotonic(); print(' read_export %.2f'%(t2-t1),flush=True)
    births=M.facet_births(cofaces,gab,'gabriel'); t3=time.monotonic(); print(' births %.2f'%(t3-t2),flush=True)
    facets,plateaus=C.facet_levels(cofaces,gab); t4=time.monotonic(); print(' facet_levels %.2f'%(t4-t3),flush=True)
    nodes,roots=C.merge_tree(facets,plateaus,births); t5=time.monotonic(); print(' merge_tree %.2f nodes %d roots %d'%(t5-t4,len(nodes),len(roots)),flush=True)
    sums,totals,masses,covered=M.measure(cofaces,gab,1,'gabriel'); t6=time.monotonic(); print(' measure %.2f'%(t6-t5),flush=True)
    n=int(f.split('_')[1])
    clusters,order=C.condense(nodes,roots,masses,births,math.sqrt(n),'radius',1,C.STABILITY_SCALES[0]); t7=time.monotonic(); print(' condense %.2f total %.2f'%(t7-t6,t7-t0),flush=True)
