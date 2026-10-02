from pathlib import Path
import json,sys,platform
from model import models
R=Path(__file__).resolve().parent
(R/'MODEL.json').write_text(json.dumps(models(),indent=2,sort_keys=True)+'\n')
rows=[]
for cap in ['catalogue2','catalogue3']:
 d=json.loads((R/'sources/development_live/morsehgp3D_v11/receipts/catalogue_20261002'/cap/'catalogue.json').read_text())
 for row in d['runs']:
  if row['status']=='ok' and row['case']=='uniform_u18_n8000' and row['kmax']==5:
   e=next(v for v in row['events'] if v['phase']=='catalogue')
   rows.append({'capture':cap,'leaf_size':d.get('leaf_size',32),'repeat':row['repetition'],'seconds':e['wall_ns']/1e9,'balls':e['balls'],'levels':e['levels'],'incidences':e['incidences'],'peak_reserved_bytes':e['peak_reserved_bytes'],'canonical_sha256_declared':row['canonical_sha256'],'logical_one_pass':e['logical'],'generation_passes':e['generation_passes']})
(R/'ABLATION.json').write_text(json.dumps({'rows':rows,'scope':'Read published structured outputs only; no native run, no recompilation, no rehash of canonical files removed on VM, no new performance qualification.'},indent=2,sort_keys=True)+'\n')
(R/'ENVIRONMENT.json').write_text(json.dumps({'python':sys.version,'platform':platform.platform(),'scope':'Python receipt and analytic models only'},indent=2)+'\n')
print(json.dumps({'model':'MODEL.json','ablation_rows':len(rows),'ablation':rows},sort_keys=True))
