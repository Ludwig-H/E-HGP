#!/usr/bin/env python3
"""Source-only comparison of adopted B3-K with the measured keys arm."""
import hashlib,io,json,subprocess,sys,tarfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE='aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66'
MEASURED='81b0883d11df14e4b63d82e85b6539d6abc7bf92'
PRODUCT='2aaed1847e63ff86550db1fb1ba6e36313aa58bc'
PREFIX='morsehgp3D_v12/'
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def run(repo):
 def git(*args):return subprocess.check_output(['git',*args],cwd=repo)
 def read(pin,path):return git('show',pin+':'+PREFIX+path)
 def sources(pin):
  with tarfile.open(fileobj=io.BytesIO(git('archive',pin,PREFIX+'src'))) as t:
   return {m.name[len(PREFIX):]:t.extractfile(m).read()for m in t if m.isfile()}
 specification=read(MEASURED,'microbancs/mes_t2d_b3/bras_t2d_b3.json')
 spec=json.loads(specification)['bras']['cles']['fichiers']
 expected=sources(BASE);before=dict(expected);n=0
 for name,rules in spec.items():
  need(sha(expected[name])==rules['sha256_avant'],'preimage '+name)
  body=expected[name].decode()
  for s in rules['substitutions']:
   need(body.count(s['cherche'])==1,'unique substitution '+name)
   body=body.replace(s['cherche'],s['remplace'],1);n+=1
  expected[name]=body.encode()
  need(sha(expected[name])==rules['sha256_apres'],'postimage '+name)
 actual=sources(PRODUCT)
 need(actual==expected,'all product source bytes equal measured keys arm')
 need(actual['src/tower/resolve.cpp']==before['src/tower/resolve.cpp'],'scan removed exactly')
 manifest=json.loads(read(MEASURED,'tests/mutants/tower.json'))
 need(manifest['plancher']==74,'measured floor')
 manifest['plancher']=73
 rows=manifest['mutants'];manifest['mutants']=[m for m in rows if m['id']!='balayage_dernier_oublie']
 need(len(rows)-len(manifest['mutants'])==1,'one scan mutant removed')
 need(manifest==json.loads(read(PRODUCT,'tests/mutants/tower.json')),'product mutant manifest')
 need(read(PRODUCT,'tests/mutants/catalogue.json')==read(MEASURED,'tests/mutants/catalogue.json'),'catalogue manifest unchanged')
 inventory=''.join(sha(actual[name])+'  '+name+'\n'for name in sorted(actual)).encode()
 return dict(product=git('rev-parse',PRODUCT).decode().strip(),base=BASE,measured=MEASURED,source_files=len(actual),changed_source_files=sorted(name for name in actual if actual[name]!=before[name]),substitutions=n,product_inventory_sha256=sha(inventory),arm_specification_sha256=sha(specification),tower_mutant_floor=73,native_runs=0)
if __name__=='__main__':
 need(len(sys.argv)in(2,3),'check.py REPO [--record]')
 r=run(sys.argv[1])
 if len(sys.argv)==3:
  need(sys.argv[2]=='--record','record flag');(HERE/'capture.json').write_text(json.dumps(r,indent=2)+'\n')
 else:need(r==json.loads((HERE/'capture.json').read_text()),'capture equality')
 print(json.dumps(r))
