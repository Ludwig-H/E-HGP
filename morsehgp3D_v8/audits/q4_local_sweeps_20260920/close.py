#!/usr/bin/env python3
"""Read-only final closure, including binary/input links outside read.py's scope."""
import gzip
import json
from pathlib import Path
import re
import subprocess

from evidence import BASE, ROOT, pins, sha


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path):
    raw = path.read_bytes()
    return json.loads(gzip.decompress(raw) if path.suffix == '.gz' else raw)


def closure(folder):
    manifest, completion = load(folder/'MANIFEST.json'), load(folder/'COMPLETION.json')
    require(manifest['status'] == completion['status'] == 'completed', 'Incomplete capture')
    require(completion['manifest_sha256'] == sha(folder/'MANIFEST.json'), 'Manifest changed')
    require(manifest['sources'] == pins(), 'Pinned sources changed')
    return manifest


def child(record, binary):
    require(record['returncode'] == 0 and not record['stderr'], 'Child failed')
    command = record['command']
    require(Path(command[0]).resolve() == binary.resolve(), 'Wrong child binary')
    source = Path(command[1])
    require(source.parent.resolve() == (BASE/'.inputs').resolve(), 'Wrong input location')
    require(sha(source) == record['input_sha256'], 'Input changed')
    result = json.loads(record['stdout'])
    require(result['input_sha256'] == record['input_sha256'], 'Child input mismatch')
    require(result['input_bytes'] == source.stat().st_size, 'Input size mismatch')
    require(result['n'] == int(source.read_bytes().splitlines()[0]), 'Population mismatch')
    require(command[2:] == [str(result[k]) for k in ('kmax', 'depth', 'node_budget', 'domain', 'mode')],
            'Command/config mismatch')
    require(result['status'] == 'passed' and result['product_code_used'] is False
            and result['positive_support_filter_executed'] is False, 'Wrong child scope')
    return result


def main():
    qualification = closure(BASE/'qualification')
    require(len(qualification['commands']) == 9, 'Missing qualification command')
    for command in qualification['commands']:
        require(command['returncode'] == 0 and not command['stderr'], 'Qualification failed')
    for path, digest in qualification['binaries'].items():
        require(sha(BASE/path) == digest, 'Qualified binary changed')
    commands = qualification['commands']
    require(json.loads(commands[4]['stdout']) == json.loads(commands[5]['stdout']), 'Math modes differ')
    reports = [json.loads(c['stdout']) for c in commands[6:]]
    require(all(r['status'] == 'passed' and r['counts'] == reports[0]['counts'] for r in reports),
            'Oracle summaries differ')
    discrete = []
    for report, command in zip(reports, commands[6:]):
        filename = Path(command['command'][-1]).name
        path = BASE/'qualification'/filename
        require(sha(path) == qualification['oracle_captures'][filename] == report['capture_sha256'],
                'Oracle capture changed')
        binary = Path(command['command'][-2])
        require(sha(binary) == report['binary_sha256'], 'Oracle binary mismatch')
        records = load(path)
        require(len(records) == 216, 'Missing oracle calls')
        data = [child(record, binary) for record in records]
        keys = ('input_sha256', 'kmax', 'depth', 'node_budget', 'domain', 'mode', 'work',
                'rejected_seed_ids', 'root_digest', 'roots')
        discrete.append([{k: row[k] for k in keys} for row in data])
    require(discrete[0] == discrete[1] == discrete[2], 'Oracle discrete payload/work differs')

    capture = closure(BASE/'capture')
    binary = Path(capture['binary'])
    require(sha(binary) == capture['binary_sha256'] == qualification['binaries']['.build/qualification/release'],
            'Measured binary not qualified Release')
    require(capture['mode'] == 1 and len(capture['records']) == 66, 'Wrong campaign')
    rows = []
    for entry in capture['records']:
        path = BASE/'capture'/entry['path']
        require(sha(path) == entry['sha256'], 'Measured record changed')
        record = load(path)
        data = child(record, binary)
        require(record['n'] == data['n'] and record['domain'] == data['domain'], 'Record config mismatch')
        rows.append(dict(case=record['case'], **data))
    for optimized, name in ((False, 'normal'), (True, 'optimized')):
        command = ['python3', '-B'] + (['-O'] if optimized else []) + [str(BASE/'read.py'), str(BASE/'capture')]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        require(result.returncode == 0 and not result.stderr, 'Capture reader failed')
        require(json.loads(result.stdout) == load(BASE/f'READ_{name}.json.gz'), 'Saved reader result differs')
    require(load(BASE/'READ_normal.json.gz') == load(BASE/'READ_optimized.json.gz'), 'Reader modes differ')

    clip = load(BASE/'CLIP_CHECKS.json')
    require(len(clip) == 2 and all(c['returncode'] == 0 and not c['stderr'] for c in clip), 'Clipping failed')
    require(json.loads(clip[0]['stdout']) == json.loads(clip[1]['stdout']), 'Clipping modes differ')
    for command in clip:
        require(Path(command['command'][-1]).resolve() == BASE/'clip_gate.py', 'Wrong clipping model')
    preflight = load(BASE/'COUNT_PREFLIGHT.json')
    require(preflight['binary_sha256'] == sha(binary), 'Preflight binary differs')
    require(child(preflight, binary)['mode'] == 0, 'Preflight was not count only')
    require(len(load(BASE/'PREFLIGHT.json')['attempts']) == 2, 'Construction failures missing')

    reference = load(BASE/'REFERENCE_PINS.json')
    for path, digest in reference['explicit_reuse'].items():
        require(sha(ROOT/path) == digest, 'Reused audit source changed')
    docs = [BASE/'README.md', BASE/'MATH.md', BASE/'CLIPPING.md', BASE.parent/'DIALOGUE_COURANT.md']
    links = 0
    for doc in docs:
        for link in re.findall(r'\]\(([^)]+)\)', doc.read_text()):
            target = link.split('#', 1)[0]
            if target and '://' not in target:
                # CHECKS is the output of this check and may not exist yet.
                require((doc.parent/target).exists() or (doc.parent/target).resolve() == BASE/'CHECKS.json',
                        'Missing document link: '+target)
                links += 1
    dense = {r['case']: r['work']['W'] for r in rows if r['domain'] == 1 and r['case'].startswith('dense_')}
    lidar = [r for r in rows if r['domain'] == 1 and not r['case'].startswith('dense_')]
    require(len(lidar) == 27 and len(dense) == 6, 'Case families differ')
    lidar_work = {key: sum(r['work'][key] for r in lidar)
                  for key in ('I', 'A', 'W', 'sort_comparisons', 'groups', 'lowdepthgroups', 'shell_ids')}
    inventory = {str(p.relative_to(BASE)): sha(p) for p in sorted(BASE.rglob('*'))
                 if p.is_file() and not any(part.startswith('.') or part == '__pycache__'
                                           for part in p.relative_to(BASE).parts)
                 and p.name != 'CHECKS.json'}
    print(json.dumps(dict(schema='mhgp8_audit_local_sweeps_closure_v1', status='passed',
                         qualification_commands=9, oracle_counts_each=reports[0]['counts'],
                         discrete_oracles_equal=True, math=json.loads(commands[4]['stdout']),
                         clipping=json.loads(clip[0]['stdout']), measured_configurations=66,
                         binary_and_input_links_checked=True, readers_equal=True,
                         dense_positive_W=dense, lidar_positive_work=lidar_work,
                         local_document_links=links, files=inventory,
                         scope='Independent audit only; constructor read-time hashes are not a qualification.',
                         gcp_used=False), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
