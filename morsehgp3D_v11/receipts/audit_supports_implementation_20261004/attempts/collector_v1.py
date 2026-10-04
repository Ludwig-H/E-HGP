from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT=Path(__file__).resolve().parent
def need(ok,message):
    if not ok:raise SystemExit(message)
def inventory(folder):
    expected={}
    for line in (folder/'SHA256SUMS').read_text().splitlines():
        digest,name=line.split('  ',1)
        rel=Path(name)
        need(not rel.is_absolute() and '..' not in rel.parts,'invalid inventory path')
        need(name not in expected,'duplicate inventory path')
        expected[name]=digest
    actual={}
    for f in sorted(folder.rglob('*')):
        need(not f.is_symlink(),'symlink payload')
        if f.is_file() and f!=folder/'SHA256SUMS':
            actual[str(f.relative_to(folder))]=hashlib.sha256(f.read_bytes()).hexdigest()
    need(actual==expected,'inventory mismatch: '+folder.name)
inventory(ROOT)
source=json.loads((ROOT/'SOURCE.json').read_text())
jobs=[('tower','check_contract.py','normal.json','optimized.json',462),
      ('qb','check_shape.py','normal.json','optimized.json',529),
      ('evidence','check_d2_signature.py','normal.json','optimized.json',36),
      ('api','check.py','normal.json','optimized.json',92)]
counts={}
for name,script,normal,optimized,count in jobs:
    inventory(ROOT/name)
    need(hashlib.sha256((ROOT/name/'SHA256SUMS').read_bytes()).hexdigest()==source['child_inventory_sha256'][name],
         'child closure pin')
    outputs=[]
    for flags,saved in (([],normal),(['-O'],optimized)):
        result=subprocess.run([sys.executable,'-B','-S']+flags+[script],cwd=ROOT/name,
                              capture_output=True,timeout=45)
        need(result.returncode==0 and not result.stderr,'child refused: '+name)
        need(result.stdout==(ROOT/name/saved).read_bytes(),'child output drift: '+name)
        need(json.loads(result.stdout)['checks']==count,'child count mismatch')
        outputs.append(result.stdout)
    need(outputs[0]==outputs[1],'normal/optimized mismatch')
    counts[name]=count
report={'status':'PASS','child_guards':counts,'total_child_guards':sum(counts.values()),
        'normal_optimized_identical':True,'native_executed':False,
        'scope':'bounded WIP source review and stdlib/Fraction models; no native or G4 qualification'}
need(report==json.loads((ROOT/'RESULTS.json').read_text()),'parent result mismatch')
print(json.dumps(report,sort_keys=True,indent=2))
