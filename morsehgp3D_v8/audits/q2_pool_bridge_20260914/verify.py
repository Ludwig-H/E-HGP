#!/usr/bin/env python3
"""Independent closure of audit builds, paired streams and receipt mutants."""
from __future__ import annotations
import argparse
import copy
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
sys.path.insert(0,str(BASE))
import measure


def require(ok,message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def strict_row(row):
    # The frozen runner already applies this at ingestion. Repeat recursively
    # here: its replay check only tests top-level durations for finiteness.
    json.dumps(row,allow_nan=False)
    measure.check(row)


def check():
    pins={}
    def pin(path):
        pins[str(path.relative_to(ROOT))]=sha(path)
    builds={}
    for name in ('r1','san_r1','r2','san_r2'):
        path=BASE/(name+'_BUILD.json')
        r=json.loads(path.read_text())
        archive=ROOT/r['archive']
        require(r['source_commit']==measure.COMMIT and sha(archive)==r['archive_sha256'], 'wrong/changed build')
        require(r['status']==('failed' if name=='san_r1' else 'passed'), 'unexpected build status')
        with zipfile.ZipFile(archive) as z:
            require(set(z.namelist())==set(r['source_sha256']), 'wrong archive paths')
            for p,h in r['source_sha256'].items():
                require(hashlib.sha256(z.read(p)).hexdigest()==h, 'changed archived source')
        if name in ('r2','san_r2'):
            require(all(s['returncode']==s['expected_returncode'] for s in r['steps']), 'build/gate failed')
            for p,h in r['audit_inputs'].items():
                require(sha(ROOT/p)==h, 'changed active adapter')
                pin(ROOT/p)
        if name=='san_r1':
            require(any('LeakSanitizer' in s.get('stderr','') for s in r['steps']), 'lost initial sandbox failure')
        builds[name]=r
        pin(path);pin(archive)
    require(builds['r2']['source_sha256']==builds['san_r2']['source_sha256'], 'Release/sanitizer sources differ')
    gates=[]
    for name in ('r2','san_r2'):
        gates.append(next(json.loads(s['stdout']) for s in builds[name]['steps'] if s['stdout'].startswith('{')))
    require(gates[0]==gates[1] and gates[0]['pipeline_runs']==840 and gates[0]['rejected_inputs']==8,
            'gate results differ or lost coverage')
    require(builds['san_r2']['gate_environment']==dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
            UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'), 'sanitizer controls changed')
    directories=sorted(BASE.glob('campaign_*'))
    require({p.name for p in directories}=={'campaign_'+p for p in measure.PLANS}, 'missing campaign')
    summaries=[];rows=[];discrete={}
    for directory in directories:
        # completed alone is not an authority: recompute all pair checks.
        summaries.append(measure.validate(directory))
        for path in directory.iterdir():
            require(path.is_file(), 'unexpected campaign artifact')
            pin(path)
        for line in (directory/'MEASURES.jsonl').read_text().splitlines():
            row=json.loads(line);strict_row(row);rows.append(row)
            r=row['result'];key=tuple(row['key'])
            value={k:v for k,v in r.items() if not k.endswith('_ms')}
            require(key not in discrete or discrete[key]==value, 'repeat changed work/output')
            discrete[key]=value
    require(len(rows)==36 and len(discrete)==33, 'wrong measurement coverage')
    old={}
    for directory in sorted((BASE.parent/'q2_order_lidar_20260914').glob('campaign_*')):
        done=json.loads((directory/'COMPLETION.json').read_text())
        require(done['status']=='completed' and done['measures_sha256']==sha(directory/'MEASURES.jsonl'),
                'old LiDAR baseline not closed')
        for p in (directory/'MEASURES.jsonl',directory/'COMPLETION.json'):pin(p)
        for line in (directory/'MEASURES.jsonl').read_text().splitlines():
            row=json.loads(line)
            if row['key'][5:]==['sibling','complement']:
                require(row['status']=='completed' and row['returncode']==0, 'failed old baseline')
                old[tuple(row['key'][:3])]=row['result']
    for folder in ('paired_8k','growth_s8'):
        directory=ROOT/'morsehgp3D_v8/receipts/q2_witness_order_20260914'/folder
        done=json.loads((directory/'COMPLETION.json').read_text())
        require(done['status']=='completed' and done['source_hashes_unchanged'] and done['probe_hash_unchanged'],
                'constructor baseline not closed')
        for p in (directory/'MEASURES.jsonl',directory/'COMPLETION.json'):pin(p)
        for line in (directory/'MEASURES.jsonl').read_text().splitlines():
            row=json.loads(line);r=row['result']
            if (r['family'],r['s'],r['kmax'],r['seed'],r['front_mode'],r['census_mode'],
                r['sibling_mode'],r['witness_order'])==('clusters',8,10,3,'samples','shared','sibling','complement'):
                require(row['status']=='completed' and row['exit_code']==0, 'failed constructor baseline')
                old[('clusters',r['n'],r['s'])]=r
    baseline_matches=0
    fields=('digest','front_work','census_work','order_work','sibling_work','candidate_pairs',
            'accepted_pairs','rejected_pairs','input_rectangles','anchor_queries')
    for row in rows:
        if row['key'][3]=='baseline':
            previous=old[tuple(row['key'][:3])]
            require(all(previous[f]==row['result'][f] for f in fields), 'published baseline work/output changed')
            baseline_matches+=1
    require(baseline_matches==12, 'missing baseline match')
    mutants=[]
    seed=next(r for r in rows if r['key']==['single_000000',50000,8,'pool-shared'])
    edits=(('wrong_mode',lambda r:r.update(mode='baseline')),
           ('lost_Pool_mass',lambda r:r['pool_work'].__setitem__('filtered_pairs',0)),
           ('wrong_cutoff',lambda r:r.update(cutoff=2)),
           ('false_roots',lambda r:r['census_work'].__setitem__('count_root_starts',0)),
           ('recopied_cloud',lambda r:r.update(cloud_coordinate_copies=2*r['n'])),
           ('missing_shell',lambda r:r['digest'].__setitem__('shell_ids',0)),
           ('nested_nan',lambda r:r['front_work'].__setitem__('proposed_sites',float('nan'))),
           ('infinite_time',lambda r:r.update(total_ms=float('inf'))))
    for name,edit in edits:
        row=copy.deepcopy(seed);edit(row['result']);row['stdout']=json.dumps(row['result'])
        try:strict_row(row)
        except (RuntimeError,ValueError,KeyError) as error:mutants.append(dict(name=name,detected_by=str(error)))
        else:raise RuntimeError('undetected receipt mutant '+name)
    row=copy.deepcopy(seed);row['status']='failed'
    try:strict_row(row)
    except RuntimeError as error:mutants.append(dict(name='failed_row',detected_by=str(error)))
    else:raise RuntimeError('failed row accepted')
    pin(BASE/'measure.py');pin(Path(__file__))
    compact=[]
    for row in rows:
        r=row['result']
        compact.append(dict(key=row['key'],total_ms=r['total_ms'],pipeline_ms=r['pipeline_ms'],
                            count_nodes=r['census_work']['count_node_visits'],pool_work=r['pool_work'],
                            supports=r['accepted_pairs'],candidates=r['candidate_pairs'],
                            preparation_ms=r['pool_preparation_ms'],query_build_ms=r['query_build_ms'],
                            selected_total_ms=r['selected_total_ms'],payload_ms=r['payload_ms']))
    return dict(status='passed',rows=len(rows),configurations=len(discrete),campaigns=summaries,
                baseline_matches=baseline_matches,gate=gates[0],receipt_mutants=mutants,pins=pins,measures=compact)


def record(name):
    require(name.isidentifier(), 'invalid receipt name')
    output=BASE/(name+'_VALIDATION.json')
    require(not output.exists(), 'refuse overwrite')
    report=dict(status='running',schema='mhgp8_audit_pool_bridge_validation_v1',commands=[],
                started_utc=datetime.now(timezone.utc).isoformat(),public_status='not_claimed',gcp_used=False)
    def save():output.write_text(json.dumps(report,indent=2,sort_keys=True,allow_nan=False)+'\n')
    def run(command):
        step=dict(command=command);report['commands'].append(step);save()
        result=subprocess.run(command,cwd=ROOT,text=True,capture_output=True,timeout=120)
        step.update(returncode=result.returncode,stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),stderr=result.stderr)
        require(result.returncode==0,'reader or docs failed: '+result.stderr)
        return result.stdout
    save()
    try:
        normal=json.loads(run([sys.executable,'-B',str(Path(__file__)),'--check']))
        optimized=json.loads(run([sys.executable,'-B','-O',str(Path(__file__)),'--check']))
        require(normal==optimized,'normal/optimized closure differ')
        report.update(checks=normal,normal_and_optimized_identical=True)
        sys.path.insert(0,str(ROOT/'tools'))
        import check_docs
        docs=(BASE/'README.md',BASE.parent/'DIALOGUE_COURANT.md')
        errors=[e for p in docs for e in check_docs.validate(p)]
        require(not errors,'audit docs errors '+str(errors))
        report['documents']={str(p.relative_to(ROOT)):sha(p) for p in docs}
        report['canonical_docs']=run([sys.executable,'-B','tools/check_docs.py'])
        report['registry']=run([sys.executable,'-B','tools/check_implementation_status.py'])
        require(all(sha(ROOT/p)==h for p,h in normal['pins'].items()), 'pin changed during closure')
        report['status']='passed'
    except BaseException as error:
        report.update(status='failed',error_type=type(error).__name__,error=str(error));raise
    finally:
        report['finished_utc']=datetime.now(timezone.utc).isoformat();save()
    print(json.dumps(dict(status=report['status'],receipt=str(output))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true');parser.add_argument('--name',default='r1')
    args=parser.parse_args()
    if args.check:print(json.dumps(check(),sort_keys=True))
    else:record(args.name)
