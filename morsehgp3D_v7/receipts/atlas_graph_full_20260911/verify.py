#!/usr/bin/env python3
"""One-parent differential reader. Read-only unless --extract is explicit."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath

BASE_MANIFEST = '341c8c228a9d008084010db7b010adac77beefba123fa8a59d1d7c9023652013'


def need(value, why):
    if not value:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    need(isinstance(name, str) and name != '', 'empty path')
    p = PurePosixPath(name)
    need(not p.is_absolute() and all(part not in ('..', '.') for part in p.parts) and
         str(p) == name and '\\' not in name, f'unsafe path:{name}')
    return p


class Reader:
    def __init__(self, package):
        self.package = Path(package).resolve()
        self.manifest = json.loads((self.package / 'MANIFEST.json').read_text())
        need(self.manifest['format'] == 'atlas_graph_full_one_parent_v1', 'format')
        parent = self.manifest['parent']
        need(parent == {'directory': '../rank_atlas_20260911', 'manifest': 'manifest.json',
                       'sha256': BASE_MANIFEST}, 'single explicit parent')
        self.parent = self.package.parent / 'rank_atlas_20260911'
        data = (self.parent / 'manifest.json').read_bytes()
        need(sha(data) == BASE_MANIFEST, 'parent manifest absent or changed')
        self.parent_files = json.loads(data)['files']

    def bytes(self, logical):
        safe(logical)
        row = self.manifest['files'][logical]
        digest = row['sha256']
        need(len(digest) == 64 and all(c in '0123456789abcdef' for c in digest), 'sha256 format')
        if row['provider'] == 'rank_atlas':
            path = row['path']
            safe(path)
            need(path.startswith('source/') or path == 'rank_atlas.hpp', 'parent scope')
            need(self.parent_files.get(path) == digest, 'parent path/hash binding')
            target = self.parent / path
        else:
            need(row['provider'] == 'object' and row['path'] == 'objects/' + digest, 'local object binding')
            target = self.package / row['path']
        need(target.is_file() and not target.is_symlink(), f'not regular:{target}')
        data = target.read_bytes()
        need(len(data) == row['size'] and sha(data) == digest, f'bytes:{logical}')
        need(data[:4] != b'\x7fELF', f'ELF forbidden:{logical}')
        return data

    def json(self, logical):
        return json.loads(self.bytes(logical))

    def verify(self):
        need(set(self.manifest['package_files']) == {'README.md', 'verify.py', 'qualification.py', 'freeze.json'},
             'exact package source set')
        for name, digest in self.manifest['package_files'].items():
            safe(name)
            target = self.package / name
            need(target.is_file() and not target.is_symlink() and sha(target.read_bytes()) == digest,
                 f'package source:{name}')
        freeze = json.loads((self.package / 'freeze.json').read_text())
        need(set(freeze) == {'files', 'omitted_elf', 'parent_sha256', 'root_approval_required', 'note'} and
             freeze['parent_sha256'] == BASE_MANIFEST and freeze['root_approval_required'] is True, 'freeze schema')
        need(set(freeze['files']) == set(self.manifest['files']) and
             freeze['omitted_elf'] == self.manifest['omitted_elf'], 'freeze exact logical set')
        roots = ('build/v7_atlas_graph_20260911/', 'build/v7_composable_msf_20260911/', 'build/v7_graph_full_20260911/')
        alias = 'build/v7_filtered_calendar_20260911/filtered_calendar.hpp'
        for group in ('files', 'omitted_elf'):
            for logical, row in freeze[group].items():
                safe(logical)
                need(set(row) == {'source', 'sha256', 'size'} and
                     (logical.startswith(roots) or (group == 'files' and logical == alias)), 'freeze source scope')
                need(row['source'] == (roots[0] + 'filtered_calendar.hpp' if logical == alias else logical),
                     'freeze no hidden source redirection')
                if group == 'files':
                    actual = self.manifest['files'][logical]
                    need(row['sha256'] == actual['sha256'] and row['size'] == actual['size'], 'freeze content pin')
        expected_files = {'MANIFEST.json', *self.manifest['package_files']}
        expected_files.update(row['path'] for row in self.manifest['files'].values() if row['provider'] == 'object')
        actual_files = set()
        for path in self.package.rglob('*'):
            need(not path.is_symlink(), 'physical symlink forbidden')
            if path.is_file():
                actual_files.add(path.relative_to(self.package).as_posix())
        need(actual_files == expected_files, 'exact physical inventory, no hidden/unreferenced object')
        borrowed = 0
        for logical, row in self.manifest['files'].items():
            self.bytes(logical)
            borrowed += row['provider'] == 'rank_atlas'
        need(borrowed > 0, 'borrowed source non-vacuity')
        for logical, row in self.manifest['omitted_elf'].items():
            safe(logical)
            need(logical not in self.manifest['files'] and row['size'] > 0 and len(row['sha256']) == 64,
                 'ELF omission attestation')
        spec = importlib.util.spec_from_file_location('atlas_graph_qualification', self.package / 'qualification.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        result = module.verify(self)
        return {'status': 'passed', 'logical_files': len(self.manifest['files']),
                'borrowed_files': borrowed, 'gcp_used': False, 'qualification': result}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--extract', type=Path)
    args = parser.parse_args()
    reader = Reader(Path(__file__).resolve().parent)
    result = reader.verify()
    if args.extract:
        destination = args.extract.resolve()
        need(not destination.exists(), 'extract destination must be new')
        destination.mkdir(parents=True, exist_ok=False)
        for logical in reader.manifest['files']:
            target = destination / safe(logical)
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write(reader.bytes(logical))
        result['extracted_to'] = str(destination)
        result['standalone_source_tree'] = True
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
