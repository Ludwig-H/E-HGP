#!/usr/bin/env python3
"""Replay every saved output against the independent complete small oracle."""
import json
from pathlib import Path
import sys

from campaign import BASE, ROOT, configurations, input_hash, load, normalized, pins, require, sha, verify
import oracle


def main():
    folder = BASE/'qualification_r2'
    manifest, completion = load(folder/'MANIFEST.json'), load(folder/'COMPLETION.json')
    require(manifest['status'] == completion['status'] == 'completed', 'Capture incomplete')
    require(completion['manifest_sha256'] == sha(folder/'MANIFEST.json'), 'Manifest changed')
    require(manifest['sources'] == pins(), 'Dependencies changed')
    require(completion['records'] == len(manifest['records']), 'Record inventory differs')
    for filename, digest in manifest['binaries'].items():
        require(sha(Path(filename)) == digest, 'Executable changed')
    commands = []
    for entry in manifest['records']:
        path = folder/entry['path']
        require(sha(path) == entry['sha256'], 'Record changed')
        record = load(path)
        require(record['returncode'] == 0 and not record['stderr'], 'Saved command failed')
        commands.append(record)
    require(len(commands) >= 6 and json.loads(commands[4]['stdout']) == json.loads(commands[5]['stdout']),
            'Math model Python modes differ')
    model = json.loads(commands[4]['stdout'])
    require(model['status'] == 'passed' and len(model['mutants']) == 7, 'Missing model/mutants')
    fixtures = dict(oracle.fixtures())
    planned = {(name,tuple(config),binary) for name in fixtures for config in configurations(name)
               for binary in manifest['binaries']}
    seen, logical, outputs, counts = set(), {}, {}, {}
    totals = dict(configurations=0,calls=0,emissions=0,shell_ids=0,max_shell=0,parallel_equalities=0)
    case_summary = {}
    for record in commands[6:]:
        config = tuple(record['configuration'])
        name, points = record['case'], fixtures[record['case']]
        command = record['command']
        require(command[0] in manifest['binaries'] and command[2:] == list(map(str, config)), 'Command mismatch')
        identity = name,config,command[0]
        require(identity not in seen and identity in planned, 'Duplicate/unplanned call')
        seen.add(identity)
        source = Path(command[1])
        require(source == BASE/'.inputs'/(name+'.txt') and sha(source) == record['input_sha256'], 'Input changed')
        lines = source.read_text().splitlines()
        parsed = [tuple(map(int,line.split())) for line in lines[1:]]
        require(int(lines[0]) == len(points) and parsed == [tuple(p) for p in points], 'Fixture changed')
        data = json.loads(record['stdout'])
        expected_id = name,config
        if expected_id not in outputs:
            outputs[expected_id] = oracle.expected(points,config[0],config[2],counts)
            totals['configurations'] += 1
        expected = outputs[expected_id]
        verify(data,points,config,expected)
        work_id = name,*config[:-1]
        invariant = data['front'],data['logical_work_words'],data['records']
        if work_id in logical:
            require(logical[work_id] == invariant, 'Geometry/payload differs across execution modes')
            if Path(command[0]).name == 'release':
                totals['parallel_equalities'] += 1
        else:
            logical[work_id] = invariant
        totals['calls'] += 1
        totals['emissions'] += len(expected)
        totals['shell_ids'] += sum(len(r['shell']) for r in expected)
        totals['max_shell'] = max([totals['max_shell']]+[len(r['shell']) for r in expected])
        row = case_summary.setdefault(name,dict(n=len(points),calls=0,q3=0,q4=0,max_shell=0,
                                               rejected_q3_mass=0,rejected_q4_mass=0,multiworker_calls=0))
        row['calls'] += 1
        for q in (3,4):
            row[f'q{q}'] += sum(r['arity'] == q for r in expected)
            row[f'rejected_q{q}_mass'] += data['front']['rejected_pair_mass'][q-2]
        row['max_shell'] = max([row['max_shell']]+[len(r['shell']) for r in expected])
        row['multiworker_calls'] += sum(w[0] > 0 for w in data['worker_words']) > 1
    require(seen == planned and totals == manifest['totals'] and counts == manifest['oracle_counts'],
            'Campaign inventory or oracle counts differ')
    require(sum(r['rejected_q3_mass'] for r in case_summary.values()) > 0 and
            sum(r['rejected_q4_mass'] for r in case_summary.values()) > 0 and
            sum(r['multiworker_calls'] for r in case_summary.values()) > 0, 'Vacuous front/parallel checks')

    # The first failed capture remains distinct, including its exact sources
    # and the LeakSanitizer diagnostic; it is never promoted to qualification.
    failed = BASE/'qualification'
    failure = load(failed/'FAILURE.json')
    require(failure['status'] == 'failed' and failure['manifest_sha256'] == sha(failed/'MANIFEST.json'),
            'Initial failure manifest changed')
    for filename, digest in failure['source_snapshots'].items():
        require(sha(failed/filename) == digest, 'Initial source snapshot changed')
    record = load(failed/failure['record'])
    require(sha(failed/failure['record']) == failure['record_sha256'] and record['stderr'] == failure['stderr'] and
            record['returncode'] == 1 and 'LeakSanitizer does not work under ptrace' in record['stderr'],
            'Initial sanitizer failure missing')

    print(json.dumps(dict(schema='mhgp8_audit_global_contract_readback_v1',status='passed',
        source_snapshot=load(BASE/'SNAPSHOT.json'),source_count=len(manifest['sources']),
        capture_records=len(commands),totals=totals,oracle_counts=counts,model=model,
        cases=case_summary,normalization='full exact records; no digest-only output comparison',
        complete_oracle=True,initial_sandbox_failure_preserved=True,
        scope='Explicit uncommitted constructor31 snapshot; small complete canonical stream only; no LiDAR performance or FULL qualification',
        gcp_used=False),sort_keys=True,indent=2))


if __name__ == '__main__':
    require(len(sys.argv) == 1, 'usage: read.py')
    main()
