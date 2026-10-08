#!/usr/bin/env python3
"""T2dB3: source/protocol metadata only; no engine, controller or input payload."""
from pathlib import Path
import argparse,hashlib,io,json,subprocess,tarfile
HERE=Path(__file__).resolve().parent
PREFIX='morsehgp3D_v12/'
SCOPES=[PREFIX+x for x in ['src','tests','bench','microbancs','cmake','CMakeLists.txt']]+['gcp-migration/v12_worker.sh']
def need(x,w):
 if not x:raise ValueError(w)
def sha(b):return hashlib.sha256(b).hexdigest()
def pin(b):return dict(bytes=len(b),sha256=sha(b))
def selected(p):return any(p==x or p.startswith(x+'/')for x in SCOPES)
def archive(raw):
 out={}
 with tarfile.open(fileobj=io.BytesIO(raw))as t:
  for m in t:
   p=Path(m.name);need(not p.is_absolute()and'..'not in p.parts and not m.issym()and not m.islnk(),'unsafe source member')
   if m.isfile()and selected(m.name):need(m.name not in out,'duplicate member');out[m.name]=t.extractfile(m).read()
 return out
def inventory(fs):return sha(''.join(sha(b)+'  '+n+'\n'for n,b in sorted(fs.items())).encode())
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--repo',required=True,type=Path);ap.add_argument('--session',required=True,type=Path);ap.add_argument('--before-archive',required=True,type=Path);a=ap.parse_args();cap=json.loads((HERE/'protocol.json').read_text())
 pre_raw=(a.session/'preflight.json').read_bytes();plan_raw=(a.session/'package/plan.json').read_bytes();package_raw=(a.session/'package/package.tar.gz').read_bytes();before_raw=a.before_archive.read_bytes()
 for name,raw in [('preflight',pre_raw),('plan',plan_raw),('package',package_raw),('before_archive',before_raw)]:need(pin(raw)==cap['pins'][name],'pin '+name)
 pre,plan=json.loads(pre_raw),json.loads(plan_raw);need(pre['commit']==cap['source_git']and pre['source_kind']=='commit','source declaration');need(pre['package_sha256']==sha(package_raw)and pre['plan_sha256']==sha(plan_raw),'package/plan declaration')
 for which,raw,commit in [('candidate',package_raw,cap['source_git']),('before',before_raw,cap['before_git'])]:
  fs=archive(raw);git_raw=subprocess.check_output(['git','-C',str(a.repo),'archive',commit,*(SCOPES if which=='candidate'else SCOPES[:-1])]);need(fs==archive(git_raw),'source package versus Git '+which);need(dict(files=len(fs),inventory_sha256=inventory(fs))==cap['sources'][which],'source inventory '+which)
  if which=='candidate':candidate=fs
  else:before=fs
 pilot=candidate[PREFIX+'microbancs/mes_t2d_b3/pilote_t2d_b3.py'];arms_raw=candidate[PREFIX+'microbancs/mes_t2d_b3/bras_t2d_b3.json'];need(pin(pilot)==cap['pins']['pilot']and pin(arms_raw)==cap['pins']['arms'],'embedded pilot/arms')
 need(sha(pilot)=='6a798b312bfb59e14bfc65df209b57ab13ed2bd6fd247ce7d530d4d2953c60f2','original vulnerable judge, not proposed patch')
 arms=json.loads(arms_raw);need(arms['base']=='8a0716e74','arm base')
 reconstructed={}
 for name,arm in arms['bras'].items():
  product={p:b for p,b in before.items()if p.startswith(PREFIX+'src/')}
  for rel,change in arm['fichiers'].items():
   p=PREFIX+rel;need(sha(product[p])==change['sha256_avant'],'arm preimage');s=product[p].decode()
   for replace in change['substitutions']:
    need(s.count(replace['cherche'])==1,'unique substitution');s=s.replace(replace['cherche'],replace['remplace'],1)
   product[p]=s.encode();need(sha(product[p])==change['sha256_apres'],'arm postimage')
  reconstructed[name]=dict(changed_files=len(arm['fichiers']),product_inventory_sha256=inventory(product))
  if name=='apres':need(product=={p:b for p,b in candidate.items()if p.startswith(PREFIX+'src/')},'apres equals packaged product')
 need(reconstructed==cap['reconstructed_arms'],'arm inventories')
 allowed={'--fils','--passes','--processus','--jobs','--delai'};commands=[]
 for c in plan['commands']:
  args=c['argv'];commands.append(dict(name=c['name'],timeout_seconds=c['timeout_seconds'],options={x:args[i+1]for i,x in enumerate(args[:-1])if x in allowed}))
 need(commands==cap['commands'],'public command projection');args=plan['commands'][1]['argv'];need('tout'in args and '--essai'not in args,'complete decisive run requested')
 need(args[args.index('--avant-sha256')+1]==sha(before_raw),'commanded baseline SHA')
 baseline_name=Path(args[args.index('--avant-archive')+1]).name;meta={d['name']:d for d in pre['data_files']};need(meta[baseline_name]['sha256']==sha(before_raw)and meta[baseline_name]['size']==len(before_raw),'baseline input metadata')
 need(not any(x in args for x in ['--sequentiel','--cache']),'default product route/cache')
 now=dict(done=(a.session/'DONE').is_file(),receipt=(a.session/'receipt.json').is_file(),results_archive=(a.session/'results/results.tar.gz').is_file())
 result=dict(source_git=cap['source_git'],before_git=cap['before_git'],sources=cap['sources'],pilot_sha256=sha(pilot),known_identity_admission_gap=True,proposed_patch_embarked=False,commands=commands,cohorts=cap['cohorts'],results_admitted=False,native_executed=False)
 need(result==json.loads((HERE/'protocol_results.json').read_text()),'result projection');print(json.dumps(dict(protocol_verified=True,results_admitted=False,local_presence_now=now,pilot_sha256=sha(pilot),native_executed=False)))
if __name__=='__main__':main()
