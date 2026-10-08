#!/usr/bin/env python3
"""Raccord textuel G-c/TMVR ; extrait seulement les fichiers concernes, aucun build."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'
MODULE = PREFIX + 'src/tower/module.cmake'
MANIFEST = PREFIX + 'tests/mutants/tower.json'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def apply(root, data, *options, check=False):
    args = ['git', 'apply', *options]
    if check:
        args.append('--check')
    return subprocess.run(args, cwd=root, input=data, capture_output=True)


def subsequence(before, after):
    it = iter(after.splitlines())
    return all(any(x == line for x in it) for line in before.splitlines())


def compose(repo, patches, base, supplied=None):
    paths = []
    for data in patches:
        lines = subprocess.check_output(['git', 'apply', '--numstat'], input=data).decode().splitlines()
        names = {line.split('\t', 2)[2] for line in lines}
        require(all(n.startswith(PREFIX) and '..' not in Path(n).parts for n in names), 'chemin interdit')
        paths.append(names)
    gc_paths, tmvr_paths = paths[0] | paths[1], paths[2]
    all_paths = gc_paths | tmvr_paths
    tracked = set(subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', base],
                                         cwd=repo, text=True).splitlines())

    def original(rel):
        return subprocess.check_output(['git', 'show', base + ':' + rel], cwd=repo)

    def extract(root, names):
        root.mkdir()
        for rel in names & tracked:
            dest = root / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(original(rel))

    with tempfile.TemporaryDirectory(prefix='audit-composition-gc-tmvr-') as directory:
        folder = Path(directory)
        root, standalone = folder / 'combined', folder / 'tmvr'
        extract(root, all_paths)
        extract(standalone, tmvr_paths)
        for data in patches[:2]:
            require(apply(root, data, check=True).returncode == 0, 'application G-c')
            require(apply(root, data).returncode == 0, 'application G-c')
        gc = {rel: (root / rel).read_bytes() for rel in gc_paths}
        require(apply(standalone, patches[2], check=True).returncode == 0, 'TMVR seul')
        require(apply(standalone, patches[2]).returncode == 0, 'TMVR seul')
        tmvr = {rel: (standalone / rel).read_bytes() for rel in tmvr_paths}
        failed = apply(root, patches[2], check=True)
        conflicts = set(re.findall(r'error: patch failed: (.*):\d+', failed.stderr.decode()))
        require(failed.returncode != 0 and conflicts == {MODULE, MANIFEST}, 'conflits differents')
        options = ['--exclude=' + MODULE, '--exclude=' + MANIFEST]
        require(apply(root, patches[2], *options, check=True).returncode == 0, 'TMVR partiel')
        require(apply(root, patches[2], *options).returncode == 0, 'TMVR partiel')
        marker = '# Etages T, M, V, R et export FUL1.\n'
        require(tmvr[MODULE].decode().count(marker) == 1, 'bloc TMVR ambigu')
        module = gc[MODULE] + tmvr[MODULE][tmvr[MODULE].index(marker.encode()):]
        gj, tj = json.loads(gc[MANIFEST]), json.loads(tmvr[MANIFEST])
        gby, tby = ({m['id']: m for m in j['mutants']} for j in (gj, tj))
        shared = set(gby) & set(tby)
        require(len(gby) == len(gj['mutants']) and len(tby) == len(tj['mutants']), 'identifiants repetes')
        require(all(gby[k] == tby[k] for k in shared), 'mutants communs divergents')
        merged = dict(gj)
        merged['mutants'] = gj['mutants'] + [m for m in tj['mutants'] if m['id'] not in gby]
        merged['plancher'] = len(merged['mutants'])
        manifest = (json.dumps(merged, indent=2, ensure_ascii=False) + '\n').encode()
        fixes = {MODULE: module, MANIFEST: manifest}
        patch = ''.join(''.join(difflib.unified_diff(gc[rel].decode().splitlines(True),
                      fixes[rel].decode().splitlines(True), fromfile='a/' + rel, tofile='b/' + rel))
                        for rel in (MODULE, MANIFEST)).encode()
        if supplied is not None:
            require(supplied == patch, 'raccord different de l union exacte')
        require(apply(root, patch, check=True).returncode == 0, 'raccord non applicable')
        require(apply(root, patch).returncode == 0, 'raccord non applicable')
        for rel, data in fixes.items():
            require((root / rel).read_bytes() == data, 'raccord incomplet')
        for rel in gc_paths - tmvr_paths:
            require((root / rel).read_bytes() == gc[rel], 'source G-c alteree : ' + rel)
        for rel in tmvr_paths - gc_paths:
            require((root / rel).read_bytes() == tmvr[rel], 'source TMVR alteree : ' + rel)
        for rel in (gc_paths & tmvr_paths) - {MODULE, MANIFEST}:
            final = (root / rel).read_text()
            require(subsequence(gc[rel].decode(), final), 'lignes G-c perdues : ' + rel)
            old = original(rel).decode().splitlines(True)
            added = tmvr[rel].decode().splitlines(True)
            for tag, _, _, lo, hi in difflib.SequenceMatcher(a=old, b=added, autojunk=False).get_opcodes():
                if tag != 'equal':
                    require(tag == 'insert' and ''.join(added[lo:hi]) in final,
                            'ajout TMVR perdu ou changement non couvert : ' + rel)
        extra = {}

        def read(rel):
            p = root / rel
            if p.exists():
                return p.read_bytes()
            data = original(rel)
            extra[rel] = digest(data)
            return data

        sources = re.findall(r'mhgp12_module_sources\((.*?)\)', module.decode(), re.S)
        names = [word for group in sources for word in group.split()]
        require(len(names) == len(set(names)), 'source module repetee')
        for name in names:
            read(PREFIX + 'src/tower/' + name)
        cmake = (root / (PREFIX + 'tests/tower/tests.cmake')).read_text()
        gates = set()
        for unit, args in re.findall(r'mhgp12_add_unit\((\S+)\s+(.*?)\)', cmake, re.S):
            group = re.search(r'GROUPS\s+(.*?)\s+LABELS', args, re.S)
            if group:
                gates.update(unit + '_' + g for g in group.group(1).split())
        occurrences = 0
        for mutant in merged['mutants']:
            require(mutant['porte'] in gates, 'porte non declaree : ' + mutant['porte'])
            current = {}
            for action in [mutant, *mutant.get('aussi', [])]:
                rel = PREFIX + action['fichier']
                text = current.get(rel)
                if text is None:
                    text = read(rel).decode()
                require(text.count(action['cherche']) == 1, 'emplacement mutant non unique : ' + mutant['id'])
                current[rel] = text.replace(action['cherche'], action['remplace'], 1)
                occurrences += 1
        h = hashlib.sha256()
        for rel in sorted(all_paths):
            h.update((rel + '\0' + digest((root / rel).read_bytes()) + '\n').encode())
        return dict(base=base, patched_files=len(all_paths), combined_files_sha256=h.hexdigest(),
                    module_sha256=digest(module), manifest_sha256=digest(manifest),
                    module_sources=len(names), common_mutants=len(shared), gc_only_mutants=len(set(gby) - shared),
                    tmvr_only_mutants=len(set(tby) - shared), mutants=len(merged['mutants']),
                    unique_mutation_locations=occurrences, gates_declared=len({m['porte'] for m in merged['mutants']}),
                    unmodified_dependencies_sha256=extra, source_lines_preserved=True, native_qualified=False,
                    native_runs=0, builds=0, gcp_used=False), patch


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--gc', type=Path, required=True)
    parser.add_argument('--tmvr', type=Path, required=True)
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    paths = [args.gc / capture['patches'][i]['name'] for i in (0, 1)] + [args.tmvr / capture['patches'][2]['name']]
    patches = [p.read_bytes() for p in paths]
    require([digest(p) for p in patches] == [p['sha256'] for p in capture['patches']], 'patch modifie')
    result, _ = compose(args.repo, patches, capture['base'], (HERE / 'raccord.patch').read_bytes())
    require(result == capture['result'], 'resultat different')
    require([p.read_bytes() for p in paths] == patches, 'patch modifie pendant lecture')
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
