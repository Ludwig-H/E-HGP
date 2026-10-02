from pathlib import Path
import hashlib,json,subprocess,datetime
R=Path(__file__).resolve().parent
WT=Path('/workspaces/E-HGP/build/v11-independent-audit-20261002')
DEV=Path('/workspaces/E-HGP/build/v11-development-20261002')
COMMIT=subprocess.check_output(['git','rev-parse','e6fe34cb0'],cwd=WT,text=True).strip()
paths=subprocess.check_output(['git','ls-tree','-r','--name-only',COMMIT],cwd=WT,text=True).splitlines()
selected=[p for p in paths if p.startswith(('morsehgp3D_v11/src/catalogue/','morsehgp3D_v11/src/cloud/','morsehgp3D_v11/src/core/','morsehgp3D_v11/src/num/','morsehgp3D_v11/tests/catalogue/','morsehgp3D_v11/bench/catalogue/')) or p in ('AGENTS.md','morsehgp3D_v11/docs/ARCHITECTURE.md','morsehgp3D_v11/tests/test.hpp')]
entries=[]
for p in selected:
 b=subprocess.check_output(['git','show',COMMIT+':'+p],cwd=WT)
 target=R/'sources/product_e6'/p
 target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(b)
 entries.append({'group':'product_e6','path':p,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b)})
for p in ['morsehgp3D_v11/docs/CONCEPTION_MOTEUR.md','morsehgp3D_v11/docs/DEVELOPPEMENT.md','morsehgp3D_v11/bench/plans/catalogue_leaf16_g4.json','morsehgp3D_v11/receipts/catalogue_20261002/README.md','morsehgp3D_v11/receipts/catalogue_20261002/catalogue3/catalogue.json','morsehgp3D_v11/receipts/catalogue_20261002/catalogue3/receipt.json','morsehgp3D_v11/receipts/catalogue_20261002/catalogue3/inputs.json','morsehgp3D_v11/receipts/catalogue_20261002/catalogue3/matrix.json','morsehgp3D_v11/receipts/catalogue_20261002/catalogue3/results.tar.gz']:
 b=(DEV/p).read_bytes();h=hashlib.sha256(b).hexdigest()
 target=R/'sources/development_live'/p
 target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(b)
 again=(DEV/p).read_bytes()
 if hashlib.sha256(again).hexdigest()!=h:raise RuntimeError('source changed during capture: '+p)
 entries.append({'group':'development_live','path':p,'sha256':h,'size':len(b),'double_read_stable':True})
manifest={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'product_commit':COMMIT,'development_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=DEV,text=True).strip(),'product_head_at_capture':subprocess.check_output(['git','rev-parse','HEAD'],cwd=WT,text=True).strip(),'f391_e6_src_diff':subprocess.check_output(['git','diff','--name-only','f391bf13e',COMMIT,'--','morsehgp3D_v11/src'],cwd=WT,text=True).splitlines(),'entries':entries,'scope':'static review; no build, product test, GCP, or allocation campaign'}
(R/'SOURCE_BEFORE.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'captured_files':len(entries),'product_commit':COMMIT,'development_head':manifest['development_head'],'f391_e6_src_diff':manifest['f391_e6_src_diff']}))
