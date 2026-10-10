#!/usr/bin/env python3
"""LEM-T1 : equivalence de l'inclusion avec recherche de F prive du support."""
import argparse,difflib,hashlib,itertools,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent

def need(ok,why):
 if not ok:raise ValueError(why)
def check_population(f,inner,shell,skip=()):
 visits=[];calls=0;next_support=0
 for x in f:
  if next_support<len(skip)and x==skip[next_support]:next_support+=1;continue
  visits.append(x);calls+=1
  if x in inner:continue
  calls+=1
  if x not in shell:return False,visits,calls,x
 return True,visits,calls,None

def lem(f,s,table,optimized):
 miss=False
 if len(s)<2 or len(s)>4 or tuple(sorted(set(s)))!=s or not set(s)<=set(f):return None,miss
 found=table.get(s)
 if found is None:return None,True
 inner,shell=found
 return (s if check_population(f,inner,shell,s if optimized else ())[0]else None),miss

def run(repo):
 cap=json.loads((HERE/'capture.json').read_text());sources={}
 for p,h in cap['sources']:
  data=subprocess.check_output(['git','show',cap['base_git']+':'+p],cwd=repo);need(hashlib.sha256(data).hexdigest()==h,'source pin');sources[p]=data.decode()
 path=cap['sources'][0][0];before=sources[path];after=before
 for x,y in cap['replacements']:need(after.count(x)==1,'unique replacement');after=after.replace(x,y)
 need(hashlib.sha256(after.encode()).hexdigest()==cap['patched_source_sha256'],'postimage')
 patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+path,tofile='b/'+path));need(patch==(HERE/'proposition.patch').read_text(),'patch exact')
 need('if (support.size() < 2 || support.size() > 4 || !sorted_subset(support, f)) return std::nullopt;'in after,'public guard retained')
 need('std::span<const u32>(cert->support.data(), cert->arity)'in after,'post-certificate support')
 # Six identifiers : S always in U; each other site chooses F membership and I/U/outside independently.
 count=success=failed=zero=lookups_saved=0;U=tuple(range(6))
 for q in (2,3,4):
  for s in itertools.combinations(U,q):
   rest=[x for x in U if x not in s]
   for states in itertools.product(range(6),repeat=len(rest)):
    inner=[];shell=list(s);f=list(s)
    for x,state in zip(rest,states):
     if state%2:f.append(x)
     if state//2==0:inner.append(x)
     if state//2==1:shell.append(x)
    f=tuple(sorted(f));inner=tuple(inner);shell=tuple(sorted(shell));a=check_population(f,inner,shell);b=check_population(f,inner,shell,s)
    need(a[0]==b[0]and a[3]==b[3],'same decision and first failure');need([x for x in a[1]if x not in s]==b[1],'same remaining query order');need(a[2]-b[2]==2*sum(x in s for x in a[1]),'two membership calls per skipped shell site');need(lem(f,s,{s:(inner,shell)},False)==lem(f,s,{s:(inner,shell)},True),'same lem result/miss')
    count+=1;success+=a[0];failed+=not a[0];lookups_saved+=a[2]-b[2]
    if len(f)==q:need(b[2]==0,'F=S needs no query');zero+=1
 # Public malformed supports still refuse before any table success.
 table={(0,2):((1,),(0,2,4))};f=(0,1,2,4);invalid=[(),(0,),(0,0),(2,0),(0,3),(0,1,2,3,4)]
 for s in invalid:need(lem(f,s,table,False)==lem(f,s,table,True)==(None,False),'invalid support guard')
 need(lem(f,(1,4),table,False)==lem(f,(1,4),table,True)==(None,True),'table miss contract')
 need(lem((0,1),(0,2),table,False)==lem((0,1),(0,2),table,True)==(None,False),'found support outside F still refused')
 need(lem((0,2,5),(0,2),table,True)==(None,False),'non-support outsider still refused')
 # Broken Catalogue deliberately outside theorem: dropping a missing support site would hide corruption.
 broken={(0,2):((),(0,))};need(lem((0,2),(0,2),broken,False)!=(lem((0,2),(0,2),broken,True)),'catalogue invariant essential')
 # Explicit bit-pattern examples used by native test proposal, not geometrical qualification.
 examples=[{'F':[0,1,2,4],'S':[0,2],'I':[1],'U':[0,2,4]}, {'F':[0,2,5],'S':[0,2],'I':[1],'U':[0,2,4]}, {'F':[0,1,2],'S':[0,1,2],'I':[],'U':[0,1,2]}, {'F':[0,1,2,3],'S':[0,1,2,3],'I':[],'U':[0,1,2,3]}]
 for e in examples:
  a=check_population(e['F'],e['I'],e['U']);b=check_population(e['F'],e['I'],e['U'],e['S']);e['result']=a[0];e['before_calls']=a[2];e['after_calls']=b[2]
 return {'cases':count,'success':success,'refused':failed,'F_equals_S_cases':zero,'membership_calls_saved_in_model':lookups_saved,'public_invalid_support_cases':len(invalid),'table_miss_preserved':True,'non_support_outside_refused':True,'invalid_catalogue_not_covered':True,'examples':examples,'native_execution':False,'time_gain_claimed':False}
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--repo',type=Path,required=True);a.add_argument('--write',action='store_true');x=a.parse_args();r=run(x.repo)
 if x.write:(HERE/'results.json').write_text(json.dumps(r,separators=(',',':'))+'\n')
 else:need(r==json.loads((HERE/'results.json').read_text()),'results changed')
 print(json.dumps({'status':'ok',**{k:r[k]for k in ('cases','success','refused','F_equals_S_cases','membership_calls_saved_in_model')}}))
