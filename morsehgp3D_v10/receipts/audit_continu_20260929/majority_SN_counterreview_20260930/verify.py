"""Hash-first portable four-site N membership counterexample."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parent
FILES={'README.md','PROTOCOL.txt','check.py','receipt.json','normal.stdout','normal.stderr','optimized.stdout','optimized.stderr','verify.py'}
PIN='ee2718a2e6c68b818186a1669669c636b9ddea5694e9b29e79661cbbf9c73d3c'

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
    require(pins['check.py']==PIN,'source identity')
    return pins

def main():
    pins=manifest()
    r=json.loads((ROOT/'receipt.json').read_text())
    require(r['schema']=='mhgp10.majority_SN.v1' and r['source_before']==r['source_after']==PIN
            and r['native_calls']==0 and r['GCP_used'] is False and len(r['commands'])==2,'receipt')
    for i,c in enumerate(r['commands']):
        mode='optimized' if i else 'normal'
        require(c['argv']==['/home/codespace/.python/current/bin/python3','-B']+(['-O'] if i else [])+['check.py']
                and c['code']==0 and c['stderr']=='' and c['end_ns']>=c['start_ns']>0
                and c['start_utc']<=c['end_utc'],'command')
        require((ROOT/(mode+'.stdout')).read_text()==c['stdout'] and (ROOT/(mode+'.stderr')).read_text()==c['stderr'],'logs')
        d=json.loads(c['stdout'])
        require(d['status']=='MM_D_GAMMA_G1_MEMBERSHIP_COUNTEREXAMPLE' and d['K']==2 and len(d['points'])==4
                and d['gamma']=='1/50' and d['kappa']=='25' and d['MM_date_radius']=='202/165'
                and d['fusion_radius2']=='12101/2525' and d['actual_date_later_than_deadline'] is True
                and len(d['all_pair_births'])==6 and len(d['all_triple_events'])==4
                and d['native_calls']==0 and d['GCP_used'] is False,'result')
        replay=subprocess.run([sys.executable,'-B']+(['-O'] if i else [])+[str(ROOT/'check.py')],cwd=ROOT,text=True,capture_output=True,check=False,timeout=10)
        require(replay.returncode==0 and replay.stderr=='' and replay.stdout==c['stdout'],'replay')
    require(r['commands'][0]['stdout']==r['commands'][1]['stdout'] and manifest()==pins,'closure')
    print(json.dumps(dict(status='MM_G1_MEMBERSHIP_ARCHIVE_PASS',files=9,cases=1,replays=2,native_calls=0,GCP_used=False),sort_keys=True))

if __name__=='__main__':main()
