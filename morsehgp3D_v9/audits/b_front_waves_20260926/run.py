#!/usr/bin/env python3
"""Capture audit-only CPU wave scheduling; no GCP and no product mutation."""
import argparse
import hashlib
import json
import subprocess
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--mode', choices=('fixtures','scaling','scale32','frame'), required=True)
    parser.add_argument('--packed', action='store_true')
    parser.add_argument('--frame', type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    binary = args.binary.resolve()
    files = sorted((ROOT/'morsehgp3D_v9/src/gen').rglob('*.hpp')) + sorted((ROOT/'morsehgp3D_v9/src/gen').rglob('*.cpp'))
    files += [HERE/'front_waves.cpp', HERE/'run.py']
    sources = {str(p.relative_to(ROOT)): sha(p) for p in files}
    provenance = dict(schema='mhgp9_front_waves_provenance_v1', binary=str(binary), binary_sha256=sha(binary),
                      head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                      sources=sources, mode=args.mode, packed=args.packed, GCP_used=False)
    (args.out/'PROVENANCE.json').write_text(json.dumps(provenance,indent=2)+'\n')
    if args.mode == 'fixtures':
        commands=[(['--fixtures'],0)] + [(['--mutant',m],1) for m in ('drop','depth','pending','mask')]
    elif args.mode == 'scaling':
        commands=[(['--synthetic',kind,str(n),'5','8'],0)
                  for kind in ('uniform','terrain','rows') for n in (8000,16000,32000)]
    elif args.mode == 'scale32':
        commands=[(['--synthetic','uniform','32000','5','8'],0)]
    else:
        if args.frame is None:
            raise ValueError('--frame required')
        provenance['frame_sha256']=sha(args.frame)
        provenance['frame_bytes']=args.frame.stat().st_size
        (args.out/'PROVENANCE.json').write_text(json.dumps(provenance,indent=2)+'\n')
        commands=[(['--frame',str(args.frame.resolve()),'5',str(s)],0) for s in (8,10,12)]
    rows=[]
    outcomes=[]
    for i,(tail,expected) in enumerate(commands):
        cmd=[str(binary)]+(['--packed'] if args.packed else [])+tail
        print('RUN',i,' '.join(tail),flush=True)
        start=time.monotonic()
        with (args.out/f'{i:02d}.stdout').open('w') as out, (args.out/f'{i:02d}.stderr').open('w') as err:
            result=subprocess.run(cmd,stdout=out,stderr=err,check=False,cwd=ROOT)
        outcome=dict(command=cmd,returncode=result.returncode,expected=expected,wall_s=time.monotonic()-start)
        outcomes.append(outcome)
        (args.out/'COMMANDS.json').write_text(json.dumps(outcomes,indent=2)+'\n')
        if result.returncode != expected:
            raise ValueError(f'case {i}: return {result.returncode}, expected {expected}')
        stdout=(args.out/f'{i:02d}.stdout').read_text()
        stderr=(args.out/f'{i:02d}.stderr').read_text()
        if expected == 0:
            current=[json.loads(line) for line in stdout.splitlines()]
            if not current or any(not r['equal_all_work'] or not r['equal_rectangles'] for r in current):
                raise ValueError(f'case {i}: missing equality')
            rows.extend(current)
        elif not stderr.startswith('cause=waves.'):
            raise ValueError(f'case {i}: mutant killed noncausally: {stderr}')
        print('DONE',i,round(outcome['wall_s'],3),'s',flush=True)
    stable=all(sha(ROOT/p)==value for p,value in sources.items()) and sha(binary)==provenance['binary_sha256']
    summary=dict(schema='mhgp9_front_waves_capture_v1',sources_stable=stable,commands=len(commands),
                 rows=rows,mutants=sum(expected!=0 for _,expected in commands),GCP_used=False,
                 public_status='not_claimed',scope='CPU_front_only_no_S2_no_q34_generation_no_FULL')
    (args.out/'SUMMARY.json').write_text(json.dumps(summary,indent=2)+'\n')
    if not stable:
        raise ValueError('sources/binary changed during capture')
    files=sorted(p for p in args.out.iterdir() if p.is_file())
    (args.out/'SHA256SUMS').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in files))
    print('PASS',len(rows),'comparisons',summary['mutants'],'mutants',flush=True)

if __name__ == '__main__':
    main()
