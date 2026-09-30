"""Hash-first Gamma/CDF proof reader plus exact Euler1D control, no native/GCP."""
from fractions import Fraction as F
from itertools import combinations
from math import comb
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parent
FILES={'README.md','PROTOCOL.txt','check.py','receipt.json','normal.stdout','normal.stderr','optimized.stdout','optimized.stderr','verify.py'}
PIN='d8a0c7fd1e8bd15d35d72f46e9b2bb9adc3b108e2240626dd9e19141c166488c'

def require(ok,why):
    if not ok: raise ValueError(why)

def manifest():
    pins={}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        h,n=line.split('  ',1)
        require(n in FILES and n not in pins and len(h)==64 and all(c in '0123456789abcdef' for c in h),'manifest')
        pins[n]=h
    require(set(pins)==FILES and {str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}==FILES|{'SHA256SUMS'},'inventory')
    require(all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in pins.items()),'hash')
    require(pins['check.py']==PIN,'proof identity')
    return pins

def euler1d(case,snapshot):
    z=list(map(F,case['coordinates']))
    K=case['K']; r=F(snapshot['radius']); t=min(r,F(case['fixed_band_upper_radius']))
    vertices={tuple(S) for S in snapshot['active_vertices']}
    endpoints=sorted({-t,t}|{c+s*t for c in z for s in (-1,1) if -t<=c+s*t<=t})
    def weight(c):
        Q=[i for i,v in enumerate(z) if abs(c-v)<=t]
        require(0 in Q,'center outside query ball')
        if len(Q)<K: return 0
        # Every K-partie covered by this center is already in the unique
        # component; all K+1 cofaces fit t<=r, even at equality.
        require(all(S in vertices for S in combinations(Q,K)),'owner/admission mismatch')
        return comb(len(Q)-1,K-1)
    point_sum=sum(weight(c) for c in endpoints)
    edge_sum=sum(weight((a+b)/2) for a,b in zip(endpoints,endpoints[1:]))
    require(point_sum-edge_sum==snapshot['actual_point0_mass'],'Euler count mismatch')
    return dict(K=K,radius=str(r),points=point_sum,open_edges=edge_sum,mass=point_sum-edge_sum)

def main():
    pins=manifest()
    r=json.loads((ROOT/'receipt.json').read_text())
    require(r['schema']=='mhgp10.component_cdf.v1' and r['source_before']==r['source_after']==PIN
            and r['native_calls']==0 and r['GCP_used'] is False and len(r['commands'])==2,'receipt')
    eulers=None
    for i,c in enumerate(r['commands']):
        mode='optimized' if i else 'normal'
        require(c['argv']==['/home/codespace/.python/current/bin/python3','-B']+(['-O'] if i else [])+['check.py']
                and c['code']==0 and c['stderr']=='' and c['end_ns']>=c['start_ns']>0
                and c['start_utc']<=c['end_utc'],'command')
        require((ROOT/(mode+'.stdout')).read_text()==c['stdout'] and (ROOT/(mode+'.stderr')).read_text()==c['stderr'],'logs')
        d=json.loads(c['stdout'])
        require(d['status']=='COMPONENT_COVER_IS_NOT_BALLOT_CDF_PROVED' and [a['K'] for a in d['cases']]==[2,5]
                and d['native_calls']==0 and d['GCP_used'] is False,'case inventory')
        require([[s['actual_point0_mass'] for s in a['snapshots']] for a in d['cases']]==[[2,3],[5,15]],'mass inventory')
        eulers=[euler1d(a,s) for a in d['cases'] for s in a['snapshots']]
        replay=subprocess.run([sys.executable,'-B']+(['-O'] if i else [])+[str(ROOT/'check.py')],cwd=ROOT,text=True,capture_output=True,check=False,timeout=10)
        require(replay.returncode==0 and replay.stderr=='' and replay.stdout==c['stdout'],'replay')
    require(r['commands'][0]['stdout']==r['commands'][1]['stdout'] and manifest()==pins,'closure')
    print(json.dumps(dict(status='COMPONENT_CDF_ARCHIVE_PASS',files=9,cases=2,Euler1D=eulers,replays=2,native_calls=0,GCP_used=False),sort_keys=True))

if __name__=='__main__': main()
