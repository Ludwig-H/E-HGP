#!/usr/bin/env python3
"""Lecture des commits et journaux uniquement ; aucun natif, build ni CTest."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
DELIVERY = '8ba7d728711995e73913487adb46dea9e7dcc2a6'
PROTOTYPE = '66c41ede461dcc7a235a2414079c916841c4fb46'
DELIVERY_B = 'c903774b1d16c3b18c15b86fa2a55d11d44b8bc8'
PROTOTYPE_B = '3b445b763057adc405141a6650efa14bfd4e276a'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def git(repo, *args):
    return subprocess.check_output(['git', *args], cwd=repo)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--main', required=True, type=Path)
    p.add_argument('--scratch', required=True, type=Path)
    args = p.parse_args()
    prototype = args.scratch / 'v12_gpu/repo99'
    capture = json.loads((HERE / 'capture.json').read_text())
    def pins():
        for rel, sha in capture['artifact_hashes'].items():
            need(hashlib.sha256((args.scratch / rel).read_bytes()).hexdigest() == sha, 'hash: ' + rel)
    pins()
    changed = git(args.main, 'diff-tree', '--no-commit-id', '--name-only', '-r', DELIVERY).decode().splitlines()
    report = 'morsehgp3D_v12/receipts/developpement_20261007/catalogue_gpu_T1b_RAPPORT.md'
    removed = ['morsehgp3D_v12/src/catalogue/sort.cpp', 'morsehgp3D_v12/src/catalogue/sort.hpp']
    present = [f for f in changed if f not in removed + [report]]
    need(len(changed) == 46 and len(present) == 43, 'delivery inventory')
    for rel in present:
        need(git(args.main, 'show', DELIVERY + ':' + rel) == git(prototype, 'show', PROTOTYPE + ':' + rel), rel)
    for repo, pin in ((args.main, DELIVERY), (prototype, PROTOTYPE)):
        paths = set(git(repo, 'ls-tree', '-r', '--name-only', pin).decode().splitlines())
        need(all(f not in paths for f in removed), 'deleted serial sort')
    modules = {}
    for module in ('core', 'catalogue'):
        rel = 'morsehgp3D_v12/src/' + module
        a = git(prototype, 'ls-tree', '-r', PROTOTYPE, '--', rel)
        need(a == git(args.main, 'ls-tree', '-r', DELIVERY, '--', rel), 'whole module: ' + module)
        modules[module] = len(a.splitlines())
    reasons = 'morsehgp3D_v12/src/core/reasons.def'
    rows = lambda text: re.findall(r'^MHGP12_REASON\(([^)]+)\)', text, re.M)
    before = rows(git(args.main, 'show', DELIVERY + '^:' + reasons).decode())
    after = rows(git(args.main, 'show', DELIVERY + ':' + reasons).decode())
    need(after[:len(before)] == before and len(before) == 29, 'old reason order preserved')
    need(after[29:] == ['device_unavailable, resource_exhausted, catalogue',
                       'device_fault, invariant_violated, catalogue'], 'new reasons appended')
    log = (args.scratch / 'build_v12_u21.ctest_gpuA.log').read_text()
    cases = re.findall(r'^[ \t]*\d+/687 Test\s+#\d+: (\S+)\s+\.+\s*(Passed|\*\*\*Skipped)\s+', log, re.M)
    need(len(cases) == 687 and len({x[0] for x in cases}) == 687, 'fast cohort')
    skipped = [name for name, status in cases if status != 'Passed']
    need(skipped == ['mhgp12_support_lidar_sentinel'], 'one skipped sentinel')
    for name in ('mhgp12_core_unit_block_cache_limit', 'mhgp12_core_unit_reasons',
                 'mhgp12_catalogue_device_unit_pipeline_witnesses', 'mhgp12_catalogue_device_unit_device_open'):
        need((name, 'Passed') in cases, 'required gate: ' + name)
    build = (args.scratch / 'build_v12_u21.build_gpuA.log').read_text()
    need('device_stub.cpp.o' in build and 'device_cuda.cu.o' not in build, 'CPU-only build')
    need('warning:' not in build, 'build warnings')
    configure = (args.scratch / 'build_v12_u21.conf_gpuA.log').read_text()
    need('bits = 21' in configure and '717 portes enregistrees' in configure, 'configuration')
    archive = git(prototype, 'archive', PROTOTYPE, 'morsehgp3D_v12')
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        files = {m.name.removeprefix('morsehgp3D_v12/'): tar.extractfile(m).read() for m in tar if m.isfile()}
    h = hashlib.sha256()
    total = 0
    for top in ('CMakeLists.txt', 'cmake', 'src', 'cli', 'bench', 'tests', 'tools', 'reference', 'docs'):
        for rel in sorted(f for f in files if (f == top or f.startswith(top + '/'))
                          and '/__pycache__/' not in f and not f.endswith('.pyc')):
            h.update(rel.encode() + b'\0')
            h.update(hashlib.sha256(files[rel]).hexdigest().encode() + b'\n')
            total += 1
    campaigns = {}
    for module, filename, n in (('catalogue', 'report.json', 15), ('core', 'report_core.json', 1)):
        r = json.loads((args.scratch / 'v12_gpu/mutA3' / filename).read_text())
        need(r['sources_sha256'] == h.hexdigest(), 'mutants exact A tree')
        need(r['manifeste_sha256'] == hashlib.sha256(files['tests/mutants/' + module + '.json']).hexdigest(),
             'mutant manifest')
        need(r['code'] == 0 and r['temoin'] == 'vert' and len(r['mutants']) == n, 'mutant campaign')
        need(all(x['verdict'] == 'TUE' for x in r['mutants']), 'all killed')
        campaigns[module] = {cause: sum(x['detail'] == cause for x in r['mutants'])
                             for cause in ('code', 'signal', 'delai', 'construction')}
    need(campaigns['catalogue'] == {'code': 14, 'signal': 1, 'delai': 0, 'construction': 0}, 'catalogue causes')
    need(campaigns['core'] == {'code': 1, 'signal': 0, 'delai': 0, 'construction': 0}, 'swap mutant cause')
    # Ajout distinct apres cloture de A : identite des sources B seulement, aucune campagne rejouee.
    bfiles = git(args.main, 'diff-tree', '--no-commit-id', '--name-only', '-r', DELIVERY_B).decode().splitlines()
    need(bfiles == ['morsehgp3D_v12/src/catalogue/' + name for name in
                    ('leaf_census.hpp', 'leaf_common.hpp', 'leaf_j3.hpp', 'simt.hpp')], 'B inventory')
    for rel in bfiles:
        need(git(args.main, 'show', DELIVERY_B + ':' + rel) == git(prototype, 'show', PROTOTYPE_B + ':' + rel),
             'B changed source: ' + rel)
    bmodules = {}
    for module in ('core', 'catalogue'):
        rel = 'morsehgp3D_v12/src/' + module
        a = git(prototype, 'ls-tree', '-r', PROTOTYPE_B, '--', rel)
        need(a == git(args.main, 'ls-tree', '-r', DELIVERY_B, '--', rel), 'B whole module: ' + module)
        bmodules[module] = len(a.splitlines())
    pins()
    print(json.dumps({'status': 'ok', 'delivery': DELIVERY, 'prototype': PROTOTYPE,
        'changed_files_identical_to_A': 43, 'deleted_in_both': 2, 'whole_modules_identical': modules,
        'old_reasons_preserved': 29, 'new_reason_indices': {'device_unavailable': 29, 'device_fault': 30},
        'fast_cpu': {'selected': 687, 'passed': 686, 'skipped': skipped, 'cuda_enabled': False, 'bits': 21},
        'A_mutants': campaigns, 'A_tree_files': total, 'A_tree_sha256': h.hexdigest(),
        'B_source_identity_only': {'delivery': DELIVERY_B, 'prototype': PROTOTYPE_B,
            'changed_files_identical': len(bfiles), 'whole_modules_identical': bmodules,
            'new_timing_qualification': False, 'new_native_campaign': False},
        'artifact_hashes_verified_before_after': len(capture['artifact_hashes']),
        'native_executed_by_auditor': False}, indent=2, sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
