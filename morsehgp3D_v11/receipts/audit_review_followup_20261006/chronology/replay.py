"""Pinned real read_points(exact=True), stdlib only; no native/geometry execution."""
import argparse,hashlib,json,struct,subprocess,sys,types
from pathlib import Path

def need(ok,message):
    if not ok:raise ValueError(message)

def repository(here):
    for path in (here,*here.parents):
        if (path/'.git').exists():return path
    raise ValueError('Use --repo with a repository containing the pinned Git objects')

def load_readers(repo,manifest):
    blobs={}
    for name in ('catalogue_semantic','full_semantic','mhgp11_formats'):
        path='morsehgp3D_v11/bench/'+name+'.py'
        raw=subprocess.check_output(['git','-C',str(repo),'show',manifest['source_pin']+':'+path])
        need(hashlib.sha256(raw).hexdigest()==manifest['sources_sha256'][path],'source hash '+path)
        blobs[name]=raw.decode()
    for name,source in blobs.items():
        module=types.ModuleType(name);module.__file__=manifest['source_pin']+':'+name+'.py';sys.modules[name]=module
        exec(compile(source,module.__file__,'exec'),module.__dict__)
    baseline=sys.modules['mhgp11_formats']
    source=blobs['mhgp11_formats'];old=manifest['reader_guard_before']
    need(source.count(old)==1,'repair anchor')
    fixed=types.ModuleType('formats_with_proposed_guard');fixed.__file__='proposed:morsehgp3D_v11/bench/mhgp11_formats.py'
    exec(compile(source.replace(old,manifest['reader_guard_after']),fixed.__file__,'exec'),fixed.__dict__)
    return baseline,fixed

NONE=(1<<32)-1

def column(values,width=4):
    raw=struct.pack('<'+str(len(values))+('I' if width==4 else 'B'),*values)
    return raw+bytes((-len(raw))%8)

def point_file(target,when,bits=21):
    n,L,N,P,Bk,W=3,3,4,3,3,4 if bits==24 else 3
    sites=b''.join(column(v) for v in ([0,1,2],[0,0,0],[0,0,0],[9,17,100]))
    levels=column([0,1,2])
    # Exact levels 0,1,4 give increasing plateau dates 0,1,2; W little endian limbs per rational value.
    for values in ([0,1,4],[1,1,1]):
        levels+=struct.pack('<'+str(W*L)+'Q',*[word for value in values for word in ([value]+[0]*(W-1))])
    nodes=column([3,3,3,NONE])+column([0,0,0,1])
    owner=2 if when==0 else 3
    hanging=b''.join(column(v) for v in ([0,0,when],[0,0,0],[0,0,0],[0,1,owner],[0,0,when]))+column([0,0,0],1)
    tree=b''.join(column(v) for v in ([0,1,2],[0,0,0],[0,0,0],[0,0,1],[2,2,NONE],[0,1,target],[0,0,when]))
    sections=(sites,levels,nodes,hanging,tree);offsets=[144]
    for sec in sections:offsets.append(offsets[-1]+len(sec))
    header=b'MHGP11PT'+struct.pack('<17Q',1,bits,1,1,1,n,L,W,N,P,Bk,*offsets)
    need(len(header)==144,'header')
    return header+b''.join(sections)

def verdict(reader,raw,bits):
    try:
        f=reader.read_points(raw,bits,exact=True)
    except ValueError as error:return {'accepted':False,'reason':str(error)}
    return {'accepted':True,'block_plateau':list(f.block_plateau),'block_parent':list(f.block_parent),
            'site_block':list(f.site_block),'site_plateau':list(f.site_plateau)}

def proof(repo,manifest):
    baseline,fixed=load_readers(repo,manifest);out={}
    cases=(('before_parent',0,0,True),('at_parent',0,1,False),('after_parent',0,2,False),
           ('root_at_birth',2,1,True),('root_later',2,2,True),('before_block_birth',2,0,False))
    for name,target,when,expected in cases:
        out[name]={}
        for bits in (18,21,24):
            raw=point_file(target,when,bits);a=verdict(baseline,raw,bits);b=verdict(fixed,raw,bits)
            need(a['accepted']==(name!='before_block_birth'),'baseline '+name)
            need(b['accepted']==expected,'fixed '+name)
            out[name][str(bits)]={'file_bytes':len(raw),'file_sha256':hashlib.sha256(raw).hexdigest(),
                                  'baseline':a,'proposed_guard':b}
    return {'source_pin':manifest['source_pin'],'reader':'real read_points(..., exact=True) from verified Git blobs',
            'cases':out,'native_runs':0,'cloud_runs':0,'math_boundary':'block_birth <= site_entry < parent_birth; root has no upper bound',
            'limits':'Hand-serialized external metadata; no cloud realizing this tree and no native factory defect demonstrated'}

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path);parser.add_argument('--check',type=Path)
args=parser.parse_args();here=Path(__file__).resolve().parent;manifest=json.loads((here/'source_manifest.json').read_text())
result=proof(args.repo or repository(here),manifest)
if args.check:
    need(json.loads(args.check.read_text())==result,'frozen JSON differs')
    print('chronology reader PASS:18 cases,36 real exact reads baseline+guard; before/at/after parent, root/birth; native0 cloud0')
else:print(json.dumps(result,indent=2,ensure_ascii=False,sort_keys=True))
