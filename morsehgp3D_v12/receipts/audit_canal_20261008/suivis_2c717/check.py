#!/usr/bin/env python3
"""Conservation du registre lors du déplacement, sans rejouer les preuves."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import posixpath
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, label):
    if not ok:
        raise ValueError(label)


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def links(text, directory):
    return sorted(posixpath.normpath(posixpath.join(directory, p))
                  for p in re.findall(r'\]\(([^)]+)\)', text))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', default='/workspaces/E-HGP')
    args = parser.parse_args()
    m = json.loads((HERE / 'mapping.json').read_text())
    changes = json.loads((HERE / 'proposition.json').read_text())
    get = lambda p: subprocess.check_output(['git', '-C', args.repo, 'show', m['commit'] + ':' + p]).decode()
    old = get(m['source'])
    need(sha(old) == m['source_sha256'], 'source Git')
    need(set(changes) == {'CST-0018', 'CST-0207', 'CST-0233', 'CST-0234'}, 'portée des lignes')
    original = old.splitlines(keepends=True)
    proposed = original.copy()
    archive = (HERE / 'README.md').read_text()
    archive_dir = 'morsehgp3D_v12/receipts/audit_canal_20261008/suivis_2c717'
    original_dir = 'morsehgp3D_v12/audits'
    link_count = 0
    seen = set()
    for row in m['rows']:
        cid = row['id']
        need(cid not in seen, 'ligne dupliquée')
        seen.add(cid)
        index = row['source_line'] - 1
        line = original[index]
        need(sha(line) == row['row_sha256'], 'hash ligne ' + cid)
        cols = line.rstrip('\n').split('|')
        need(len(cols) == 12 and cols[1].strip() == '`' + cid + '`', 'table ' + cid)
        need(cols[9].strip() == 'en cours', 'état source ' + cid)
        proof = cols[10].strip()
        need(sha(proof) == row['followup_sha256'], 'hash suivi ' + cid)
        start = '\n## ' + cid + '\n'
        need(archive.count(start) == 1, 'ancre ' + cid)
        section = archive.split(start, 1)[1].split('\n## ', 1)[0]
        archived = proof.replace('(../receipts/', '(../../')
        need('\n\n' + archived + '\n' in section, 'suivi historique exact ' + cid)
        for field in (3, 4, 7, 8, 9):
            needle = cols[field].strip().replace('(../receipts/', '(../../')
            need(needle in section, 'métadonnée conservée ' + cid)
        old_links = links(proof, original_dir)
        need(old_links == links(archived, archive_dir), 'cibles des liens ' + cid)
        need(len(old_links) == row['preserved_links'], 'nombre de liens ' + cid)
        link_count += len(old_links)
        need('README.md#' + cid.lower() == row['anchor'], 'mapping ancre ' + cid)
        cols[10] = ' ' + changes[cid] + ' '
        proposed[index] = '|'.join(cols) + '\n'
        need(cols[:10] == line.rstrip('\n').split('|')[:10], 'neuf colonnes modifiées')
        need(len(line.encode()) == row['old_row_bytes'] and
             len(proposed[index].encode()) == row['new_row_bytes'], 'tailles des lignes')
    need(seen == set(changes), 'cohorte des constats')
    new = ''.join(proposed)
    need(sha(new) == m['proposed_sha256'], 'proposition exacte')
    expected = ''.join(difflib.unified_diff(original, proposed,
                       fromfile='a/' + m['source'], tofile='b/' + m['source'], n=0))
    need((HERE / 'proposition.patch').read_text() == expected, 'patch hors portée')
    saved = len(old.encode()) - len(new.encode())
    need(saved == m['saved_bytes'] and saved >= 2500, 'économie insuffisante')
    paths = subprocess.check_output(['git', '-C', args.repo, 'ls-tree', '-r', '--name-only',
                                     m['commit'], original_dir], text=True).splitlines()
    size = sum(len(get(p).encode()) for p in paths)
    need(size == m['source_channel_bytes'] and size - saved == m['proposed_channel_bytes'], 'taille canal')
    with tempfile.TemporaryDirectory(prefix='audit-canal-') as raw:
        target = Path(raw) / m['source']
        target.parent.mkdir(parents=True)
        target.write_text(old)
        for flag in ['--check', None]:
            cmd = ['git', 'apply', '--unidiff-zero'] + ([flag] if flag else []) + [str(HERE / 'proposition.patch')]
            subprocess.run(cmd, cwd=raw, check=True, capture_output=True)
        need(target.read_text() == new, 'application isolée du patch')
    print(json.dumps({'rows': sorted(seen), 'first_nine_columns_unchanged': True,
                      'historical_followup_links_preserved': link_count,
                      'source_channel_bytes': size, 'proposed_channel_bytes': size - saved,
                      'saved_bytes': saved, 'isolated_patch_applies': True,
                      'state_changes': 0, 'product_or_main_changes': False}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
