#!/usr/bin/env python3
"""Additive publication of closed local captures; only offline verification runs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'build/v7_extra_shell_20260906'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_extra_shell_50000_20260906'
OLD = DEST.parent / 'full_census_payload_20260906'
PINS = {
    'run_r1': 'd835d6c2c8bbc31a73a0028f6d1611bde352999bfe70f93290d4f6dfef045092',
    'run_r2': '505c39961c2d3beca5b2a7a08d6e9575ad2df0baa3ee4d5674f7b62b851b3f70',
    'run_r3': 'f24c397a98ce3ff09e55c9fad01fbecf763878d483dcd59a4dea973518f0e298',
    'ctest_r1': '5dee16d27d55a3bbfc5be65ead64dd6ca75fa0ae583f0cc3058e8d9284deeee7',
    'ctest_r1/metadata_close': 'bcc575c6e7c0f4d11b0554f337a96158ba12400ddede1ba8823e56e91255f59a',
}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    return json.loads(path.read_text())


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def copy(source, name):
    need(source.is_file() and not source.is_symlink(), 'regular source: ' + str(source))
    dest = DEST / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    with source.open('rb') as src, dest.open('xb') as dst:
        shutil.copyfileobj(src, dst)
    need(sha(source) == sha(dest), 'copy mismatch')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args()
    for name, pin in PINS.items():
        need(sha(BASE / name / 'receipt.json') == pin, 'receipt pin: ' + name)
    need(sha(BASE / 'ctest_r1/source_controls/CMakeLists.txt') ==
         '15b800e374dbc938e6feff820ee28fc60bd2d775be622fe51304885fe10d86da', 'captured CMake')
    if not args.execute:
        print(json.dumps({'status': 'prepared_no_writes', 'destination': str(DEST)}))
        return
    need(not DEST.is_symlink(), 'destination symlink')
    if DEST.exists():
        need({p.name for p in DEST.iterdir()} <= {'reader'} and
             not (DEST / 'reader').is_symlink(), 'base publication already exists')
    else:
        DEST.mkdir(parents=True)
    old = load(OLD / 'publication.json')['source_references']
    old_by_name = {}
    for name, row in old.items():
        if 'morsehgp3D_v7/' in name:
            old_by_name[name[name.index('morsehgp3D_v7/'):]] = (OLD / row['relative_path'], row['sha256'])
    deps = load(BASE / 'run_r3/dependencies.json')
    need(len(deps) == 43, '43 dependencies')
    compiled = load(BASE / 'ctest_r1/metadata_close/compiled_sources_before.json')
    required = dict(deps)
    for name, pin in compiled.items():
        need(name not in required or required[name] == pin, 'source conflict')
        required[name] = pin
    locations = {}
    for name, pin in sorted(required.items()):
        known = old_by_name.get(name)
        original_copy = OLD / 'run_r1/sources' / name
        if known and known[1] == pin:
            source = known[0].resolve()
        elif original_copy.is_file() and sha(original_copy) == pin:
            source = original_copy
        elif name == 'morsehgp3D_v7/cmake/run_expect.cmake':
            source = DEST.parent / 'full_probe_no_quotas_20260906/source_snapshot' / name
        else:
            source = BASE / 'run_r3/sources' / name
            if name == 'morsehgp3D_v7/CMakeLists.txt':
                source = BASE / 'ctest_r1/source_controls/CMakeLists.txt'
            need(sha(source) == pin, 'new source pin')
            copy(source, 'source_snapshot/' + name)
            source = DEST / 'source_snapshot' / name
        need(source.resolve().is_relative_to(DEST.parent) and sha(source) == pin, 'sealed source reference')
        locations[name] = {'relative_path': os.path.relpath(source, DEST), 'sha256': pin}
    omitted = {}
    for run in ('run_r1', 'run_r2', 'run_r3', 'ctest_r1'):
        for source in sorted((BASE / run).rglob('*')):
            if not source.is_file():
                continue
            relative = source.relative_to(BASE / run).as_posix()
            name = run + '/' + relative
            if source.name in ('gate_O2', 'gate_SAN', 'probe_O3'):
                omitted[name] = {'sha256': sha(source), 'reason': 'ELF_not_published'}
            elif source.name.endswith('.intent.json') and source.with_name(source.name.replace('.intent.json', '.command.json')).exists():
                omitted[name] = {'sha256': sha(source), 'reason': 'redundant_intent_command_retained'}
            elif relative.startswith('sources/') or relative.startswith('source_controls/'):
                repo_name = (relative[len('sources/'):] if relative.startswith('sources/') else
                             'morsehgp3D_v7/' + relative[len('source_controls/'):])
                row = locations[repo_name]
                need(sha(source) == row['sha256'], 'deduplicated source')
                omitted[name] = {'sha256': row['sha256'], 'reason': 'source_deduplicated',
                                 'replacement': row['relative_path']}
            elif '__pycache__' not in source.parts:
                copy(source, name)
    copy(BASE / 'record.py', 'protocols/record_r3.py')
    copy(BASE / 'verify_publication.py', 'verify.py')
    copy(Path(__file__), 'protocols/publish.py')
    copy(BASE / 'publication_README.md', 'README.md')
    artifacts = {p.relative_to(DEST).as_posix(): sha(p) for p in sorted(DEST.rglob('*'))
                 if p.is_file() and p.relative_to(DEST).parts[0] != 'reader'}
    save(DEST / 'publication.json', {
        'schema': 'mhgp7-extra-shell-publication-v1', 'status': 'closed_captures_preserved',
        'public_status': 'not_claimed', 'actual_GCP_used': False,
        'performance_contract_certified': False, 'source_locations': locations,
        'payload_artifacts': artifacts, 'omitted_artifacts': omitted,
        'receipt_sha256': PINS, 'probe_dependency_count': 43,
        'historical_r1_recorder_source_available': False, 'historical_r1_gate_source_available': False,
        'recorder_copy_scope': 'r3 protocol only; not substituted for unavailable historical r1 recorder',
        'recorded_absolute_paths': 'historical metadata, not required for offline verification',
        'manifest_scope': 'BASE_SHA256SUMS excludes reader/; ROOT closes global manifest',
    })
    checks = DEST / 'offline_checks'
    checks.mkdir()
    results = []
    for mode, flags in [('normal', []), ('optimized', ['-O'])]:
        argv = [sys.executable, '-B', *flags, 'verify.py']
        start = time.time_ns()
        with (checks / (mode + '.stdout')).open('xb') as out, (checks / (mode + '.stderr')).open('xb') as err:
            child = subprocess.Popen(argv, cwd=DEST, stdout=out, stderr=err)
            code = child.wait()
        row = {'argv': argv, 'cwd': 'receipt-root', 'pid': child.pid, 'exit_code': code,
               'started_ns': start, 'ended_ns': time.time_ns(),
               'stdout_sha256': sha(checks / (mode + '.stdout')),
               'stderr_sha256': sha(checks / (mode + '.stderr')), 'engine_run': False}
        save(checks / (mode + '.command.json'), row)
        results.append(row)
    save(checks / 'receipt.json', {'status': 'completed' if all(r['exit_code'] == 0 for r in results) else 'failed',
                                 'commands': results, 'new_engine_runs': 0})
    files = sorted(p for p in DEST.rglob('*') if p.is_file() and p.relative_to(DEST).parts[0] != 'reader')
    with (DEST / 'BASE_SHA256SUMS').open('x') as stream:
        stream.writelines(sha(p) + '  ' + p.relative_to(DEST).as_posix() + '\n' for p in files)
    need(all(r['exit_code'] == 0 for r in results), 'offline verifier failed; captures preserved')
    print(json.dumps({'status': 'published', 'base_files': len(files) + 1, 'sources': len(locations),
                      'BASE_SHA256SUMS': sha(DEST / 'BASE_SHA256SUMS'), 'publication_sha256': sha(DEST / 'publication.json')}))


if __name__ == '__main__':
    main()
