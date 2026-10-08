#!/usr/bin/env python3
"""Relecture de traces closes et patch documentaire ; aucun moteur ni build."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def main():
    here = Path(__file__).resolve().parent
    repo, snap = map(Path, sys.argv[1:])
    c = json.loads((here / 'capture.json').read_text())
    pin = c['git_commit']
    for collection in ('traces', 'mutant_primary'):
        for name, item in c[collection].items():
            need(sha((snap / name).read_bytes()) == item['sha256'], 'trace changée : ' + name)
    log = (snap / 'ctest2.log').read_text()
    need(len(re.findall(r'\sPassed\s', log)) == 753, 'passes')
    need(len(re.findall(r'\*\*\*Skipped', log)) == 1, 'sauts')
    need('100% tests passed, 0 tests failed out of 754' in log, 'clôture CTest')
    report = json.loads((snap / 'mut3.json').read_text())
    need(type(report['code']) is int and report['code'] == 0 and report['temoin'] == 'vert', 'témoin/code')
    need({m['id'] for m in report['mutants']} == {'fermeture_sans_attente', 'aide_sans_garde'} and
         len(report['mutants']) == 2, 'cohorte mutants')
    need(all(m['verdict'] == 'TUE' and m['detail'] == 'code' and
             m['juge'] == 'mhgp12_tower_region_fermeture' for m in report['mutants']), 'issues mutants')
    raw = (snap / 'source_inventory.json').read_bytes()
    need(sha(raw) == c['source_inventory_sha256'], 'inventaire')
    inventory = json.loads(raw)
    groups = ('CMakeLists.txt', 'cmake', 'src', 'cli', 'bench', 'tests', 'tools', 'reference', 'docs')
    digest = hashlib.sha256()
    changed = []
    for group in groups:
        for p in sorted(p for p in inventory if p == group or p.startswith(group + '/')):
            digest.update(p.encode() + b'\0' + inventory[p].encode() + b'\n')
            blob = subprocess.check_output(['git', 'show', pin + ':morsehgp3D_v12/' + p], cwd=repo)
            if sha(blob) != inventory[p]:
                changed.append(p)
    names = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', pin, 'morsehgp3D_v12/'],
                                    cwd=repo, text=True).splitlines()
    git_names = {n[len('morsehgp3D_v12/'):] for n in names
                 if n[len('morsehgp3D_v12/'):].split('/')[0] in groups}
    need(git_names == set(inventory), 'cohorte fichiers Git')
    need(inventory['tests/mutants/tower.json'] == report['manifeste_sha256'], 'manifeste mutant')
    need(digest.hexdigest() == report['sources_sha256'], 'source mutants')
    need(changed == c['git_comparison']['changed_git'] and len(inventory) == 413, 'écarts Git')
    for p, expected in c['source_pins'].items():
        blob = subprocess.check_output(['git', 'show', pin + ':morsehgp3D_v12/' + p], cwd=repo)
        need(sha(blob) == expected, 'raccord tour : ' + p)
    with tempfile.TemporaryDirectory(prefix='a6b_doc_') as folder:
        root = Path(folder)
        for p, hashes in c['documentary_patch'].items():
            b = subprocess.check_output(['git', 'show', pin + ':morsehgp3D_v12/' + p], cwd=repo)
            need(sha(b) == hashes['before'], 'préimage')
            f = root / 'morsehgp3D_v12' / p
            f.parent.mkdir(parents=True, exist_ok=True)
            f.write_bytes(b)
        subprocess.run(['git', 'apply', '--check', str(here / 'proposition.patch')], cwd=root, check=True)
        subprocess.run(['git', 'apply', str(here / 'proposition.patch')], cwd=root, check=True)
        for p, hashes in c['documentary_patch'].items():
            after = (root / 'morsehgp3D_v12' / p).read_bytes()
            need(sha(after) == hashes['after'], 'postimage')
            if p.startswith('src/'):
                before = subprocess.check_output(['git', 'show', pin + ':morsehgp3D_v12/' + p], cwd=repo)
                clean = lambda b: re.sub(r'\s+', '', re.sub(r'//[^\n]*|/\*.*?\*/', '', b.decode(), flags=re.S))
                need(clean(before) == clean(after), 'patch modifie le calcul produit')
    print(json.dumps({'ctest_passed': 753, 'ctest_skipped': 1, 'ctest_failed': 0,
                      'mutants_killed_by_code': 2, 'source_files': 413, 'git_equal_files': 408,
                      'native_invocations_by_audit': 0, 'documentary_patch_files': 4}, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
