import sys, json, time
sys.dont_write_bytecode=True
sys.path.insert(0,'.'); sys.path.insert(0,'../synthetic_bench_20260928')
import cluster as C, measure as M
OUT=sys.argv[1]; f=sys.argv[2]
rep=json.load(open('%s/g_%s_k2.json'%(OUT,f)))
cofaces,gab,size=M.read_export(rep); del rep
births=M.facet_births(cofaces,gab,'gabriel')
facets,plateaus=C.facet_levels(cofaces,gab)
t=time.monotonic(); nodes,roots=C.merge_tree(facets,plateaus,births); t=time.monotonic()-t
tot=sum(len(v['members']) for v in nodes.values())
print(f,'facets',len(facets),'nodes',len(nodes),'roots',len(roots),'sum_members',tot,'merge_s %.2f'%t,flush=True)
