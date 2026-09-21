#!/usr/bin/env python3
"""Independent exhaustive check of the explicit constructor31 snapshot."""
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
sys.path.insert(0, str(BASE))
import oracle


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    raw = path.read_bytes()
    return json.loads(gzip.decompress(raw) if path.suffix == '.gz' else raw)


def write(path, data):
    path.write_text(json.dumps(data, sort_keys=True, indent=2)+'\n')


def pins():
    declared = load(ROOT/'morsehgp3D_v8/receipts/q4_window_20260920/scale_qv0aggg3/MANIFEST.json')['source_sha256']
    paths = []
    for name, digest in declared.items():
        if name.startswith('morsehgp3D_v8/src/'):
            require(sha(ROOT/name) == digest, 'Frozen dependency changed: '+name)
            paths.append(ROOT/name)
    paths += [BASE/name for name in ('oracle.py', 'global_probe.cpp', 'campaign.py', 'SNAPSHOT.json',
                                     'snapshot/pipeline/wspd_q34.cpp', 'snapshot/pipeline/wspd_q34.hpp')]
    paths += [BASE.parent/'q4_center_blocks_20260920'/name for name in ('oracle_gate.py','fixtures.py')]
    paths += [ROOT/'build'/build/'libmhgp8_p0.a' for build in ('v8_q4_window_20260920','v8_q4_window_sanitize_20260920')]
    for source in load(BASE/'SNAPSHOT.json')['sources'].values():
        require(sha(BASE/source['snapshot']) == source['sha256'], 'Snapshot changed')
    return {str(path.relative_to(ROOT)):sha(path) for path in paths}


def execute(command):
    start = time.monotonic()
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    try:
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, timeout=180)
        return dict(command=command, returncode=result.returncode, stdout=result.stdout,
                    stderr=result.stderr, seconds=time.monotonic()-start)
    except BaseException as error:
        def string(value):
            return value.decode(errors='replace') if isinstance(value, bytes) else (value or '')
        return dict(command=command, returncode=None, error=repr(error),
                    stdout=string(getattr(error, 'stdout', '')), stderr=string(getattr(error, 'stderr', '')),
                    seconds=time.monotonic()-start)


def append(folder, manifest, record):
    path = folder/f'{len(manifest["records"]):04d}.json.gz'
    path.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    manifest['records'].append(dict(path=path.name, sha256=sha(path)))
    write(folder/'MANIFEST.json', manifest)
    require(record['returncode'] == 0 and not record['stderr'], 'Failed command preserved: '+str(record['command']))


def configurations(name):
    # Tuple K, separation, mask, backend, front, workers. Zero workers is the
    # original mono entry; 1/4 use the independent Coarse team entry.
    if name == 'line260':
        return [(3,8,6,30,'samples',w) for w in (0,1,4)]
    if name == 'shell30':
        return [(3,8,6,backend,'samples',w) for backend,w in ((28,0),(30,0),(30,4))]
    return [(1,8,6,30,'samples',4), (2,12,4,30,'samples',4), (2,8,6,28,'samples',0),
            (3,8,6,28,'samples',0), (3,8,6,28,'samples',4),
            (3,8,6,30,'samples',0), (3,8,6,30,'samples',1), (3,8,6,30,'samples',4),
            (5,10,6,28,'pure',0), (5,10,6,30,'pure',4),
            (5,12,2,30,'samples',0), (5,12,4,28,'samples',0), (10,8,6,30,'samples',0)]


def input_file(name, points):
    folder = BASE/'.inputs'
    folder.mkdir(exist_ok=True)
    path = folder/(name+'.txt')
    raw = (str(len(points))+'\n'+'\n'.join(' '.join(map(str,p)) for p in points)+'\n').encode()
    require(not path.exists() or path.read_bytes() == raw, 'Input overwrite refused')
    path.write_bytes(raw)
    return path


def normalized(records):
    return sorted((r['arity'], tuple(r['support']), tuple(map(int,r['coefficients'])),
                   r['depth'], tuple(r['shell'])) for r in records)


def input_hash(points):
    value = 14695981039346656037
    for word in [len(points)]+[x for p in points for x in p]:
        for _ in range(8):
            value = ((value ^ (word & 255))*1099511628211) & ((1<<64)-1)
            word >>= 8
    return value


