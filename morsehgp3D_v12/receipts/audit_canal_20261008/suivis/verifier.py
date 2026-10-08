#!/usr/bin/env python3
"""Rejoue le compactage épinglé ; n'écrit que le patch explicitement demandé."""
import argparse
import difflib
import hashlib
import json
import posixpath
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
LINK = re.compile(r'\[([^\]]+)\]\(([^)]+)\)')
RECEIPT = 'morsehgp3D_v12/receipts/audit_canal_20261008/suivis'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def rows(text):
    result = {}
    for line in text.splitlines(keepends=True):
        if not line.startswith('| `CST-'):
            continue
        cells = re.split(r'(?<!\\)\|', line.rstrip('\n'))[1:-1]
        require(len(cells) == 10, 'dix colonnes requises')
        key = cells[0].strip(' `')
        require(key not in result, 'identifiant dupliqué')
        # La dernière séparation avant la cellule de suivi ne peut être échappée.
        end = line.rfind('|')
        split = line.rfind('|', 0, end) + 1
        result[key] = (line[:split], cells[-1], line[end:])
    return result


def paths(text, base):
    values = []
    for _, target in LINK.findall(text):
        target = target.partition('#')[0]
        if not target or re.match(r'[A-Za-z][A-Za-z0-9+.-]*:', target):
            continue
        values.append(posixpath.normpath(posixpath.join(base, target)))
    return values


def verify(repo, patch):
    proposal = json.loads((HERE / 'proposition.json').read_text())
    pin, path = proposal['source_commit'], proposal['source_path']
    raw = git(repo, 'show', f'{pin}:{path}')
    require(sha(raw) == proposal['source_sha256'], 'source modifiée')
    require(len(raw) == proposal['source_bytes'], 'taille source')
    source = raw.decode('utf-8')
    before = rows(source)
    require(len(before) == 75, 'cohorte de 75 constats requise')
    tree = set(git(repo, 'ls-tree', '-r', '--name-only', pin).decode().splitlines())
    channel = [name for name in tree if name.startswith('morsehgp3D_v12/audits/')]
    total = sum(len(git(repo, 'show', f'{pin}:{name}')) for name in channel)
    require(total == proposal['source_channel_bytes'], 'taille canal source')
    readme = (HERE / 'README.md').read_text()
    changed = set()
    target = source
    counts = []
    for row in proposal['rows']:
        key = row['id']
        require(key not in changed and key in before, 'sélection dupliquée ou absente')
        changed.add(key)
        prefix, old, suffix = before[key]
        require(sha(prefix.encode()) == row['columns_1_9_sha256'], key + ': colonnes 1–9')
        require(old == row['before'], key + ': ancien suivi divergent')
        require('|' not in row['after'] and '\n' not in row['after'], key + ': structure du nouveau suivi')
        target = target.replace(prefix + old + suffix, prefix + row['after'] + suffix, 1)
        old_links = paths(old, posixpath.dirname(path))
        new_links = paths(row['after'], posixpath.dirname(path))
        section = re.search(r'^## ' + key + r'\n(.*?)(?=^## |\Z)', readme, re.S | re.M)
        require(section is not None, key + ': section archive absente')
        archived_links = paths(section[1], RECEIPT)
        require(sorted(old_links) == sorted(archived_links), key + ': preuve déplacée perdue/ajoutée')
        require(all(link in tree for link in old_links), key + ': ancienne preuve absente du commit')
        require(all(link in tree or link == RECEIPT + '/README.md' for link in new_links), key + ': nouveau lien absent')
        require(f'](../receipts/audit_canal_20261008/suivis/README.md#{key.lower()})' in row['after'], key + ': raccord archive absent')
        kept = sum(link in new_links for link in old_links)
        counts.append({'id': key, 'before_bytes': len(old.encode()), 'after_bytes': len(row['after'].encode()),
                       'links_kept_direct': kept, 'links_moved_to_archive': len(old_links) - kept})
    after = rows(target)
    require(list(before) == list(after), 'identifiants ou ordre changés')
    require(len(changed) == 8, 'huit suivis attendus')
    for key in before:
        require(before[key][0] == after[key][0] and before[key][2] == after[key][2], key + ': colonnes modifiées')
        if key not in changed:
            require(before[key] == after[key], key + ': ligne hors portée modifiée')
    require(sha(target.encode()) == proposal['expected_after_sha256'], 'hash résultat')
    require(len(target.encode()) == proposal['expected_after_bytes'], 'taille résultat')
    require(target.count('\n') == source.count('\n'), 'nombre de lignes')
    if patch:
        patch.write_text(''.join(difflib.unified_diff(source.splitlines(keepends=True),
                         target.splitlines(keepends=True), fromfile='a/' + path, tofile='b/' + path, n=0)))
    saved = len(raw) - len(target.encode())
    return {'source_commit': pin, 'constats': len(before), 'changed_cells': len(changed),
            'first_nine_columns_equal': True, 'unselected_rows_equal': True,
            'all_old_proof_links_preserved': True, 'before_bytes': len(raw), 'after_bytes': len(target.encode()),
            'saved_bytes': saved, 'channel_before_bytes': total, 'channel_after_bytes': total - saved, 'rows': counts}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    parser.add_argument('--check', action='store_true', help='vérification seule (également le défaut)')
    parser.add_argument('--patch', type=Path, help='écrit uniquement ce patch, sans l’appliquer')
    args = parser.parse_args()
    try:
        result = verify(args.repo, args.patch)
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as exc:
        parser.exit(2, f'REFUS: {exc}\n')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
