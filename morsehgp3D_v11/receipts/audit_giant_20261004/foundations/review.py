#!/usr/bin/env python3
"""Portable metadata re-read only: no build, native program, VM or product import."""
from pathlib import Path
import hashlib
import json
import re
import sys
import tarfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
PIN = '0f5e8a207f2974e262cd40a8882b97af1da396af'
QUAL = 'c40f40798375a0fc37917499401f16876cccbd2a'
checks = 0


def need(condition, message):
    global checks
    checks += 1
    if not condition:
        raise RuntimeError(message)


def load(path):
    return json.loads((ROOT / path).read_text())


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    sources = []
    for path in sorted(ROOT.glob('SOURCE*BEFORE.json')):
        value = json.loads(path.read_text())
        need(value['source_commit'] == PIN, 'source pin ' + path.name)
        sources.extend(value['sources'])
    need(len({x['payload'] for x in sources}) == len(sources), 'source inventory duplicated')
    for item in sources:
        data = (ROOT / item['payload']).read_bytes()
        need(len(data) == item['bytes'] and sha(data) == item['sha256'], 'source capture ' + item['path'])
    after = load('SOURCE_AFTER.json')
    need(after['source_commit'] == PIN, 'after pin')
    need({x['path'] for x in after['sources']} == {x['path'] for x in sources}, 'after inventory')
    for item in after['sources']:
        need(item['unchanged'] and item['source_sha256_before'] == item['source_sha256_after'] == item['capture_sha256'],
             'before/after source drift ' + item['path'])
    parity = load('proof/source_parity_c40.json')
    need(parity['qualification_source'] == QUAL and parity['review_source'] == PIN, 'native/source parity pins')
    foundational = [x for x in parity['sources'] if x['foundation_scope']]
    need({x['path'] for x in foundational if not x['same']} ==
         {'morsehgp3D_v11/README.md', 'morsehgp3D_v11/docs/PROVENANCE.md'}, 'unexpected foundation source change')
    inventory = load('module_inventory.json')
    need(all(inventory['modules'][x] for x in ('core', 'cloud', 'sched')), 'foundation module absent')
    need(not any(inventory['modules'][x] for x in ('parallel', 'io', 'api')) and not inventory['cli_present'],
         'planned facade mistakenly counted as existing')
    proof = load('PROOF_BEFORE.json')
    archive = (ROOT / proof['archive']['payload']).read_bytes()
    need(len(archive) == proof['archive']['bytes'] and sha(archive) == proof['archive']['sha256'], 'native archive bytes')
    receipt = load('proof/receipt_subset.json')
    need(receipt['commit'] == QUAL and receipt['worker_source'] == 'commit:' + QUAL, 'native receipt source')
    need(receipt['status'] == 'completed' and receipt['worker_exit_code'] == 0 and receipt['results_verified'] is True,
         'historical qualification incomplete')
    need(receipt['results_sha256'] == sha(archive) and receipt['results_bytes'] == len(archive), 'receipt archive identity')
    need(receipt['targeted_shutdown_certified'] is True and receipt['generation'] == receipt['closing_generation'],
         'historical target closure')
    minfacts = load('proof/target_running_minimal.json')
    need(minfacts['machineType'].endswith('/g4-standard-48') and minfacts['status'] == 'RUNNING', 'G4 hardware capture')
    need(minfacts['lastStartTimestamp'] == receipt['generation'], 'historical generation identity')
    expected_configs = {
        'gcc_release': (18, None), 'bits21': (21, None), 'bits24': (24, None),
        'gcc_asan_ubsan': (24, '-DMHGP11_SANITIZE=ON'),
        'gcc_tsan': (21, '-DMHGP11_TSAN=ON'), 'poison': (21, '-DMHGP11_POISON=ON')}
    native = []
    with tarfile.open(fileobj=__import__('io').BytesIO(archive), mode='r:gz') as tf:
        members = {m.name: m for m in tf if m.isfile()}
        need(len(members) == len([m for m in tf.getmembers() if m.isfile()]), 'duplicate archive path')
        manifest = tf.extractfile('results/MANIFEST.sha256').read().decode()
        listed = set()
        for line in manifest.splitlines():
            digest, relative = line.split('  ', 1)
            name = 'results/' + relative.removeprefix('./')
            need(name not in listed and name in members, 'native manifest membership')
            listed.add(name)
            need(sha(tf.extractfile(name).read()) == digest, 'native manifest hash ' + name)
        need(listed == set(members) - {'results/MANIFEST.sha256'}, 'native manifest exhaustive')
        need(len(listed) == receipt['results_manifest_files'], 'native manifest file count')
        summary = json.load(tf.extractfile('results/cmd/000_matrice/files/matrix/summary.json'))
        need(summary['complete'] is True and summary['conforming'] is True and summary['exit_code'] == 0,
             'historical matrix verdict')
        cfg = {x['name']: x for x in summary['configurations']}
        need(cfg['clang_release']['status'] == 'absent', 'Clang unexpectedly qualified')
        for name, (bits, variant) in expected_configs.items():
            c = cfg[name]
            need(c['status'] == 'ok' and c['conforming'] is True and not c['failures'] and not c['not_run'],
                 'native configuration ' + name)
            need('-DMHGP11_COORD_BITS=' + str(bits) in c['cmake_options'] and
                 (variant is None or variant in c['cmake_options']), 'native profile ' + name)
            base = 'results/cmd/000_matrice/files/matrix/' + name + '/'
            tests = json.load(tf.extractfile(base + 'tests.json'))
            xml = ET.fromstring(tf.extractfile(base + 'junit.xml').read())
            played = list(xml.iter('testcase'))
            need({x['name'] for x in tests} == {x.attrib['name'] for x in played}, 'native test inventory ' + name)
            scoped = [x for x in played if x.attrib['name'].startswith(('mhgp11_core_', 'mhgp11_cloud_', 'mhgp11_sched_'))]
            need(all(x.attrib.get('status') == 'run' and x.find('failure') is None and
                     x.find('error') is None and x.find('skipped') is None for x in scoped), 'foundation native verdict ' + name)
            counts = {module: sum(x.attrib['name'].startswith('mhgp11_' + module + '_') for x in scoped)
                      for module in ('core', 'cloud', 'sched')}
            need(counts == {'core': 30 if name == 'poison' else 29, 'cloud': 20, 'sched': 13},
                 'foundation native counts ' + name)
            native.append({'configuration': name, 'bits': bits, 'variant': variant,
                           'counts': counts, 'passed': len(scoped)})
        mutantlog = tf.extractfile('results/cmd/000_matrice/files/matrix/mutants/LastTest.log').read().decode()
        mutants = {}
        pattern = r'mutants_ok module=(core|cloud|sched) mutants=(\d+) tues=(\d+) dont_signal=(\d+) dont_delai=(\d+) dont_construction=(\d+) plancher=(\d+)'
        for match in re.finditer(pattern, mutantlog):
            module, *nums = match.groups()
            need(module not in mutants, 'duplicate mutation summary')
            count, killed, signals, delays, construction, floor = map(int, nums)
            declared = load('source/morsehgp3D_v11/tests/mutants/' + module + '.json')
            need(count == killed == floor == len(declared['mutants']) == declared['plancher'], 'mutation count ' + module)
            need(signals == delays == 0, 'noncausal kill class ' + module)
            need(construction == (2 if module == 'core' else 0), 'construction classification ' + module)
            mutants[module] = {'declared': count, 'killed': killed, 'construction': construction,
                               'signals': signals, 'timeouts': delays}
        need(set(mutants) == {'core', 'cloud', 'sched'}, 'mutation evidence incomplete')
    # Source-level control transition only; no claim of native scheduling or TSan reproduction.
    text = (ROOT / 'source/morsehgp3D_v11/src/tower/forest_internal.hpp').read_text()
    need('return done || idx(level) < closed;' in text, 'ready predicate source')
    none = 2**32 - 1
    level = 1
    ready_after_abandon = False or level < none
    need(ready_after_abandon, 'sentinel no longer passes lower ready')
    outcome = {'scope': 'read-only source and existing native metadata; no build/native/cloud',
               'source': PIN, 'qualification_source': QUAL, 'checks': checks,
               'source_files': len(sources), 'native_foundation_configurations': native,
               'native_foundation_total': sum(x['passed'] for x in native), 'mutants': mutants,
               'foundation_new_defects': [],
               'pipeline_adverse': {'lower_view': {'closed': none, 'done': False, 'abandoned': True},
                                    'level': level, 'ready': ready_after_abandon,
                                    'status': 'confirmed control-flow gap; no native race or false FULL success reproduced'}}
    print(json.dumps(outcome, sort_keys=True, indent=2))


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print(json.dumps({'status': 'failed', 'checks_completed': checks, 'error': str(error)}, sort_keys=True))
        sys.exit(1)
