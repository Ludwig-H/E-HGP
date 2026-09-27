#!/usr/bin/env python3
"""Four read-only Python gates, frozen FULL proof closure, fresh capture only.

Historical native binaries/objects are hashed as provenance, never executed.
The explicit dependency list covers project imports and dynamically loaded
oracles; the Python executable/version is recorded, not a hermetic OS image.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
import traceback

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
V9 = HERE.parents[1]
WEIGHTED = HERE.parent/'weighted_clustering_20260927'
PYTHON = Path('/home/codespace/.python/current/bin/python')
FULL = Path('/workspaces/E-HGP/build/v9-weighted-full-qualification-20260927-r1')
FULL_SHA = '35126c2a591a4d410b44d86a80967b50f4574bcdb9cadc5d94d9a982bafaba9c'
FROZEN = {
    'point_tree.py': 'ed3a15592cdcc95aa5c3dab33ed10707e0a0e1b80c73f9b9d19c2762bbb64975',
    'test_point_tree.py': '4a459b58571269f9a6483d3085833b2dc240664ca2aa5b55a953af15a452d8a5',
    'point_eom.py': 'aef65dc4dbc03a66fa0e975bbb33b68bced162ea16d25d0d65372f413ebf3cb3',
    'test_point_eom_audit.py': '9bb0bcc8e75ee21464ff8500875c5360286ec42673a7e6bb19934c71b3cec558',
}
PROJECT_DEPENDENCIES = [HERE/'run_qualification.py', *[HERE/name for name in FROZEN],
    *[WEIGHTED/name for name in ('point_routing_reference.py', 'weighted_model.py',
        'full_weighted_tree.py', 'full_attachment_oracle.py', 'qualify_geometry.py',
        'test_weighted_eom.py', 'weighted_eom.py')],
    V9/'audits/b_gaussian_point_clustering_20260927/condensed.py',
    V9/'audits/b_point_hierarchy_k_20260927/eom.py']
EXPECTED_TREE = dict(status='passed', fixtures=181, cuts=3124, refusals=27,
                     qualified_integrations=3, scope='point forest/cuts; no new geometry or EOM')


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2)
        stream.write('\n'); stream.flush(); os.fsync(stream.fileno())


def snapshot(expected):
    return {path: sha(path) if Path(path).is_file() else None for path in sorted(expected)}


def inventory():
    expected = {}
    def add(path, digest):
        path = str(path)
        need(path not in expected or expected[path] == digest, 'conflicting proof pin: '+path)
        expected[path] = digest
    proof_path = FULL/'receipt.json'
    need(sha(proof_path) == FULL_SHA, 'frozen FULL qualification receipt')
    proof = json.loads(proof_path.read_text())
    need(proof['status'] == 'passed' and proof['sources_before'] == proof['sources_after'],
         'historical FULL source closure')
    add(proof_path, FULL_SHA)
    for path, digest in proof['sources_before'].items():
        add(path, digest)
    actual_names = {p.name for p in FULL.iterdir() if p.is_file() and p.name != 'receipt.json'}
    need(actual_names == set(proof['artifacts']), 'historical FULL artifact inventory')
    for name, digest in proof['artifacts'].items():
        need(Path(name).name == name, 'historical artifact basename')
        add(FULL/name, digest)
    build_path = Path(proof['build_receipt'])
    need(sha(build_path) == proof['build_receipt_sha256'], 'historical build receipt pin')
    build = json.loads(build_path.read_text())
    need(build['status'] == 'completed' and build['pins_before'] == build['pins_after'], 'historical native build closure')
    need(build['binary'] == proof['native_binary'] and build['binary_sha256'] == proof['native_binary_sha256'],
         'historical native binary binding')
    add(build_path, proof['build_receipt_sha256'])
    for key in ('pins_before', 'artifacts_sha256'):
        for path, digest in build[key].items():
            add(path, digest)
    # The FULL receipt also pins the generated attachment wrapper receipt
    # and sources. Validate its own closure, without executing its builder.
    wrapper_path = build_path.parent.with_name(build_path.parent.name+'-adapter')/'receipt.json'
    need(str(wrapper_path) in expected, 'wrapper receipt included in FULL proof')
    need(sha(wrapper_path) == expected[str(wrapper_path)], 'historical wrapper receipt pin')
    wrapper = json.loads(wrapper_path.read_text())
    need(wrapper['status'] == 'completed' and wrapper['pins_match'] is True and
         wrapper['pins_before'] == wrapper['pins_after'], 'historical wrapper closure')
    for key in ('pins_before', 'generated_sha256'):
        for path, digest in wrapper[key].items():
            add(path, digest)
    add(PYTHON, sha(PYTHON)); add(PYTHON.resolve(), sha(PYTHON.resolve()))
    for path in PROJECT_DEPENDENCIES:
        add(path, FROZEN.get(path.name, sha(path)) if path.parent == HERE else sha(path))
    fixtures = {row['name']: row for row in proof['fixtures']}
    commands = {row['name']: row for row in proof['commands']}
    used = []
    for name in ('e5_silent_k2', 'square_k2', 'square_k1'):
        command, fixture = commands[name], fixtures[name]
        need(command['returncode'] == 0, 'historical fixture command succeeded')
        need(json.loads((FULL/(name+'.command.json')).read_text()) == command, 'fixture command receipt binding')
        for suffix in ('stdout', 'stderr'):
            need(proof['artifacts'][name+'.'+suffix] == command[suffix+'_sha256'], 'fixture stream binding')
        need(command['argv'] == [proof['native_binary'], '--input', str(FULL/(name+'.u32le')),
             '--k', str(fixture['k']), '--workers', '1', '--verify-coverage'], 'fixture native argv binding')
        need((FULL/(name+'.u32le')).read_bytes() == b''.join(struct.pack('<III', *point) for point in fixture['points']),
             'fixture oracle coordinates match native input')
        used.append(dict(name=name, n=len(fixture['points']), k=fixture['k'],
            files={str(FULL/(name+'.'+suffix)): proof['artifacts'][name+'.'+suffix]
                   for suffix in ('stdout', 'stderr', 'command.json', 'u32le')}))
    return dict(sorted(expected.items())), used


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, help='must not exist; default: mktemp directory under /tmp')
    args = parser.parse_args()
    need(Path(sys.executable).resolve() == PYTHON.resolve(), 'run with canonical Python')
    if args.output is None:
        output = Path(tempfile.mkdtemp(prefix='mhgp9-point-dendrogram-qualification-20260927-', dir='/tmp'))
    else:
        output = args.output.absolute(); output.mkdir(parents=False, exist_ok=False)
    receipt = dict(schema='mhgp9_point_dendrogram_qualification_v1', status='running',
        output=str(output), cwd=str(ROOT), python=str(PYTHON), python_resolved=str(PYTHON.resolve()),
        python_version=sys.version, GCP_used=False, native_geometry_executed=False,
        native_build_executed=False, commands=[], sources_before={}, sources_after={},
        project_dependencies=list(map(str, PROJECT_DEPENDENCIES)),
        scope='point_forest_and_common_EOM_python_gates_existing_FULL_proofs_only',
        historical_proof_policy='hash closure, not a new native build or all-33-fixture geometry replay',
        environment_overrides=dict(PYTHONDONTWRITEBYTECODE='1', PYTHONHASHSEED='0', PYTHONNOUSERSITE='1'))
    save(output/'intent.json', receipt)
    print(json.dumps(dict(status='started', output=str(output))), flush=True)
    started = time.monotonic()
    try:
        pins, used = inventory(); receipt['sources_before'] = snapshot(pins)
        need(receipt['sources_before'] == pins, 'current inputs differ from frozen proof pins')
        receipt.update(fixtures_used=used, expected_pins=pins, pin_count=len(pins))
        save(output/'preflight.json', dict(expected_pins=pins, fixtures_used=used))
        environment = dict(os.environ, **receipt['environment_overrides'])
        for test in ('test_point_tree.py', 'test_point_eom_audit.py'):
            for optimized in (False, True):
                name = Path(test).stem+('_optimized' if optimized else '_normal')
                argv = [str(PYTHON), '-B']+(['-O'] if optimized else [])+[str(HERE/test)]
                if test == 'test_point_tree.py':
                    argv += ['--qualified-fixtures', str(FULL)]
                row = dict(name=name, argv=argv, cwd=str(ROOT), timeout_seconds=120,
                           environment_overrides=receipt['environment_overrides'], returncode=None)
                tick = time.monotonic()
                try:
                    with (output/(name+'.stdout')).open('xb') as out, (output/(name+'.stderr')).open('xb') as err:
                        completed = subprocess.run(argv, cwd=ROOT, env=environment, stdout=out, stderr=err,
                                                   timeout=120, check=False)
                    row['returncode'] = completed.returncode
                except BaseException as error:
                    row['error'] = repr(error)
                    raise
                finally:
                    row.update(elapsed_seconds=time.monotonic()-tick,
                        stdout_sha256=sha(output/(name+'.stdout')), stderr_sha256=sha(output/(name+'.stderr')))
                    receipt['commands'].append(row); save(output/(name+'.command.json'), row)
                need(row['returncode'] == 0, 'gate failed: '+name)
                if test == 'test_point_tree.py':
                    need(json.loads((output/(name+'.stdout')).read_text()) == EXPECTED_TREE, 'point gate coverage')
                    need((output/(name+'.stderr')).stat().st_size == 0, 'point gate stderr')
                else:
                    log = (output/(name+'.stderr')).read_text()
                    need('Ran 9 tests in ' in log and log.rstrip().endswith('OK'), 'EOM unittest coverage')
                    need((output/(name+'.stdout')).stat().st_size == 0, 'EOM gate stdout')
        receipt.update(status='passed', summary=dict(commands=4, point_tree=EXPECTED_TREE, eom_unittest_gates=9,
            count_policy='each mode separately; never add normal and optimized counts'))
    except BaseException as error:
        receipt.update(status='failed', error=repr(error), traceback=traceback.format_exc())
    finally:
        receipt['sources_after'] = snapshot(receipt['sources_before'])
        if receipt['sources_after'] != receipt['sources_before']:
            receipt.update(status='failed', closure_error='source/proof hashes changed')
        receipt['elapsed_seconds'] = time.monotonic()-started
        receipt['artifacts'] = {p.name: sha(p) for p in sorted(output.iterdir()) if p.is_file()}
        save(output/'receipt.json', receipt)
    print(json.dumps(dict(status=receipt['status'], output=str(output), receipt_sha256=sha(output/'receipt.json'),
                          commands=len(receipt['commands']), pins=receipt.get('pin_count'),
                          elapsed_seconds=receipt['elapsed_seconds'])), flush=True)
    return 0 if receipt['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
