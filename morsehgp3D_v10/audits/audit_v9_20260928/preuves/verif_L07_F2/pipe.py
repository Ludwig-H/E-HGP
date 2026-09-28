import sys, json, time, math, resource
sys.dont_write_bytecode=True
T='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928'
sys.path.insert(0,T)
import cluster as C, measure as M
path=sys.argv[1]; n=int(sys.argv[2])
def rss(): return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1e6
t0=time.monotonic(); rep=json.load(open(path)); t1=time.monotonic(); print('json %.2f rss %.2fGB'%(t1-t0,rss()),flush=True)
nat_roots=rep['native']['roots']
cofaces,gab,size=M.read_export(rep); del rep; t2=time.monotonic(); print('read %.2f cofaces %d gabriel %d'%(t2-t1,len(cofaces),len(gab)),flush=True)
births=M.facet_births(cofaces,gab,'gabriel'); t3=time.monotonic(); print('births %.2f'%(t3-t2),flush=True)
facets,plateaus=C.facet_levels(cofaces,gab); t4=time.monotonic(); print('levels %.2f facets %d plateaus %d'%(t4-t3,len(facets),len(plateaus)),flush=True)
nodes,roots=C.merge_tree(facets,plateaus,births); t5=time.monotonic()
tot=sum(len(v['members']) for v in nodes.values())
print('merge_tree %.2f nodes %d roots %d sum_members %d rss %.2fGB native_roots %s'%(t5-t4,len(nodes),len(roots),tot,rss(),nat_roots),flush=True)
# composantes du graphe des facettes : taille (en facettes) de chaque racine
sizes=sorted((len(nodes[r]['members']) if r in nodes else 1) for r in roots)
print('root sizes', sizes, flush=True)
sums,totals,masses,covered=M.measure(cofaces,gab,1,'gabriel'); t6=time.monotonic(); print('measure %.2f covered %d/%d'%(t6-t5,len(covered),n),flush=True)
clusters,order=C.condense(nodes,roots,masses,births,math.sqrt(n),'radius',1,'lambda'); t7=time.monotonic()
print('condense %.2f total(excl export) %.2f rss %.2fGB'%(t7-t6,t7-t0,rss()),flush=True)