def verify(data, points, config, expected):
    k,s,mask,backend,mode,workers = config
    require(data['status'] == 'completed' and data['schema'] == 'audit_q34_global_contract_probe_v1', 'Probe schema/status')
    require([data[x] for x in ('kmax','s','mask','q4_backend','front_mode','workers')] == list(config), 'Config mismatch')
    require(data['n'] == len(points) and data['input_fnv1a_u64_le'] == input_hash(points), 'Parsed input differs')
    require(normalized(data['records']) == normalized(expected), 'Complete rational oracle differs')
    active = mask & ((1 << min(k,3))-1)
    front, ledger = data['front'], data['ledger']
    mass = len(points)*(len(points)-1)//2
    require(front['total_unordered_pairs'] == mass and front['active_lane_mask'] == active, 'Front metadata differs')
    for lane in range(3):
        require(front['rejected_pair_mass'][lane]+front['residual_pair_mass'][lane] == (mass if active&(1<<lane) else 0),
                'Front lane coverage mass differs')
    require(ledger['q3_edges'] == front['residual_pair_mass'][1] and ledger['q4_edges'] == front['residual_pair_mass'][2],
            'Expanded lane work differs')
    require(ledger['expanded_pairs'] == ledger['cover_builds'] == ledger['q3_edges']+ledger['q4_edges']-ledger['both_edges'],
            'Shared cover ledger differs')
    require(ledger['q3_emitted']+ledger['q4_emitted'] == len(expected), 'Emission ledger differs')
    require(ledger['payload_shell_ids'] == sum(len(r['shell']) for r in expected), 'Shell ledger differs')
    require(len(data['work_words']) == len(data['logical_work_words']) == 279 and len(front['words']) == 49,
            'Complete work layout differs')
    if workers:
        p = data['parallel_words']
        require(len(p) == 11 and p[0] == workers and p[1] == len(data['worker_words']) == min(workers,p[3]), 'Worker ledger differs')
        require(p[4] == p[3] == sum(w[0] for w in data['worker_words']), 'Job completion differs')
        require(sum(w[3] for w in data['worker_words']) == ledger['expanded_pairs'], 'Worker edge sum differs')
        require(sum(w[4] for w in data['worker_words']) == ledger['q3_emitted'] and
                sum(w[5] for w in data['worker_words']) == ledger['q4_emitted'], 'Worker emission sum differs')
        require(sum(w[6] for w in data['worker_words']) == p[10], 'Worker peak sum differs')
    else:
        require(not any(data['parallel_words']) and not data['worker_words'], 'Mono hid parallel work')


def qualify(folder):
    require(folder.parent == BASE and not folder.exists(), 'Fresh direct-child capture required')
    folder.mkdir()
    build = BASE/'.build'/folder.name
    require(not build.exists(), 'Fresh build required')
    build.mkdir(parents=True)
    before = pins()
    manifest = dict(schema='mhgp8_audit_global_contract_campaign_v1', status='started', sources=before, records=[])
    flags = ['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(BASE/'snapshot'),'-I',str(ROOT/'morsehgp3D_v8/src')]
    sources = [str(BASE/'global_probe.cpp'),str(BASE/'snapshot/pipeline/wspd_q34.cpp')]
    commands = [['g++','--version'],['clang++','--version'],
        ['g++',*flags,'-O2',*sources,str(ROOT/'build/v8_q4_window_20260920/libmhgp8_p0.a'),'-pthread','-o',str(build/'release')],
        ['clang++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',*sources,
         str(ROOT/'build/v8_q4_window_sanitize_20260920/libmhgp8_p0.a'),'-pthread','-o',str(build/'sanitize')]]
    commands += [['python3','-B',*flags,str(BASE/'oracle.py')] for flags in ([],['-O'])]
    for command in commands:
        append(folder,manifest,execute(command))
        print('PASS setup',len(manifest['records']),flush=True)
    manifest['binaries'] = {str(build/name):sha(build/name) for name in ('release','sanitize')}
    counts = {}
    totals = dict(configurations=0,calls=0,emissions=0,shell_ids=0,max_shell=0,parallel_equalities=0)
    logical = {}
    for name, points in oracle.fixtures():
        path = input_file(name,points)
        for config in configurations(name):
            expected = oracle.expected(points,config[0],config[2],counts)
            prior = None
            for kind in ('release','sanitize'):
                command = [str(build/kind),str(path),*map(str,config)]
                record = dict(case=name,configuration=config,input_sha256=sha(path),**execute(command))
                append(folder,manifest,record)
                data = json.loads(record['stdout'])
                verify(data,points,config,expected)
                identity = name,*config[:-1]
                invariants = data['front'],data['logical_work_words'],data['records']
                if identity in logical:
                    require(logical[identity] == invariants, 'Mono/parallel or Release/sanitizer logical work differs')
                    if kind == 'release':totals['parallel_equalities'] += 1
                else:
                    logical[identity] = invariants
                if prior is not None:
                    require(prior == invariants, 'Release/sanitizer differs')
                prior = invariants
                totals['calls'] += 1
                totals['emissions'] += len(expected)
                totals['shell_ids'] += sum(len(r['shell']) for r in expected)
                totals['max_shell'] = max([totals['max_shell']]+[len(r['shell']) for r in expected])
            totals['configurations'] += 1
        print('PASS oracle',name,flush=True)
    require(totals['max_shell'] == 30 and totals['emissions'] > 0 and totals['parallel_equalities'] > 0, 'Vacuous oracle')
    require(before == pins(), 'Dependencies changed during campaign')
    manifest.update(status='completed',totals=totals,oracle_counts=counts)
    write(folder/'MANIFEST.json',manifest)
    write(folder/'COMPLETION.json',dict(status='completed',manifest_sha256=sha(folder/'MANIFEST.json'),records=len(manifest['records'])))
    print(json.dumps(dict(status='passed',totals=totals,oracle_counts=counts),sort_keys=True),flush=True)


if __name__ == '__main__':
    require(len(sys.argv) == 2, 'usage: campaign.py FRESH_CAPTURE_NAME')
    qualify(BASE/sys.argv[1])
