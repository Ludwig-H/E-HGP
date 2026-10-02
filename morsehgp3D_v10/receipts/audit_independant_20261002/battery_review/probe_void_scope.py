"""Sonde de metrique seule, cinq points abstraits ; aucun moteur ni donnee LiDAR."""
import importlib.util
import hashlib
import json
from pathlib import Path
import numpy as np
source = Path("/workspaces/E-HGP/build/v10-tour-vers-points/batterie_ab/code/ab_direct.py")
spec = importlib.util.spec_from_file_location("ab_direct_audit", source)
D = importlib.util.module_from_spec(spec)
spec.loader.exec_module(D)
# Meme univers d'identifiants 0..4. Cible t={0,1}; meilleur bloc B={0,1,2}.
# Les deux arbres et tous leurs rangs restent fixes quand seul le masque change.
Et = dict(rank=[0,0,1],parent=[2,2,None],target=[0,0,1,1,1])
Eb = dict(rank=[0,0,1],parent=[2,2,None],target=[0,0,0,1,1])
cases=[]
for name,w in [("tous_sites",[1,1,1,1,1]),("site_2_void",[1,1,0,1,1])]:
    target=D.Layout(Et,w)
    candidates=D.Layout(Eb,w)
    tau=D.cluster_thresholds(target)
    ev,tp,pc=D.target_forest(target,tau,2)
    m,inter,size,chosen=D.best_blocks_for_targets(candidates,target,ev)
    summary=D.summarize(m,inter,size)
    expected = {"tous_sites":([3,2],[2,2],[2,3]),"site_2_void":([2,2],[2,2],[2,2])}[name]
    if not (m.tolist(),inter.tolist(),size.tolist()) == expected:
        raise ValueError((name,m,inter,size))
    oracle=D.brute_best_blocks(Eb,Et,ev,w)
    if any(tuple(row)!=(int(a),int(b),int(c)) for row,a,b,c in zip(oracle,m,inter,size)):
        raise ValueError("desaccord avec reference ensembliste")
    target0=int(np.flatnonzero(ev==0)[0])
    den=int(size[target0]+m[target0]-inter[target0])
    cases.append(dict(case=name,geometric_sites=5,evaluated_sites=sum(w),events=ev.tolist(),
                      target_mass=target.mass[ev].tolist(),evaluated_target_size=m.tolist(),
                      intersection=inter.tolist(),candidate_size=size.tolist(),
                      focus_iou_fraction=[int(inter[target0]),den],
                      exact_targets=summary["n_eq1"],total_targets=summary["targets"]))
print(json.dumps(dict(scope="pure metric; no geometry, no labels inferred, no battery totals recomputed",
                     source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),cases=cases),
                 sort_keys=True,ensure_ascii=False,indent=2))
