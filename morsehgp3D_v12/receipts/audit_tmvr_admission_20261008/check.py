#!/usr/bin/env python3
"""Sources, rapport mutant et application textuelle ; aucun moteur ni payload."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(root, repository, capture):
    prior = HERE.parent / 'audit_tmv_traces_20261007/check.py'
    require(sha(prior) == capture['prior_reader_sha256'], 'lecteur arbre modifie')
    tree = runpy.run_path(str(prior))['tree']
    trees = {name: list(tree(root / name / 'morsehgp3D_v12')) for name in ('repo3', 'repo4', 'repo5')}
    require(trees == capture['trees'], 'sources differentes')
    report = json.loads((root / 'mutant_admission.json').read_text())
    require(report['sources_sha256'] == trees['repo4'][0], 'mutant sans son arbre repo4')
    require(report['manifeste_sha256'] == sha(root / 'repo4/morsehgp3D_v12/tests/mutants/tower.json'),
            'manifeste mutant different')
    require(report['code'] == 0 and report['temoin'] == 'vert' and report['mutants'] == [dict(
        id='admission_r_sans_decalages', juge='mhgp12_tower_forest_admission', detail='code', verdict='TUE')],
        'rapport mutant different')
    patch = (root / 'patch_tour_TMV.diff').resolve()
    paths = [r.split('\t', 2)[2] for r in subprocess.check_output(
        ['git', 'apply', '--numstat', str(patch)], text=True).splitlines()]
    require(len(paths) == len(set(paths)), 'chemin duplique')
    require(all(p.startswith('morsehgp3D_v12/') and '..' not in Path(p).parts for p in paths), 'chemin interdit')
    changed = sorted(rel for rel in paths if sha(root / 'repo3' / rel) != sha(root / 'repo5' / rel))
    base = capture['patch_base']
    tracked = set(subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', base],
                                         cwd=repository, text=True).splitlines())
    digest = hashlib.sha256()
    with tempfile.TemporaryDirectory(prefix='audit-tmvr-admission-') as directory:
        target = Path(directory)
        for rel in paths:
            if rel in tracked:
                dest = target / rel
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(subprocess.check_output(['git', 'show', f'{base}:{rel}'], cwd=repository))
        subprocess.run(['git', 'apply', '--check', str(patch)], cwd=target, check=True)
        subprocess.run(['git', 'apply', str(patch)], cwd=target, check=True)
        for rel in sorted(paths):
            actual = sha(target / rel)
            require(actual == sha(root / 'repo5' / rel), 'application differente : ' + rel)
            digest.update((rel + '\0' + actual + '\n').encode())
    after = {name: list(tree(root / name / 'morsehgp3D_v12')) for name in trees}
    require(after == trees, 'sources changees pendant lecture')
    return dict(patched_files=len(paths), patched_files_equal_repo5=len(paths),
                applied_files_sha256=digest.hexdigest(), mutant_repo4='TUE/code, temoin vert',
                changed_patch_files_since_repo3=changed, native_replayed=False, payloads_read=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prototype', type=Path, required=True)
    parser.add_argument('--git-repository', type=Path, required=True)
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    for rel, expected in capture['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier modifie : ' + rel)
    result = inspect(args.prototype, args.git_repository, capture)
    require(result == capture['result'], 'resultat different')
    for rel, expected in capture['files_sha256'].items():
        require(sha(args.prototype / rel) == expected, 'fichier modifie pendant lecture : ' + rel)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
