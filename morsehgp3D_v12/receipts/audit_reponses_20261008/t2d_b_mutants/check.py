#!/usr/bin/env python3
"""Lecture des deux campagnes closes ; aucun moteur ou compilation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def tree(root):
    h, count = hashlib.sha256(), 0
    for name in ('CMakeLists.txt', 'cmake', 'src', 'cli', 'bench', 'tests', 'tools', 'reference', 'docs'):
        p = root / name
        paths = [p] if p.is_file() else []
        for folder, folders, files in os.walk(p):
            folders[:] = sorted(x for x in folders if x != '__pycache__')
            paths += [Path(folder) / x for x in sorted(files) if not x.endswith('.pyc')]
        for p in sorted(paths):
            h.update(p.relative_to(root).as_posix().encode() + b'\0' + sha(p.read_bytes()).encode() + b'\n')
            count += 1
    return dict(files=count, sha256=h.hexdigest())


def review(e, source, cap):
    need(tree(source) == cap['source_tree'], 'arbre source différent')
    selections = re.findall(r'--only ([a-z0-9_,]+)', (e / 'outils/mutants_locaux.sh').read_text())
    need(len(selections) == 2, 'sélections du conducteur')
    result = {}
    for module, report_name, expected in (('num', 'mutants_num_garde.json', 8),
                                           ('tower', 'mutants_tower_t2d_b.json', 9)):
        report = json.loads((e / 'runs' / report_name).read_text())
        p = source / 'tests/mutants' / (module + '.json')
        manifest = json.loads(p.read_text())
        by_id = {x['id']: x for x in manifest['mutants']}
        need(report['schema'] == 'mhgp12.mutants.v1' and report['module'] == module, 'format')
        need(type(report['code']) is int and report['code'] == 0 and report['temoin'] == 'vert', 'issue')
        need(report['sources_sha256'] == cap['source_tree']['sha256'], 'arbre rapporté')
        need(report['manifeste_sha256'] == sha(p.read_bytes()), 'manifeste rapporté')
        rows = report['mutants']
        need(report['plancher'] == len(rows) == len({x['id'] for x in rows}) == expected, 'cohorte')
        need([x['id'] for x in rows] == cap['selected'][module], 'sélection différente')
        requested = selections[0 if module == 'num' else 1].split(',')
        need(len(requested) == expected and set(requested) == {x['id'] for x in rows}, 'sélection non jouée')
        for row in rows:
            need(row['verdict'] == 'TUE' and row['detail'] == 'code' and
                 row['juge'] == by_id[row['id']]['porte'], 'verdict/porte')
        result[module] = {'code_killed': expected, 'witness': 'green'}
    codes = (e / 'logs/mutants_locaux.log').read_text()
    need(codes.endswith('tower code=0 05:44:39\nfin 05:44:39\n'), 'clôture tour')
    need('num code=0 05:31:12\n' in codes, 'clôture num')
    live = json.loads((e / 'live_mutated_sources.json').read_text())
    manifest = json.loads((source / 'tests/mutants/tower.json').read_text())
    by_id = {x['id']: x for x in manifest['mutants']}
    need(len(live['mutants']) == 3, 'captures mutées')
    for obs in live['mutants']:
        mutation = by_id[obs['id']]
        raw = (source / mutation['fichier']).read_bytes()
        text = raw.decode()
        need(sha(raw) == obs['original_sha256'] and text.count(mutation['cherche']) == 1, 'source mutation')
        mutated = text.replace(mutation['cherche'], mutation['remplace']).encode()
        need(sha(mutated) == obs['mutated_sha256'] and obs['equals_manifest_substitution'] is True, 'mutation')
    result['last_live_mutated_sources_exact'] = 3
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--evidence', type=Path, required=True)
    p.add_argument('--source', type=Path, required=True)
    a = p.parse_args()
    cap = json.loads(Path(__file__).with_name('capture.json').read_text())
    for rel, pin in cap['files'].items():
        raw = (a.evidence / rel).read_bytes()
        need(len(raw) == pin['bytes'] and sha(raw) == pin['sha256'], 'pin différent : ' + rel)
    result = review(a.evidence, a.source, cap)
    need(result == cap['result'], 'résultat différent')
    print(json.dumps(result, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
