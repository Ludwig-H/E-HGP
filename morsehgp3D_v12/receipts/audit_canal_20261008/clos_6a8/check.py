#!/usr/bin/env python3
"""Conservation de dix-huit suivis clos ; aucune preuve metier rejouee."""
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
ARCHIVE = 'morsehgp3D_v12/receipts/audit_canal_20261008/clos_6a8'
AUDITS = 'morsehgp3D_v12/audits'


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def links(text, directory):
    return sorted(posixpath.normpath(posixpath.join(directory, x))
                  for x in re.findall(r'\]\(([^)]+)\)', text))


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--repo', required=True)
    repo = p.parse_args().repo
    m = json.loads((HERE / 'mapping.json').read_text())
    changes = json.loads((HERE / 'proposition.json').read_text())

    def get(path):
        return subprocess.check_output(['git', '-C', repo, 'show', m['commit'] + ':' + path], text=True)

    old = get(m['source'])
    need(sha(old) == m['source_sha256'], 'source')
    original, archive = old.splitlines(keepends=True), (HERE / 'README.md').read_text()
    proposed = original.copy()
    closed = []
    for line in original:
        c = line.rstrip('\n').split('|')
        if len(c) == 12 and c[9].strip() == 'clos':
            closed.append((len(c[10].encode()), c[1].strip(' `')))
    selected = {cid for _, cid in sorted(closed, reverse=True)[:18]}
    need(set(changes) == selected and len(selected) == 18, 'dix-huit plus longs clos')
    need(not selected & {'CST-0018', 'CST-0241'}, 'constat actif interdit')
    seen, count = set(), 0
    for row in m['rows']:
        cid, index = row['id'], row['source_line'] - 1
        need(cid not in seen, 'doublon')
        seen.add(cid)
        line = original[index]
        need(sha(line) == row['row_sha256'], 'ligne ' + cid)
        c = line.rstrip('\n').split('|')
        need(len(c) == 12 and c[1].strip() == '`' + cid + '`' and c[9].strip() == 'clos', 'etat ' + cid)
        need(sha('|'.join(c[:10])) == row['first_nine_sha256'], 'neuf colonnes source')
        follow = c[10].strip()
        need(sha(follow) == row['followup_sha256'], 'suivi ' + cid)
        tag = '\n## ' + cid + '\n'
        need(archive.count(tag) == 1, 'ancre ' + cid)
        section = archive.split(tag, 1)[1].split('\n## ', 1)[0]
        historical = follow.replace('(../receipts/', '(../../')
        need('\n\n' + historical + '\n' in section, 'historique exact ' + cid)
        targets = links(follow, AUDITS)
        need(targets == links(historical, ARCHIVE), 'liens deplaces ' + cid)
        need(len(targets) == row['preserved_links'], 'compte liens ' + cid)
        count += len(targets)
        need(row['anchor'] == 'README.md#' + cid.lower(), 'mapping ' + cid)
        primary = re.findall(r'\]\(([^)]+)\)', changes[cid])[0]
        need(primary == row['closure_proof'], 'preuve inline ' + cid)
        label = changes[cid].split('](', 1)[0]
        pins = re.findall(r'[0-9a-f]{9}', label)
        need(pins and all(pin in follow for pin in pins), 'pin de cloture ' + cid)
        for target in targets + links(changes[cid], AUDITS):
            path = target.split('#', 1)[0]
            if path == ARCHIVE + '/README.md':
                need(target.endswith('#' + cid.lower()), 'nouvelle ancre')
            else:
                get(path)  # tous les fichiers cibles existaient au pin
        c[10] = ' ' + changes[cid] + ' '
        proposed[index] = '|'.join(c) + '\n'
        need(c[:10] == line.rstrip('\n').split('|')[:10], 'neuf colonnes modifiees')
        need(len(line.encode()) == row['old_row_bytes'] and len(proposed[index].encode()) == row['new_row_bytes'],
             'taille ligne')
    need(seen == selected, 'cohorte')
    new = ''.join(proposed)
    need(sha(new) == m['proposed_sha256'], 'postimage')
    expected = ''.join(difflib.unified_diff(original, proposed, fromfile='a/' + m['source'],
                                           tofile='b/' + m['source'], n=0))
    need((HERE / 'proposition.patch').read_text() == expected, 'patch hors portee')
    saved = len(old.encode()) - len(new.encode())
    need(saved == m['saved_bytes'] and saved >= 3000, 'gain insuffisant')
    paths = subprocess.check_output(['git', '-C', repo, 'ls-tree', '-r', '--name-only', m['commit'], AUDITS],
                                    text=True).splitlines()
    size = sum(len(get(x).encode()) for x in paths)
    need(size == m['source_channel_bytes'] and size - saved == m['proposed_channel_bytes'], 'taille canal')
    with tempfile.TemporaryDirectory(prefix='audit-canal-clos-') as folder:
        target = Path(folder) / m['source']
        target.parent.mkdir(parents=True)
        target.write_text(old)
        for flags in (['--check'], []):
            subprocess.run(['git', 'apply', '--unidiff-zero', *flags, str(HERE / 'proposition.patch')],
                           cwd=folder, check=True, capture_output=True)
        need(target.read_text() == new, 'application isolee')
    print(json.dumps(dict(rows=sorted(seen), first_nine_columns_unchanged=True, states_changed=0,
                         links_preserved=count, source_channel_bytes=size, proposed_channel_bytes=size-saved,
                         saved_bytes=saved, isolated_patch_applies=True, main_or_product_modified=False),
                     ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
