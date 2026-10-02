from pathlib import Path
import json,sys,platform
R=Path(__file__).resolve().parent
num=R/'sources/morsehgp3D_v11'
mut=json.loads((num/'tests/mutants/num.json').read_text());mutants=[]
for item in mut['mutants']:
 count=(num/item['fichier']).read_text().count(item['cherche'])
 if count!=1:raise RuntimeError('nonunique mutation pattern '+item['id'])
 mutants.append({'id':item['id'],'pattern_occurrences':count,'gate':item['porte'],'options':item.get('options',[])})
matrix=json.loads((num/'tools/g4_matrix.json').read_text())
configs=[{'name':c['name'],'cmake_options':c['cmake_options']} for c in matrix['configurations']]
branches=[{'coord_bits':b,'side_bits':6*b+8,'native_presentation_arities':[q for q in range(1,5) if 6*b+8<=127 or q!=3],'wide_presentation_arities':[q for q in range(1,5) if not (6*b+8<=127 or q!=3)]} for b in (18,21,24)]
report={'branches':branches,'mutant_floor':mut['plancher'],'mutants':mutants,'configs':configs,'scope':'Static source/manifest inspection only; no gate, mutant, native executable, or GCP run.'}
(R/'STATIC_INSPECTION.json').write_text(json.dumps(report,indent=2)+'\n')
(R/'ENVIRONMENT.json').write_text(json.dumps({'python':sys.version,'platform':platform.platform(),'scope':'Read-only Python receipt tooling'},indent=2)+'\n')
print(json.dumps({'branch_table':branches,'mutants':len(mutants),'floor':mut['plancher'],'patterns_unique':True},sort_keys=True))
