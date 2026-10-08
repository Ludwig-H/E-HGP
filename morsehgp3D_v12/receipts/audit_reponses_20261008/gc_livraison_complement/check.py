#!/usr/bin/env python3
import argparse,hashlib,json,re,subprocess,tempfile
from pathlib import Path
H=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,required=True);p.add_argument('--logs',type=Path,required=True);a=p.parse_args();m=json.loads((H/'pins.json').read_text())
def need(b):
    if not b:raise RuntimeError('preuve modifiee')
def sha(b):return hashlib.sha256(b).hexdigest()
b=subprocess.check_output(['git','show',m['base']+':'+m['source']],cwd=a.repo);need(sha(b)==m['source_sha256'])
need(sha((a.repo/m['harness']['path']).read_bytes())==m['harness']['sha256'])
with tempfile.TemporaryDirectory() as td:
    f=Path(td)/m['source'];f.parent.mkdir(parents=True);f.write_bytes(b)
    for patch in m['patches']:
        x=a.repo/patch['path'];need(sha(x.read_bytes())==patch['sha256']);subprocess.run(['git','apply',str(x.resolve())],cwd=td,check=True,capture_output=True)
    need(sha(f.read_bytes())==m['combined_sha256'])
texts={}
for n,meta in m['artifacts'].items():
    b=(a.logs/n).read_bytes();need(len(b)==meta['bytes'] and sha(b)==meta['sha256']);texts[n]=b.decode()
rows=re.findall(r'^\s*\d+/701 Test\s+#\d+:\s+(\S+)\s+\.+(.*)$',texts['build_v12_u21.ctest_gc2.log'],re.M)
need(len(rows)==701 and sum('Passed' in x for _,x in rows)==700)
need([(n,'Skipped' in x) for n,x in rows if 'Passed' not in x]==[(m['ctest']['skipped_name'],True)])
print('complement_ok: composition D6 ; journal local 700 Passed +1 Skipped/701 ; aucun test rejoue')
