#!/usr/bin/env python3
"""Measure full audit Pool/census streams with immutable paired receipts."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
COMMIT = 'e3af11a7b2ecba4a71929c7112610ff2618f813e'
MODES = ('baseline', 'pool-pair', 'pool-shared')


def matrix(dataset, sizes, separations=(8,), reverse=False):
    return [(dataset,n,s,mode) for n in sizes for s in separations
            for mode in (tuple(reversed(MODES)) if reverse else MODES)]


PLANS = {
    'pilot': sum((matrix(d,(8000,)) for d in ('single_000000','single_000100','single_000200')), []),
    'growth': matrix('single_000000',(16000,32000)),
    'check50k': matrix('single_000000',(50000,)),
    'separation': matrix('single_000000',(8000,),(10,12)),
    'clusters': matrix('clusters',(8000,16000,32000)),
    'repeat50k': matrix('single_000000',(50000,),reverse=True),
}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stamp():
    return datetime.now(timezone.utc).isoformat()


def write(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True, allow_nan=False)+'\n')


def input_path(dataset,n):
    require(dataset in ('clusters','single_000000','single_000100','single_000200'), 'unknown input')
    return None if dataset == 'clusters' else BASE.parent/'lidar08_20260914/prepared'/dataset/f'n{n}.u16le'


def command_for(binary,key):
    dataset,n,s,mode = key
    return [str(binary), str(input_path(dataset,n)) if dataset != 'clusters' else 'clusters',
            str(n), '10', str(s), '0' if mode == 'baseline' else '64', mode, 'samples']


def fnv(path):
    result=14695981039346656037
    for byte in path.read_bytes():
        result=((result^byte)*1099511628211)&((1<<64)-1)
    return format(result,'x')


def check(row, completed=True):
    if completed:
        require(row['status']=='completed', 'noncompleted measure')
    require(row['returncode']==0 and not row['stderr'], 'invocation failed')
    r=row['result']
    require(json.loads(row['stdout'])==r, 'raw/result mismatch')
    dataset,n,s,mode=row['key']
    require(r['status']=='completed' and r['schema']=='mhgp8_audit_pool_bridge_v1' and
            r['n']==n and r['s']==s and r['kmax']==10 and r['mode']==mode and
            r['cutoff']==(0 if mode=='baseline' else 64) and r['threads']==1 and
            r['gcp_used'] is False and r['public_status']=='not_claimed', 'wrong measured configuration')
    if dataset!='clusters':
        require(r['input_fnv64']==row['input_fnv64'], 'input checksum differs')
    for key,value in r.items():
        if key.endswith('_ms'):
            require(isinstance(value,(int,float)) and math.isfinite(value) and value>=0, 'invalid time')
    f,c,p=r['front_work'],r['census_work'],r['pool_work']
    for counters in (c,p,r['order_work'],r['sibling_work']):
        require(all(type(v) is int and v>=0 for v in counters.values()), 'invalid counter')
    total=n*(n-1)//2
    require(f['residual_pair_mass'][0]+f['rejected_pair_mass'][0]==total and f['xi_bound_tests']==0 and
            all(f[field][1:]==[0,0] for field in ('residual_pair_mass','rejected_pair_mass','lane_rectangles')),
            'front lane/mass mismatch')
    require(r['input_rectangles']==f['emitted_rectangles']==sum(f['size_class_rectangles']) and
            f['residual_pair_mass'][0]==sum(f['size_class_pair_mass']), 'front descriptor mismatch')
    require(r['candidate_pairs']+p['filtered_pairs']==f['residual_pair_mass'][0] and
            p['selected_pairs']==p['filtered_pairs']+p['residual_pairs'] and
            r['candidate_pairs']==r['accepted_pairs']+r['rejected_pairs'], 'Pool/census mass mismatch')
    if mode=='baseline':
        require(all(v==0 for v in p.values()), 'baseline used Pool')
    else:
        require(p['selected_rectangles']==sum(f['size_class_rectangles'][3:]) and
                p['selected_pairs']==sum(f['size_class_pair_mass'][3:]), 'wrong size policy')
    require(c['count_root_starts']==r['anchor_queries']-p['original_selected_anchors']+p['local_roots'] and
            c['query_tasks']==c['count_root_starts']+2*c['query_splits'] and c['frontier_restarts']==0,
            'query root/restart mismatch')
    require(c['count_node_visits']==c['count_bound_tests']+c['count_point_tests'] and
            c['query_build_point_visits']==p['local_b_sites'] and c['query_build_nodes']==p['local_b_nodes'],
            'geometry/query tree mismatch')
    if mode!='pool-pair':
        require(c['cursor_advances']==c['count_node_visits']-c['query_splits']+sum(r['order_work'].values()),
                'cursor geometry/structure mismatch')
    d=r['digest']
    require(d['supports']==r['accepted_pairs']==c['payload_supports'] and
            d['interior_ids']==c['payload_interior_sites'] and d['shell_ids']==c['payload_shell_sites'],
            'physical payload mismatch')
    require(r['cloud_coordinate_copies']==n and r['cloud_validation_points']==n, 'cloud rebuilt per rectangle')
    require(r['total_ms']+1e-6>=r['pipeline_ms']>=r['payload_ms'] and
            r['pipeline_ms']+1e-6>=r['selected_total_ms'] and
            r['selected_total_ms']+1e-6>=r['pool_preparation_ms']+r['query_build_ms'], 'enclosing time mismatch')


def build_paths(receipt):
    r=json.loads(receipt.read_text())
    require(r['status']=='passed' and not r['sanitizer'] and r['source_commit']==COMMIT, 'wrong build')
    require(all(step['returncode']==step['expected_returncode'] for step in r['steps']), 'unclosed gate/build')
    binary,archive=ROOT/r['binary'],ROOT/r['archive']
    require(sha(binary)==r['binary_sha256'] and sha(archive)==r['archive_sha256'], 'changed build artifacts')
    for path,pin in r['audit_inputs'].items():
        require(sha(ROOT/path)==pin, 'changed adapter source '+path)
    return binary, {receipt, binary, archive, *(ROOT/p for p in r['audit_inputs'])}


def validate(directory):
    manifest=json.loads((directory/'MANIFEST.json').read_text())
    done=json.loads((directory/'COMPLETION.json').read_text())
    require(done['status']=='completed' and done['manifest_sha256']==sha(directory/'MANIFEST.json') and
            done['measures_sha256']==sha(directory/'MEASURES.jsonl'), 'campaign not closed')
    require(manifest['runner_sha256']==sha(Path(__file__)), 'changed reader')
    for path,pin in manifest['pins'].items():
        require(sha(ROOT/path)==pin, 'changed campaign input '+path)
    binary,_=build_paths(ROOT/manifest['build_receipt'])
    rows=[json.loads(line) for line in (directory/'MEASURES.jsonl').read_text().splitlines()]
    expected=[list(key) for key in PLANS[manifest['plan']]]
    require(manifest['matrix']==expected and [r['key'] for r in rows]==expected, 'incomplete paired matrix')
    pairs={}
    for row in rows:
        check(row)
        require(row['command']==command_for(binary,row['key']), 'command/result mismatch')
        path=input_path(*row['key'][:2])
        if path:
            require(row['input_sha256']==manifest['pins'][str(path.relative_to(ROOT))], 'input pin mismatch')
        r=row['result']
        compared={field:r[field] for field in ('input_fnv64','digest','front_work','accepted_pairs','input_rectangles','anchor_queries')}
        compared['payload']={k:v for k,v in r['census_work'].items() if k.startswith('payload_')}
        key=tuple(row['key'][:3])
        require(key not in pairs or pairs[key]==compared, 'paired complete output/front changed')
        pairs[key]=compared
    return dict(status='passed', plan=manifest['plan'], rows=len(rows), paired_inputs=len(pairs))


def measure(plan,receipt):
    directory=BASE/('campaign_'+plan)
    require(not directory.exists(), 'refuse overwrite')
    binary,paths=build_paths(receipt)
    paths.update(input_path(d,n) for d,n,_,_ in PLANS[plan] if d!='clusters')
    pins={str(p.relative_to(ROOT)):sha(p) for p in sorted(paths)}
    cpus=sorted(os.sched_getaffinity(0)); cpu=cpus[-1]
    manifest=dict(schema='mhgp8_audit_pool_bridge_campaign_v1', plan=plan, matrix=PLANS[plan],
                  build_receipt=str(receipt.relative_to(ROOT)), binary=str(binary.relative_to(ROOT)),
                  runner_sha256=sha(Path(__file__)), pins=pins, started_utc=stamp(),
                  platform=platform.platform(), command=sys.argv, python=sys.version,
                  allowed_cpus=cpus, selected_cpu=cpu, timeout_seconds=300,
                  timeout_policy='failed_no_partial_result', public_status='not_claimed', gcp_used=False)
    directory.mkdir()
    write(directory/'MANIFEST.json',manifest)
    status='failed'
    try:
        with (directory/'MEASURES.jsonl').open('x') as stream:
            for key in PLANS[plan]:
                command=command_for(binary,key)
                row=dict(key=key,command=command,status='failed',returncode=None,stdout='',stderr='',started_utc=stamp())
                try:
                    path=input_path(*key[:2])
                    if path: row.update(input_sha256=sha(path),input_fnv64=fnv(path))
                    row['loadavg_before']=os.getloadavg()
                    r=subprocess.run(command,capture_output=True,text=True,timeout=300,
                                     preexec_fn=lambda:os.sched_setaffinity(0,{cpu}))
                    row.update(returncode=r.returncode,stdout=r.stdout,stderr=r.stderr)
                    if r.returncode==0:
                        parsed=json.loads(r.stdout)
                        json.dumps(parsed,allow_nan=False)
                        row['result']=parsed
                    check(row,completed=False)
                    row['status']='completed'
                except BaseException as error:
                    if isinstance(error,subprocess.TimeoutExpired):
                        row.update(returncode=124,stdout=(error.stdout or b'').decode(errors='replace'),
                                   stderr=(error.stderr or b'').decode(errors='replace'))
                    row.update(error_type=type(error).__name__,error=str(error))
                    raise
                finally:
                    row['finished_utc']=stamp()
                    stream.write(json.dumps(row,allow_nan=False)+'\n');stream.flush()
                r=row['result']
                print(json.dumps(dict(key=key,total_ms=r['total_ms'],candidates=r['candidate_pairs'],
                                      filtered=r['pool_work']['filtered_pairs'],count_nodes=r['census_work']['count_node_visits'])),flush=True)
        require(all(sha(ROOT/p)==h for p,h in pins.items()), 'input/source changed during campaign')
        status='completed'
    finally:
        measures=directory/'MEASURES.jsonl'
        write(directory/'COMPLETION.json',dict(status=status,finished_utc=stamp(),
              manifest_sha256=sha(directory/'MANIFEST.json'),measures_sha256=sha(measures) if measures.exists() else None))
    print(json.dumps(validate(directory)),flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan',choices=PLANS)
    parser.add_argument('--build-receipt',type=Path)
    parser.add_argument('--validate',type=Path)
    args=parser.parse_args()
    require((args.plan is None)!=(args.validate is None), 'choose plan or validate')
    if args.validate: print(json.dumps(validate(args.validate.resolve()),sort_keys=True))
    else:
        require(args.build_receipt is not None, 'missing build receipt')
        measure(args.plan,args.build_receipt.resolve())
