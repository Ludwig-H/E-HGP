#!/usr/bin/env python3
"""Tiny synthetic M4 dumps, no external data or v11 engine used.

Square in Morton IDs A=0 (0,0), B=1 (2,0), D=2 (0,2), C=3 (2,2).
Cat_4 has four diametral edge balls at level 1 and one circle ball at level 2.
Expected FULL: k1 four births -> one four-way merge; k2 four edge births ->
one four-way merge; k3 and k4 each one circle birth at level 2.
The lower image k4 -> k3 is a birth, explicitly exercising extended T6.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess
import tempfile

NONE = (1<<32)-1


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_sources(repo, manifest_path):
    manifest = json.loads(manifest_path.read_text())
    head = subprocess.check_output(['git', '-C', str(repo), 'rev-parse', 'HEAD'], text=True).strip()
    for entry in manifest['files']:
        path = entry['path']
        blob = subprocess.check_output(
            ['git', '-C', str(repo), 'rev-parse', manifest['pin']+':'+path], text=True).strip()
        if blob != entry['blob'] or sha256(repo/path) != entry['sha256']:
            raise RuntimeError('source differs from audited pin: '+path)
    return dict(audited_pin=manifest['pin'], checkout_head=head,
                manifest_sha256=sha256(manifest_path), sources_checked=len(manifest['files']))


def section(tag, fmt, rows):
    pack = struct.Struct('<'+fmt)
    return tag, pack.size, b''.join(pack.pack(*row) for row in rows), len(rows)


def center_section(points):
    data = b''.join(int(x).to_bytes(16,'little',signed=True) for p in points for x in (*p,1))
    return 'BCENTER',64,data,len(points)


def write(path, kind, k, n, sections):
    out=bytearray(struct.pack('<8s6IQ24s',b'MHGP12DP',1,kind,21,4,k,len(sections),n,b'audit_square'))
    for tag,size,data,count in sections:
        out += struct.pack('<8sIIQ',tag.encode(),size,0,count)+data
        out += b'\0'*(-len(data)%8)
    path.write_bytes(out)


def make(directory, flower_mode):
    points=[(0,0,0),(2,0,0),(0,2,0),(2,2,0)]
    supports=[(0,1),(0,2),(1,3),(2,3),(0,3)]
    populations=[s for s in supports[:4]]+[(0,1,2,3)]
    off=[0]
    for p in populations:off.append(off[-1]+len(p))
    write(directory/'cat.bin',1,0,4,[
      section('SITEXYZ','3I',points),
      section('BALLS','8I',[(1 if i<4 else 2,0,2 if i<4 else 4,2,*s,NONE,NONE) for i,s in enumerate(supports)]),
      section('POPOFF','Q',[(x,) for x in off]),
      section('POPVAL','I',[(x,) for p in populations for x in p]),
      section('NLEVELS','Q',[(3,)])])
    birth_rows={1:[(i,0,c,0) for i,c in enumerate([0,2,1,3])],
                2:[(i,1,c,0) for i,c in enumerate([1,0,3,2])],
                3:[(4,2,0,1)],4:[(4,2,0,1)]}
    centers={1:points,2:[(1,0,0),(0,1,0),(2,1,0),(1,2,0)],3:[(1,1,0)],4:[(1,1,0)]}
    # k1 circle cell inert: four strict singleton traces, as in v11 build_cell.
    cell_rows={1:[(i,1,0,2,2,0) for i in range(4)]+[(4,2,0,4,2,1)],
               2:[(4,2,0,4,2,1)],3:[],4:[]}
    trace_sites={1:[0,1,0,2,1,3,2,3,0,1,2,3],2:[0,1,2,3],3:[],4:[]}
    trace_masks={1:[1,2,1,2,1,2,1,2,1,2,4,8],2:[3,5,10,12],3:[],4:[]}
    cell_off={1:[0,2,4,6,8,12],2:[0,4],3:[0],4:[0]}
    canonical={1:[0,2,1,3],2:[1,0,3,2]}
    for k in range(1,5):
        seeds=[(key,canonical[k][key],1,0,0) for key in trace_sites[k]]
        # Unused sections remain structurally present, as in the producer format.
        write(directory/f'ordre_{k}.bin',2,k,4,[
          section('BIRTHS','4I',birth_rows[k]),center_section(centers[k]),
          section('CELLS','6I',cell_rows[k]),section('CELLOFF','Q',[(x,) for x in cell_off[k]]),
          section('TRACEA','Q',[(m,) for m in trace_masks[k]]),section('SEEDS','2I2BH',seeds),
          section('PARTOFF','Q',[(0,)]*(len(seeds)+1)),
          section('PARTS',str(k)+'I',[]),section('PARTINF','4BI',[])])
        if k<=2:
            rank=k-1
            keys=[0,2,1,3] if k==1 else [1,0,3,2]
            nodes=[(rank,4,key,0,0) for key in keys]+[(k,NONE,NONE,4,0)]
            edges=[(i,) for i in range(4)]
            meta=[(4,),(4,),(4,)]
        else:
            nodes=[(2,NONE,4,0,0)]; edges=[]; meta=[(1,),(0,),(0,)]
        sections=[section('FNODES','4IQ',nodes),section('FEDGES','I',edges),section('FMETA','Q',meta)]
        if k>=2 and flower_mode!='absent':
            low=([4]*5 if k==2 else [4] if k==3 else [0])
            if flower_mode=='wrong' and k==2:low[0]=0
            sections.append(section('FLOWER','I',[(x,) for x in low]))
        write(directory/f'foret_{k}.bin',3,k,0,sections)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--binary',required=True,type=Path)
    parser.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[4])
    args=parser.parse_args()
    script=Path(__file__).resolve()
    before=verify_sources(args.repo_root,script.with_name('sources.json'))
    binary_before=sha256(args.binary)
    script_before=sha256(script)
    results=[]
    with tempfile.TemporaryDirectory(prefix='ehgp-m4-square-') as tmp:
        base=Path(tmp)
        for mode in ('valid','wrong','absent'):
            folder=base/mode;folder.mkdir()
            make(folder,mode)
            command=[str(args.binary),str(folder),'--repetitions','1','--fils-contraction','2']
            process=subprocess.run(command,capture_output=True,text=True,timeout=20)
            stdout=[json.loads(s) for s in process.stdout.splitlines() if s]
            results.append(dict(mode=mode,exit_code=process.returncode,stderr=process.stderr,
              rows=stdout,synthetic_dumps={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(folder.iterdir())}))
    expected={'valid':0,'wrong':1,'absent':0}
    for result in results:
        if result['exit_code']!=expected[result['mode']]:
            raise RuntimeError('observed behavior changed: '+json.dumps(result))
    checked=lambda result:sum(r.get('lem_t6',{}).get('naissances_jugees',0) for r in result['rows'])
    if checked(results[0])!=6 or checked(results[2])!=0:
        raise RuntimeError('unexpected T6 coverage')
    after=verify_sources(args.repo_root,script.with_name('sources.json'))
    if before!=after or binary_before!=sha256(args.binary) or script_before!=sha256(script):
        raise RuntimeError('pin, sources, binary or witness changed during execution')
    print(json.dumps(dict(schema='ehgp.v12.m4.t6_missing_section.v1',
      binary_sha256=binary_before,script_sha256=script_before,source_closure=before,
      before_after_unchanged=True,
      verdicts=dict(valid_square_accepted=True,wrong_vertical_refused=True,
                    missing_flower_false_success_reproduced=True),
      native_microbench_executed=True,synthetic_points=4,results=results),sort_keys=True,indent=2))


if __name__=='__main__':main()
