#!/usr/bin/env python3
"""R1 : lecture des primaires existants, jamais lancement du moteur ou des mutants."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,os,re,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
PIN='47feedc96ee4b4cb85b5a7c590f38c874bb04855'
P='morsehgp3D_v12/'
COPIED=('CMakeLists.txt','cmake','src','cli','bench','tests','tools','reference','docs')
def need(x,why):
 if not x:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def pin(b):return dict(bytes=len(b),sha256=sha(b))
def git(repo,path):return subprocess.check_output(['git','-C',str(repo),'show',PIN+':'+P+path])
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--session',type=Path,required=True);p.add_argument('--write',action='store_true');a=p.parse_args()
 root=a.snapshot/'source';h=hashlib.sha256();count=0
 for name in COPIED:
  origin=root/name;paths=[origin] if origin.is_file() else []
  for folder,dirs,files in os.walk(origin):
   dirs[:]=sorted(x for x in dirs if x!='__pycache__');paths += [Path(folder)/x for x in sorted(files) if not x.endswith('.pyc')]
  for path in sorted(paths):h.update(str(path.relative_to(root)).encode()+b'\0');h.update(sha(path.read_bytes()).encode()+b'\n');count+=1
 report_raw=(a.snapshot/'mut_r1.json').read_bytes();report=json.loads(report_raw);manifest=(root/'tests/mutants/tower.json').read_bytes();local=json.loads(manifest)
 need(report['schema']=='mhgp12.mutants.v1' and report['code']==0 and report['temoin']=='vert' and report['plancher']==15 and len(report['mutants'])==15,'local campaign closure')
 need(report['sources_sha256']==h.hexdigest() and report['manifeste_sha256']==sha(manifest) and count==405,'exact historical source tree')
 shared=['src/tower/registry_branches.cpp','tests/tower/registry_unit.cpp','tests/tower/forest_support.hpp','tests/mutants/run_mutants.py','tests/support/mhgp12_gate.py','tests/support/test.hpp','cmake/gates.cmake','cmake/run_expect.cmake']
 for n in shared:need((root/n).read_bytes()==git(a.repo,n),'shared Git bytes '+n)
 cmake=(root/'tests/tower/tests.cmake').read_bytes();reg=re.compile(rb'mhgp12_add_unit\(mhgp12_tower_registry.*?\)',re.S)
 need(reg.search(cmake).group()==reg.search(git(a.repo,'tests/tower/tests.cmake')).group(),'registry gate declaration')
 final=json.loads(git(a.repo,'tests/mutants/tower.json'));mutants={m['id']:m for m in local['mutants']};expected={m['id']:m for m in final['mutants'] if m['id'].startswith('r1_')};need(len(expected)==6,'six R1')
 records=[]
 for name,m in expected.items():
  need(mutants[name]==m,'identical R1 edit and gate '+name)
  rows=[x for x in report['mutants'] if x['id']==name];need(rows==[dict(id=name,juge=m['porte'],verdict='TUE',detail='code')],'individual code cause '+name)
  text=(root/m['fichier']).read_text();need(text.count(m['cherche'])==1 and m['remplace']!=m['cherche'],'unique nonempty mutation')
  records.append(dict(id=name,gate=m['porte'],cause='code',edit_sha256=sha(json.dumps(m,sort_keys=True,ensure_ascii=False).encode())))
 log=(a.snapshot/'mutants_r1.log').read_bytes().decode()
 need('mutants_ok module=tower mutants=15 tues=15 dont_signal=0 dont_delai=0 dont_construction=0 plancher=15' in log,'log closure')
 need(all(re.search(r'^'+re.escape(m['id'])+r'\s+TUE\s+code$',log,re.M) for m in records),'log per mutant')
 # Primary G4 evidence is deliberately distinct from the local six-mutant report.
 old=json.loads((a.repo/P/'receipts/audit_reponses_20261008/session_r1_admission/capture.json').read_text())
 archive=(a.session/'results/results.tar.gz').read_bytes();need(pin(archive)==old['files']['results/results.tar.gz'],'closed R1 archive')
 helper_path=a.repo/P/'receipts/audit_reponses_20261008/session_l1_recuperation/check.py';spec=importlib.util.spec_from_file_location('archive_reader',helper_path);helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
 fs=helper.files(archive);helper.manifest(fs,289)
 log_socle=fs['results/cmd/000_socle_ctest/stdout'];groups=['fixtures','exhaustif','aleatoire','grand','etapes','inventaire'];selected=[]
 for group in groups:
  rows=re.findall(rb'^.*Test\s+#\d+:\s+mhgp12_tower_registry_'+group.encode()+rb'\s+\.+\s+Passed\s+.*$',log_socle,re.M);need(len(rows)==1,'G4 gate '+group);selected.append(rows[0].decode().strip())
 gate=fs['results/cmd/003_mutants_tour/stdout'];need(re.search(rb'Test\s+#\d+:\s+mhgp12_mutants_tower\s+\.+\s+Passed',gate),'G4 mutants launcher')
 result=dict(git=PIN,local_source=dict(files=count,tree_sha256=h.hexdigest()),local_manifest=pin(manifest),local_report=pin(report_raw),local_log=pin((a.snapshot/'mutants_r1.log').read_bytes()),shared_sources={n:pin((root/n).read_bytes()) for n in shared},registry_cmake_block=pin(reg.search(cmake).group()),local_6=records,individual_assertions_archived=False,local_ELF_archived=False,G4=dict(archive=pin(archive),socle=pin(log_socle),registry_gates=selected,mutants_launcher=pin(gate),individual_mutant_reports_archived=False),source_scope='prototype global distinct de Git47 ; R/oracle/juge et six entrees exacts')
 f=HERE/'capture.json'
 if a.write:f.write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n')
 else:need(result==json.loads(f.read_text()),'capture stable')
 print(json.dumps(dict(local_R1_code=len(records),G4_registry_gates=len(selected),native_executions_by_audit=0)))
if __name__=='__main__':main()
