"""Hash-first portable Fraction proof reader, no native/GCP."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parent
FILES={'README.md','PROTOCOL.txt','check.py','receipt.json','normal.stdout','normal.stderr','optimized.stdout','optimized.stderr','verify.py'}
PIN='f56b11a3bf4609ed835be9e2ec17d5ebfc02ac1648a231baa63579878b7830a1'

def require(ok,message):
    if not ok: raise ValueError(message)

def manifest():
    pins={}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        value,name=line.split('  ',1)
        require(name in FILES and name not in pins and len(value)==64 and all(c in '0123456789abcdef' for c in value),'manifest')
        pins[name]=value
    require(set(pins)==FILES and {str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}==FILES|{'SHA256SUMS'},'inventory')
    require(all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==v for n,v in pins.items()),'hash')
    require(pins['check.py']==PIN,'fixed source')
    return pins

def main():
    before=manifest()
    r=json.loads((ROOT/'receipt.json').read_text())
    require(r['schema']=='mhgp10.parts_meb_count.v1' and r['source_before']==r['source_after']==PIN and r['native_calls']==0 and r['GCP_used'] is False,'receipt')
    require(len(r['commands'])==2,'commands')
    for i,c in enumerate(r['commands']):
        mode='optimized' if i else 'normal'
        require(c['argv']==['/home/codespace/.python/current/bin/python3','-B']+(['-O'] if i else [])+['check.py'],'historical argv')
        require(c['code']==0 and c['stderr']=='' and c['end_ns']>=c['start_ns']>0 and c['end_utc']>=c['start_utc'],'capture')
        require((ROOT/(mode+'.stdout')).read_text()==c['stdout'] and (ROOT/(mode+'.stderr')).read_text()==c['stderr'],'logs')
        d=json.loads(c['stdout'])
        require(d['status']=='MEB_CLASS_FORMULAS_AND_FULL_VOTE_GAP_PROVED' and d['formula_checks']==113 and len(d['class_fixtures'])==4,'proof inventory')
        require(d['missing_vote']['mass_full']==3 and d['missing_vote']['mass_from_bounded_catalogue']==2 and d['missing_vote']['missing_part']==[0,3] and d['native_calls']==0 and d['GCP_used'] is False,'scope')
        replay=subprocess.run([sys.executable,'-B']+(['-O'] if i else [])+[str(ROOT/'check.py')],cwd=ROOT,capture_output=True,text=True,check=False,timeout=10)
        require(replay.returncode==0 and replay.stderr=='' and replay.stdout==c['stdout'],'replay')
    require(r['commands'][0]['stdout']==r['commands'][1]['stdout'] and manifest()==before,'closure')
    print(json.dumps({'status':'MEB_CLASS_COUNT_ARCHIVE_PASS','files':9,'formula_checks':113,'classes':4,'missing_votes':1,'replays':2,'native_calls':0,'GCP_used':False},sort_keys=True))

if __name__=='__main__': main()
