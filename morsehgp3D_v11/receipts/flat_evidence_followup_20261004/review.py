#!/usr/bin/env python3
"""Métadonnées figées et calcul Fraction F2 ; aucun import/fit scientifique ou produit."""
from pathlib import Path
from fractions import Fraction
import hashlib,json
B=Path(__file__).resolve().parent
checks=0
def need(ok,why):
 global checks
 checks+=1
 if not ok:raise ValueError(why)
def read(rel):return json.loads((B/rel).read_text())
first=read('SOURCE_BEFORE.json')
for manifest in ('SOURCE_BEFORE.json','SOURCE_ADDITIONAL_BEFORE.json','SOURCE_TIES_BEFORE.json'):
 for row in read(manifest)['sources']:
  if row.get('payload'):
   need(hashlib.sha256((B/row['payload']).read_bytes()).hexdigest()==row['sha256'],'source '+row['payload'])
for row in first['implementation_deltas']:
 if row.get('payload'):
  need(hashlib.sha256((B/row['payload']).read_bytes()).hexdigest()==row['sha256'],'delta '+row['payload'])
unchanged={r['relative']:r['same_as_previous_capture'] for r in first['implementation_deltas']}
need(unchanged['mesure/metriques.py'],'metric code changed')
need(unchanged['equite/equivalence.py'],'fallback code changed')
need(not unchanged['equite/nary_head.py'],'exact-date storage delta missing')
need(not unchanged['modele/scripts/modele_lib.py'],'completion delta missing')
eq=read('snapshot/equite/equivalence_out.json')
need(eq['by_eps']['zero']=={'configs':2400,'fit_crash':0,'i_ne_ii':520},'official epsilon-zero counts')
need(eq['stats']['configs']==6752 and eq['stats']['i_ne_ii']==1015 and eq['stats']['fit_crash']==1672,'aggregate/fallback counts')
need(eq['sklearn']=='1.9.1' and eq['numpy']=='2.5.3','local versions')
cond=read('snapshot/modele/sorties/verif_condensation.json')
need(cond=={'seed':20261004,'clouds':60,'sites':4041,'pairs':103428,'ecarts_R':0,'ecarts_R_A':0,'ecarts_paires':0},'condensation output')
fixtures=read('snapshot/modele/sorties/fixtures_existence.json')
flags=[]
def visit(v):
 if isinstance(v,dict):
  for k,x in v.items():
   if k=='non_tranche':flags.append(x)
   else:visit(x)
 elif isinstance(v,list):
  for x in v:visit(x)
visit(fixtures)
need(len(fixtures)==13,'existence fixtures')
need(len(flags)==252 and all(x==0 for x in flags),'fixture EOM flags')
model=read('snapshot/modele/sorties/verif_eom_contre_sklearn.json')
need(model['forced']==0,'model pilot forced flags')
z=read('snapshot/modele/sorties/z_monotonie.json')
need(z['sklearn']['pairs']==5400 and z['oracle']['pairs']==3600,'monotonicity reported pairs')
need(z['sklearn']['violations']==z['oracle']['violations']==[],'monotonicity reported violations')
inventory=read('AUDITS_INVENTORY.json')
need(len(inventory['notes'])==inventory['count_md_including_readme']==11,'audit inventory')
need(inventory['scientific_or_exchange_notes_excluding_readme']==10,'notes excluding README')
need(sum(r['classification_proposal']=='history' for r in inventory['notes'])==6,'history classification')
# Contre-calcul exact F2 : k=1 implique core=0 et lien simple euclidien.
# Tous les seuils fermés sont traités ensemble : aucun groupe intermédiaire binaire de même niveau.
positions=(0,1,2,3,6,9,100,101,102,103)
thresholds=sorted({0}|{abs(x-y) for x in positions for y in positions})
universe=set();cuts=[]
for t in thresholds:
 parent=list(range(len(positions)))
 def find(i):
  while parent[i]!=i:
   parent[i]=parent[parent[i]];i=parent[i]
  return i
 for i,x in enumerate(positions):
  for j in range(i):
   if abs(x-positions[j])<=t:
    a,c=find(i),find(j)
    if a!=c:parent[a]=c
 parts={}
 for i,x in enumerate(positions):parts.setdefault(find(i),set()).add(x)
 blocks=sorted((tuple(sorted(s)) for s in parts.values()))
 universe.update(map(frozenset,blocks))
 if not cuts or cuts[-1]['blocks']!=blocks:cuts.append({'threshold':t,'blocks':blocks})
truth=(frozenset((0,1,2,3)),frozenset((6,9)),frozenset((100,101,102,103)))
need(truth[1] not in universe,'F2 phantom incorrectly atomic')
need([c['threshold'] for c in cuts]==[0,1,3,91],'F2 atomic event cuts')
def iou(a,c):return Fraction(len(a&c),len(a|c))
best_all=[max(iou(g,c) for c in universe) for g in truth]
best_big=[max(iou(g,c) for c in universe if len(c)>=2) for g in truth]
mean_all=sum(best_all,Fraction(0))/3
mean_big=sum(best_big,Fraction(0))/3
need(best_all==[Fraction(1),Fraction(1,2),Fraction(1)] and mean_all==Fraction(5,6),'F2 B with singletons')
need(best_big==[Fraction(1),Fraction(1,3),Fraction(1)] and mean_big==Fraction(7,9),'F2 B minsize2')
# Partition officielle EXISTANTE dans ties_out.txt ; le lecteur ne la recalcule pas avec sklearn.
output=(B/'snapshot/equite/ties_out.txt').read_text()
need("{'0123 | 69 | abcd': 3000}" in output,'reported official F2 partition')
perfect=sum((iou(g,g) for g in truth),Fraction(0))/3
need(perfect==1 and perfect>mean_all and perfect>mean_big,'M3 counterexample')
result={'scope':'lecture de sorties privées déjà produites + graphe k1/Fraction exact autonome ; aucun fit ni produit rejoué',
 'heads':first['heads'],'workflow':first['workflow'],
 'implementation_changed':{k:not v for k,v in unchanged.items()},
 'existing_reservations':{'official_epsilon_zero_differences':[520,2400],
   'aggregate_with_fallback':[1015,6752],'fallback_count':1672,
   'hungarian_certificate_code_unchanged':True,'G4_flat_qualification':'aucune nouvelle qualification consultée'},
 'new_model_reported_metadata':{'condensation':cond,'existence_fixtures':len(fixtures),'non_tranche_fields':len(flags),'nonzero_non_tranche':sum(x!=0 for x in flags),'z_monotonicity_pairs_reported':9000,'proof_status':'preuves des propositions non certifiées par ce lecteur'},
 'F2':{'positions':positions,'min_samples':1,'mcs':2,'closed_atomic_event_cuts':cuts,
   'truth':[sorted(g) for g in truth],'reported_sklearn_partition_source':'snapshot/equite/ties_out.txt:28–79, sklearn1.9.1 local, pas rejoué/G4',
   'miou_h_of_reported_partition':str(perfect),'B_atomic_including_singletons':str(mean_all),
   'B_atomic_candidates_size_ge_2':str(mean_big),'conclusion':'M3 ne peut inclure R0 si B utilise la hiérarchie atomisée ; restreindre les bras ou déclarer univers binaire distinct'},
 'audit_files_including_README':11,'history_classification_count':6,'checks':checks}
print(json.dumps(result,ensure_ascii=False,indent=2,sort_keys=True))
