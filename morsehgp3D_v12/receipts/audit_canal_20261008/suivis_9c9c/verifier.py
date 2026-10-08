#!/usr/bin/env python3
"""Vérifie la conservation des suivis ; produit un patch optionnel sans l'appliquer."""
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
EXCLUDED = {'CST-' + x for x in ('0018', '0105', '0107', '0022', '0207',
                               '0021', '0024', '0104', '0113', '0211', '0212', '0218')}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def rows(text):
    found = {}
    for line in text.splitlines(keepends=True):
        if not line.startswith('| `CST-'):
            continue
        cells = re.split(r'(?<!\\)\|', line.rstrip('\n'))[1:-1]
        require(len(cells) == 10, 'dix colonnes attendues')
        key = cells[0].strip(' `')
        require(key not in found, 'ID dupliqué')
        end = line.rfind('|')
        split = line.rfind('|', 0, end) + 1
        found[key] = (line[:split], cells[-1], line[end:])
    return found


def destination(target, base):
    path, marker, fragment = target.partition('#')
    require(not re.match(r'[A-Za-z][A-Za-z0-9+.-]*:', path), 'lien externe inattendu')
    return posixpath.normpath(posixpath.join(base, path)), marker + fragment


def rebase(text, old_base, new_base):
    def replace(match):
        path, fragment = destination(match[2], old_base)
        return '[' + match[1] + '](' + posixpath.relpath(path, new_base) + fragment + ')'
    return LINK.sub(replace, text)


def verify(repo, patch):
    proposal = json.loads((HERE / 'proposition.json').read_text())
    pin, path, receipt = (proposal[k] for k in ('source_commit', 'source_path', 'receipt'))
    raw = git(repo, 'show', pin + ':' + path)
    require(sha(raw) == proposal['source_sha256'], 'hash source')
    require(len(raw) == proposal['source_bytes'], 'taille source')
    source = raw.decode()
    before = rows(source)
    require(len(before) == 75, '75 IDs requis')
    tree = set(git(repo, 'ls-tree', '-r', '--name-only', pin).decode().splitlines())
    channel = [p for p in tree if p.startswith('morsehgp3D_v12/audits/')]
    channel_bytes = sum(len(git(repo, 'show', pin + ':' + p)) for p in channel)
    require(channel_bytes == proposal['source_channel_bytes'], 'taille canal source')
    archive = dict(re.findall(r'^\| <a id="cst-\d+"></a>`(CST-\d+)` \| (.*) \|$',
                              (HERE / 'README.md').read_text(), re.M))
    require(len(archive) == len(proposal['rows']) == 11, 'onze archives requises')
    target = source
    changed = set()
    details = []
    for item in proposal['rows']:
        key = item['id']
        require(key not in changed and key in before and key not in EXCLUDED, 'sélection hors portée')
        changed.add(key)
        prefix, old, suffix = before[key]
        new = item['after']
        require(sha(prefix.encode()) == item['columns_1_9_sha256'], key + ': colonnes 1–9')
        require(sha(old.encode()) == item['before_sha256'] and len(old.encode()) == item['before_bytes'],
                key + ': ancien suivi')
        require(new.startswith(' ') and new.endswith(' ') and '|' not in new and '\n' not in new,
                key + ': structure du nouveau suivi')
        require(archive[key] == rebase(old.strip(), posixpath.dirname(path), receipt),
                key + ': texte ou liens anciens non conservés')
        old_links = [destination(m[1], posixpath.dirname(path)) for m in LINK.findall(old)]
        archived = [destination(m[1], receipt) for m in LINK.findall(archive[key])]
        require(sorted(old_links) == sorted(archived), key + ': destinations de preuve')
        require(all(p in tree for p, _ in old_links), key + ': preuve absente au pin')
        new_links = [destination(m[1], posixpath.dirname(path)) for m in LINK.findall(new)]
        require(new_links == [(receipt + '/README.md', '#' + key.lower())], key + ': lien archive')
        require(f'<a id="{key.lower()}"></a>`{key}`' in (HERE / 'README.md').read_text(), key + ': ancre')
        target = target.replace(prefix + old + suffix, prefix + new + suffix, 1)
        details.append({'id': key, 'before_bytes': len(old.encode()), 'after_bytes': len(new.encode()),
                        'proof_links_archived': len(old_links)})
    require(set(archive) == changed, 'table archive hors portée')
    after = rows(target)
    require(list(before) == list(after), 'IDs ou ordre changés')
    for key in before:
        require(before[key][0] == after[key][0] and before[key][2] == after[key][2], key + ': colonnes changées')
        if key not in changed:
            require(before[key] == after[key], key + ': suivi hors portée changé')
    require(sha(target.encode()) == proposal['expected_after_sha256'], 'hash résultat')
    require(len(target.encode()) == proposal['expected_after_bytes'], 'taille résultat')
    require(source.count('\n') == target.count('\n'), 'nombre de lignes changé')
    if patch is not None:
        patch.write_text(''.join(difflib.unified_diff(source.splitlines(keepends=True),
                         target.splitlines(keepends=True), fromfile='a/' + path, tofile='b/' + path, n=0)))
    saved = len(raw) - len(target.encode())
    return {'source_commit': pin, 'constats': 75, 'changed_cells': len(changed),
            'first_nine_columns_and_states_equal': True, 'unselected_rows_equal': True,
            'old_texts_and_all_proof_links_preserved': True,
            'before_bytes': len(raw), 'after_bytes': len(target.encode()), 'saved_bytes': saved,
            'channel_before_bytes': channel_bytes, 'channel_after_bytes': channel_bytes - saved,
            'proof_links_archived': sum(r['proof_links_archived'] for r in details), 'rows': details}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path('.'))
    parser.add_argument('--check', action='store_true', help='vérification seule (défaut)')
    parser.add_argument('--patch', type=Path, help='destination du patch optionnel')
    args = parser.parse_args()
    try:
        result = verify(args.repo, args.patch)
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as exc:
        parser.exit(2, 'REFUS: ' + str(exc) + '\n')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
