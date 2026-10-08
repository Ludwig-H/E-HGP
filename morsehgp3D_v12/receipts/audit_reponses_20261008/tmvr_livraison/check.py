#!/usr/bin/env python3
"""Compare les octets livrés à repo6 ; ne construit et n'exécute aucun moteur."""
import argparse
import ast
import hashlib
import json
import os
import subprocess
import integration
from pathlib import Path

HERE = Path(__file__).resolve().parent


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def prototype(source, copied):
    result, h = {}, hashlib.sha256()
    for name in copied:
        origin = source / name
        paths = [origin] if origin.is_file() else []
        for folder, folders, files in os.walk(origin):
            folders[:] = sorted(e for e in folders if e != '__pycache__')
            paths += [Path(folder) / e for e in sorted(files) if not e.endswith('.pyc')]
        for path in sorted(paths):
            rel, digest = path.relative_to(source).as_posix(), sha(path.read_bytes())
            result[rel] = digest
            h.update(rel.encode() + b'\0' + digest.encode() + b'\n')
    return result, [h.hexdigest(), len(result)]


def inspect(repo, source, capture):
    parsed = ast.parse((source / 'tests/mutants/run_mutants.py').read_text())
    copied = next(ast.literal_eval(n.value) for n in parsed.body if isinstance(n, ast.Assign)
                  and any(isinstance(t, ast.Name) and t.id == 'COPIED' for t in n.targets))
    require(list(copied) == capture['copied'], 'périmètre changé')
    old, tree = prototype(source, copied)
    require(tree == capture['prototype_tree'], 'repo6 ne correspond plus à la capture')
    pin, prefix = capture['delivered_commit'], 'morsehgp3D_v12/'
    paths = git(repo, 'ls-tree', '-r', '--name-only', pin, prefix).decode().splitlines()
    new = {}
    for full in paths:
        rel = full[len(prefix):]
        if not any(rel == n or rel.startswith(n + '/') for n in copied):
            continue
        if '/__pycache__/' in rel or rel.endswith('.pyc'):
            continue
        new[rel] = sha(git(repo, 'show', pin + ':' + full))
    require(old.keys() == new.keys(), 'liste de fichiers différente')
    delta = [{'path': p, 'prototype_sha256': old[p], 'delivered_sha256': new[p]}
             for p in sorted(old) if old[p] != new[p]]
    require(delta == capture['differences'], 'delta différent')
    groups, delivered_hash = {}, hashlib.sha256()
    for name in copied:
        selected = sorted(p for p in new if p == name or p.startswith(name + '/'))
        groups[name] = {'files': len(selected), 'equal': sum(old[p] == new[p] for p in selected)}
        for p in selected:
            delivered_hash.update(p.encode() + b'\0' + new[p].encode() + b'\n')
    changed = git(repo, 'diff-tree', '--no-commit-id', '--name-only', '-r', pin).decode().splitlines()
    product = [p for p in changed if p.startswith(prefix) and not p.startswith(prefix + 'docs/')
               and not p.startswith(prefix + 'receipts/')]
    require(all(old[p[len(prefix):]] == new[p[len(prefix):]] for p in product), 'produit livré différent')
    require(prototype(source, copied) == (old, tree), 'sources modifiées pendant la lecture')
    result = {'prototype_tree': tree, 'delivered_tree': [delivered_hash.hexdigest(), len(new)],
              'identical_files': len(new) - len(delta), 'differences': delta, 'groups': groups,
              'commit_changed_paths': len(changed), 'changed_code_config_test_paths_equal': len(product),
              'prototype_stable': True, 'native_executed': False}
    require(result == capture['source_result'], 'résultat divergent')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, required=True)
    p.add_argument('--prototype', type=Path, required=True, help='dossier repo6/morsehgp3D_v12')
    p.add_argument('--integration-root', type=Path, help='racine des journaux scratchpad, facultative')
    a = p.parse_args()
    try:
        capture = json.loads((HERE / 'capture.json').read_text())
        result = {'sources': inspect(a.repo, a.prototype, capture)}
        if a.integration_root is not None:
            result['integration'] = integration.inspect(a.integration_root, capture['integration'])
            passed, _ = integration.rows((a.integration_root / 'build_v12_u21.ctest_tmv.log').read_text(), 717)
            require(set(capture['targeted_gates']) <= passed, 'porte TMVR attendue non passée')
            result['targeted_gates_passed'] = capture['targeted_gates']
    except (ValueError, KeyError, OSError, StopIteration, subprocess.CalledProcessError) as exc:
        p.exit(2, f'REFUS: {exc}\n')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
