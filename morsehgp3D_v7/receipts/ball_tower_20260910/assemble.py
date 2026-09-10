#!/usr/bin/env python3
"""Assemble existing, closed local captures without rerunning any benchmark."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_ball_publication_20260910'
PACKET = BASE / 'packet'
MEB = ROOT / 'build/v7_anchor_meb_20260910'
GATE = ROOT / 'build/v7_ball_tower_gate_20260910'
MONO = ROOT / 'build/v7_ball_tower_mono_20260910'
WORK = ROOT / 'build/v7_ball_work_gate_20260910'
PROBE = ROOT / 'build/v7_ball_probe_20260910/baseline_r1'
CODE = {'.hpp', '.cpp', '.cuh', '.cu', '.py'}
RAW = {'.json', '.stdout', '.stderr', '.d', '.patch'}
groups = {
    **{'meb_' + mode: MEB / ('run_' + mode) for mode in ('o2', 'san', 'mutants')},
    'full_gate_r3': GATE / 'run_r3',
    **{'mono_' + mode: MONO / ('run_' + mode)
       for mode in ('o2', 'san', 'mutant', 'extended', 'monotone_o2',
                    'monotone_san', 'monotone_mutant')},
    **{'work_' + mode: WORK / ('run_' + mode) for mode in ('o2', 'san')},
    'worker_pure': GATE / 'worker_checks_r1',
    'baseline_8k_failed': PROBE,
}


def need(ok: bool, why: str) -> None:
    if not ok:
        raise ValueError(why)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_json(path: Path):
    return json.loads(path.read_text())


def write_json(path: Path, data) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')


origins: dict[str, set[str]] = {}


def add_source(path: Path, expected: str | None = None) -> str:
    data = path.read_bytes()
    need(not data.startswith(b'\x7fELF'), 'ELF source: ' + str(path))
    need(b'\0' not in data, 'binary source: ' + str(path))
    sha = digest(data)
    need(expected is None or sha == expected, 'source pin mismatch: ' + str(path))
    target = PACKET / 'objects' / sha
    if not target.exists():
        target.write_bytes(data)
    origins.setdefault(sha, set()).add(path.relative_to(ROOT).as_posix())
    return sha


def regular_code_files(directory: Path):
    # Deliberately do not traverse private variants' links into the live tree.
    for current, directories, files in os.walk(directory, followlinks=False):
        directories[:] = sorted(name for name in directories
                                if not (Path(current) / name).is_symlink())
        for name in sorted(files):
            path = Path(current) / name
            if not path.is_symlink() and path.suffix in CODE:
                yield path


def main() -> None:
    need(not PACKET.exists(), 'refuse to overwrite an existing packet')
    need(read_json(PROBE / 'receipt.json')['all_processes_closed'] is True,
         'baseline probe still active')
    (PACKET / 'objects').mkdir(parents=True)
    for group, directory in groups.items():
        target = PACKET / 'captures' / group
        target.mkdir(parents=True)
        for path in sorted(directory.iterdir()):
            if path.is_file() and not path.is_symlink() and path.suffix in RAW:
                data = path.read_bytes()
                need(not data.startswith(b'\x7fELF') and b'\0' not in data,
                     'nontext capture: ' + str(path))
                (target / path.name).write_bytes(data)
        for path in regular_code_files(directory):
            add_source(path)
    for variant in ('baseline', 'candidate', 'extended', 'monotone', 'mutant',
                    'monotone_future_mutant'):
        for path in regular_code_files(MONO / variant):
            add_source(path)
    for directory in (MEB, MONO, WORK):
        for path in sorted(directory.glob('*.py')):
            add_source(path)

    work_receipt = read_json(WORK / 'run_o2/receipt.json')
    work_map = work_receipt['sources_before']
    need(work_map == work_receipt['sources_after'], 'work O2 drift')
    need(read_json(WORK / 'run_san/receipt.json')['sources_before'] == work_map,
         'work O2/SAN source mismatch')
    for name, sha in work_map.items():
        if not (PACKET / 'objects' / sha).exists():
            add_source(ROOT / name, sha)

    # Supplement pre-existing MEB dependency lists from pinned work-gate bytes.
    # This is a packaging-time supplement, not an invented earlier antidrift check.
    meb_sources = read_json(MEB / 'run_o2/receipt.json')['sources']
    dependencies = (MEB / 'run_o2/dependencies.d').read_text().replace('\\\n', ' ')
    meb_map = {}
    for value in dependencies.split(':', 1)[1].split():
        logical = Path(os.path.normpath(value)).relative_to(ROOT).as_posix()
        sha = meb_sources.get(logical, work_map.get(logical))
        need(sha is not None, 'unresolved MEB dependency: ' + logical)
        need((PACKET / 'objects' / sha).is_file(), 'missing MEB dependency bytes')
        meb_map[logical] = sha

    header = 'morsehgp3D_v7/src/forest/full_ball_tower.hpp'
    gate = 'morsehgp3D_v7/tests/full_ball_tower_gate.cpp'
    meb_header = 'morsehgp3D_v7/src/forest/anchor_meb.hpp'
    views = {
        'meb_nominal': {'files': meb_map, 'provenance':
            'new header/gate pinned at start; dependency supplement from work-gate pins'},
        'full_baseline': {'files': read_json(GATE / 'run_r3/sources.json'),
                          'provenance': 'isolated run_r3 snapshot and original source hashes'},
        'work_active': {'files': {name: sha for name, sha in work_map.items()
                                 if name.startswith('morsehgp3D_v7/')},
                        'provenance': 'original before/after equal in both active work gates'},
        'baseline_8k_failed': {'files': read_json(PROBE / 'sources_before.json'),
                               'provenance': 'original before pins only; run failed with exit 143'},
    }
    for mutant in ('accept_outside', 'shell_is_support', 'reject_extra_shell'):
        views['meb_' + mutant] = {'extends': 'meb_nominal', 'files': {
            meb_header: add_source(MEB / 'run_mutants' / mutant / 'src/forest/anchor_meb.hpp')},
            'provenance': 'retained mutant source; exact mutation checked against recorder'}
    full_result = read_json(GATE / 'run_r3/result.json')
    for mutant, info in full_result['mutants'].items():
        path = GATE / 'run_r3/mutants' / mutant / header
        views['full_' + mutant] = {'extends': 'full_baseline', 'files': {
            header: add_source(path, info['header_sha256'])}}
    views['mono_baseline'] = {'extends': 'full_baseline', 'files': {
        gate: add_source(MONO / 'baseline/tests/full_ball_tower_gate.cpp')}}
    views['mono_candidate'] = {'extends': 'mono_baseline', 'files': {
        header: add_source(MONO / 'candidate/src/forest/full_ball_tower.hpp'),
        'morsehgp3D_v7/tests/mono_counters_gate.cpp':
            add_source(MONO / 'candidate/tests/mono_counters_gate.cpp')}}
    views['mono_candidate_extended'] = {'extends': 'mono_candidate', 'files': {
        gate: add_source(MONO / 'extended/tests/full_ball_tower_gate.cpp')}}
    views['mono_monotone'] = {'extends': 'mono_candidate_extended', 'files': {
        header: add_source(MONO / 'monotone/src/forest/full_ball_tower.hpp'),
        'morsehgp3D_v7/tests/monotone_history_gate.cpp':
            add_source(MONO / 'monotone/tests/monotone_history_gate.cpp')}}
    for variant, parent in (('mutant', 'mono_candidate'),
                            ('monotone_future_mutant', 'mono_monotone')):
        views['mono_' + variant] = {'extends': parent, 'files': {
            header: add_source(MONO / variant / 'src/forest/full_ball_tower.hpp')}}
    worker_sources = read_json(GATE / 'worker_checks_r1/receipt.json')['sources']
    views['worker_pure'] = {'files': {'gcp-migration/' + name: sha
                                    for name, sha in worker_sources.items()}}
    write_json(PACKET / 'views.json', views)
    write_json(PACKET / 'source_origins.json',
               {sha: sorted(paths) for sha, paths in sorted(origins.items())})

    metadata = PACKET / 'metadata'
    metadata.mkdir()
    for name in ('baseline_sources.json', 'candidate.patch', 'monotone.patch',
                 'combined.patch', 'summary.json', 'monotone_summary.json'):
        shutil.copyfile(MONO / name, metadata / name)
    notes = PACKET / 'notes'
    notes.mkdir()
    for label, source in (('meb_original', MEB / 'README.md'),
                          ('full_gate_original', GATE / 'README.md'),
                          ('mono_original', MONO / 'README.md')):
        shutil.copyfile(source, notes / (label + '.md.txt'))
    # No raw CUDA host capture is promoted: it has no immutable recorder.
    shutil.copyfile(BASE / 'README.packet.md', PACKET / 'README.md')
    shutil.copyfile(BASE / 'verify.py', PACKET / 'verify.py')
    shutil.copyfile(BASE / 'assemble.py', PACKET / 'assemble.py')
    write_json(PACKET / 'capture_index.json', {
        'schema': 'v7-ball-local-publication-v1',
        'public_status': 'not_claimed', 'GCP_used': False, 'CUDA_executed': False,
        'groups': {name: path.relative_to(ROOT).as_posix() for name, path in groups.items()},
        'excluded': ['active monotone_r1 performance capture',
                     'CUDA host raw results without a proper recorder',
                     'ELF executables and all system/Boost headers',
                     'GCP session evidence and coverage-parent packet published separately'],
        'known_unavailable_source_pin': {
            'group': 'mono_o2', 'field': 'sources',
            'name': 'candidate/tests/mono_counters_gate.cpp',
            'sha256': 'bddcd449ff4e013ba681184afb8d7597b178762b9d7da40670d25976e22f5a2b',
            'reason': 'signedness fix after initial pin, before compilation; not an immutable-source validation'},
    })
    files = {path.relative_to(PACKET).as_posix(): digest(path.read_bytes())
             for path in sorted(PACKET.rglob('*')) if path.is_file()}
    write_json(PACKET / 'manifest.json', files)
    print(json.dumps({'status': 'assembled_not_yet_verified', 'files': len(files),
                      'source_objects': len(origins), 'views': len(views),
                      'bytes': sum(path.stat().st_size for path in PACKET.rglob('*')
                                   if path.is_file())}, sort_keys=True))


if __name__ == '__main__':
    main()
