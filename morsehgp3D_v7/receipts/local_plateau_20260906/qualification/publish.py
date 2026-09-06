#!/usr/bin/env python3
"""Mechanical, create-only preservation of the qualified local component."""
import hashlib
import json
import os
from pathlib import Path
import shutil

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
DEST = ROOT / 'morsehgp3D_v7/receipts/local_plateau_20260906'
DIAG = DEST.parent / 'full_extra_shell_50000_20260906'
CMAKE = ROOT / 'build/v7_local_plateau_cmake_20260906'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text())


def copy(source, relative):
    target = DEST / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    with source.open('rb') as src, target.open('xb') as dst:
        shutil.copyfileobj(src, dst)
    if sha(source) != sha(target):
        raise ValueError('copy mismatch')


def main():
    for name in ('prepare', 'san'):
        record = read(BASE / (name + '.receipt.json'))
        if record['status'] != 'completed' or not record['cpp_closed']:
            raise ValueError('qualification not closed')
    if read(CMAKE / 'run_r1/receipt.json')['status'] != 'completed':
        raise ValueError('CTest not closed')
    DEST.mkdir()
    for path in sorted(BASE.iterdir()):
        if path.is_file() and not path.name.endswith('.intent.json') and path.name not in (
                'CONTRAT_LOCAL.md', 'publication_README.md', 'verify_publication.py', 'seal.py'):
            copy(path, 'qualification/' + path.name)
    for path in sorted((BASE / 'source_snapshot').rglob('*')):
        if path.is_file():
            copy(path, 'source_snapshot/morsehgp3D_v7/' + path.relative_to(BASE / 'source_snapshot').as_posix())
    copy(BASE / 'mutant/src/forest/local_plateau.hpp', 'mutant/local_plateau.hpp')
    for path in sorted((CMAKE / 'run_r1').rglob('*')):
        if path.is_file():
            copy(path, 'ctest/' + path.relative_to(CMAKE / 'run_r1').as_posix())
    copy(CMAKE / 'capture.py', 'ctest/capture.py')
    common = read(DIAG / 'publication.json')['source_locations']
    deps = read(BASE / 'O2.sources_before.json')
    deps.update({str(ROOT / name): pin for name, pin in read(CMAKE / 'run_r1/project_sources.json').items()})
    sources = {}
    for absolute, pin in sorted(deps.items()):
        path = Path(absolute)
        if not path.is_relative_to(ROOT / 'morsehgp3D_v7'):
            continue
        name = path.relative_to(ROOT).as_posix()
        if name in common and common[name]['sha256'] == pin:
            source = DIAG / common[name]['relative_path']
        else:
            source = DEST / 'source_snapshot' / name
            if not source.exists():
                if sha(path) != pin:
                    raise ValueError('changed source before publication')
                copy(path, 'source_snapshot/' + name)
        if sha(source) != pin:
            raise ValueError('source reference mismatch')
        sources[name] = dict(relative_path=os.path.relpath(source, DEST), sha256=pin)
    # Historical prototypes are not substituted for the promoted sources.
    history = {}
    for version in (1, 2, 3):
        directory = ROOT / ('build/v7_local_plateau_20260906_qualification_r' + str(version))
        omitted = {}
        for path in sorted(directory.rglob('*')):
            if not path.is_file():
                continue
            name = path.relative_to(directory).as_posix()
            if path.is_symlink():
                omitted[name] = dict(sha256=sha(path), reason='shared_product_dependency')
            elif name.startswith('bin/') or path.name.endswith('.intent.json'):
                omitted[name] = dict(sha256=sha(path), reason='ELF_or_redundant_intent')
            else:
                copy(path, 'history/r' + str(version) + '/' + name)
        history[str(version)] = omitted
    copy(BASE / 'CONTRAT_LOCAL.md', 'CONTRAT_LOCAL.md')
    copy(BASE / 'publication_README.md', 'README.md')
    copy(BASE / 'verify_publication.py', 'verify.py')
    files = {path.relative_to(DEST).as_posix(): sha(path) for path in sorted(DEST.rglob('*')) if path.is_file()}
    publication = dict(schema='mhgp7-local-plateau-publication-v1', files=files,
        sources=sources, omitted_history=history, GCP_used=False, FULL_integrated=False,
        public_status='not_claimed', prototype_r4='prepared_only_approval_cancelled_no_execution',
        real_trace=dict(relative_path='../full_extra_shell_50000_20260906/run_r3/n50000_k10.stderr',
                        sha256='3cd74b330c62978d8c3eedd175e12bf5fe02893facb2e008150c32b5054aea72'),
        real_judge=dict(relative_path='../full_extra_shell_50000_20260906/reader/checks/normal.stdout',
                        sha256='c4a066e620b7850b6b3f1937f5b6d92b027f763012a554f9d1fbbf5512cc3c81'))
    with (DEST / 'publication.json').open('x') as stream:
        json.dump(publication, stream, sort_keys=True, indent=2)
        stream.write('\n')
    print('published', len(files), 'files,', len(sources), 'project source references')


if __name__ == '__main__':
    main()
