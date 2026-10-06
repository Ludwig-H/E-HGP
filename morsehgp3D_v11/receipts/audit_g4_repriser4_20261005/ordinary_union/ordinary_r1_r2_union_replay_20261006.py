#!/usr/bin/env python3
"""Reconciliation locale metadata seule des inventaires ordinaires R1+R2/fina2."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tarfile

SESSIONS = {
    'old': ('v11.20261005.claudefina2', '42425a0969d728688672785f34f3302b698cf2daea83ca6c62891d6ff9b28631'),
    'r1': ('v11.20261005.clauderepriser1', '1ab6efd4708f1d185398175c0a4fe4b016a031767fd1c0154c4b7883b229cbd7'),
    'r2': ('v11.20261005.clauderepriser2', '0d122a7fa1e9a1536197a5003456c105abfb5bd977d53bc4be2376379ab69347'),
}
SOURCE_PIN = '98a00955083d483306c4f92b9031e382e81b0e59'
RESULT = re.compile(r'^\s*\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+\s*(Passed|\*\*\*[^\n]+?)\s+\d+(?:\.\d+)? sec\s*$', re.M)

def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)

def digest(data):
    return hashlib.sha256(data).hexdigest()

def inventory(archive, name, passed_required):
    prefix = 'results/cmd/000_matrice/files/matrix/' + name + '/'
    member = archive.getmember(prefix + 'tests.json')
    need(member.isfile() and member.size <= 16 * 1024 * 1024, 'inventaire non borne')
    rows = json.load(archive.extractfile(member))
    names = {row['name'] for row in rows}
    need(len(names) == len(rows), 'inventaire duplique')
    if passed_required:
        member = archive.getmember(prefix + 'ctest.log')
        need(member.isfile() and member.size <= 16 * 1024 * 1024, 'journal non borne')
        found = RESULT.findall(archive.extractfile(member).read().decode())
        out = dict(found)
        need(len(found) == len(out) and set(out) == names and all(state == 'Passed' for state in out.values()),
             'verdict nouveau absent, inconnu, duplique ou non PASS')
    return names

def replay(root):
    opened = {}
    try:
        for key, (name, expected) in SESSIONS.items():
            session = root / name
            receipt = json.loads((session / 'receipt.json').read_text())
            need((session / 'DONE').is_file() and receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'],
                 'session non close')
            path = session / 'results/results.tar.gz'
            need(digest(path.read_bytes()) == expected == receipt['results_sha256'], 'archive divergente')
            if key != 'old':
                need(receipt['commit'] == SOURCE_PIN and receipt['status'] == 'completed' and
                     receipt['worker_exit_code'] == 0 and (session / 'DONE').read_text().strip() == '0',
                     'pin/resultat R1/R2 different')
            opened[key] = tarfile.open(path, 'r:gz')
        answer = []
        for name in ('gcc_release','bits21','bits24','poison'):
            old = inventory(opened['old'], name, False)
            a = inventory(opened['r1'], name, True)
            gros = inventory(opened['r2'], name + '_echelle_gros', True)
            reste = inventory(opened['r2'], name + '_echelle_reste', True)
            need(not gros & reste, 'lots R2 superposes')
            b = gros | reste
            need(not a & b and a | b == old, 'union ordinaire incomplete ou superposee')
            answer.append(dict(configuration=name, old_count=len(old), r1_count=len(a), r2_count=len(b),
                intersection=sorted(a & b), extra=sorted((a | b)-old), missing=sorted(old-(a | b)),
                union_sha256=digest(('\n'.join(sorted(a | b)) + '\n').encode())))
        return answer
    finally:
        for archive in opened.values():
            archive.close()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions', type=Path, default=Path('/workspaces/.ehgp-sessions'))
    parser.add_argument('--summary', type=Path, default=Path(__file__).with_name('ordinary_r1_r2_union_20261006.json'))
    args = parser.parse_args()
    answer = replay(args.sessions)
    need(answer == json.loads(args.summary.read_text()), 'resume fige divergent')
    print(json.dumps(dict(source_commit=SOURCE_PIN, ordinary_passed=sum(row['old_count'] for row in answer),
        disjoint=True, exact_fina2_union=True, no_native_or_cloud_execution=True, replay='conforme'), sort_keys=True))

if __name__ == '__main__':
    main()
