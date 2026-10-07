#!/usr/bin/env python3
"""Contrôle mécanique de docs/PORTS.md : empreintes SHA-256 des sources de la v11 gelée, copies à l'identique et
renommages seuls. Lecture Git seule, bibliothèque standard. Usage : verifier_ports.py <dépôt> <commit v12> [<pin v11>]."""
import hashlib
import subprocess
import sys

RENAMES = (('mhgp11', 'mhgp12'), ('MHGP11', 'MHGP12'), ('hgp11_', 'hgp12_'), ('ehgp.v11.', 'ehgp.v12.'))


def show(repo, rev, path):
    result = subprocess.run(['git', '-C', repo, 'show', '%s:%s' % (rev, path)], capture_output=True)
    return result.stdout if result.returncode == 0 else None


def rename(data):
    text = data.decode('utf-8')
    for old, new in RENAMES:
        text = text.replace(old, new)
    return text.encode('utf-8')


def main(argv):
    if len(argv) not in (3, 4):
        print(__doc__, file=sys.stderr)
        return 2
    repo, v12, v11 = argv[1], argv[2], argv[3] if len(argv) == 4 else 'ac081a06f'
    table = show(repo, v12, 'morsehgp3D_v12/docs/PORTS.md')
    if table is None:
        print('PORTS.md introuvable', file=sys.stderr)
        return 2
    counts = dict(rows=0, sha_ok=0, copies=0, renames=0, adapted=0)
    issues = []
    for line in table.decode('utf-8').splitlines():
        if not line.startswith('| `'):
            continue
        cells = [cell.strip() for cell in line.strip('|').split('|')]
        if len(cells) < 4 or not cells[2].startswith('`'):
            continue
        counts['rows'] += 1
        new, old, digest, state = cells[0].strip('`'), cells[1].strip('`'), cells[2].strip('`'), cells[3]
        source = show(repo, v11, 'morsehgp3D_v11/' + old)
        target = show(repo, v12, 'morsehgp3D_v12/' + new)
        if source is None or target is None:
            issues.append('absent : %s' % new)
            continue
        if hashlib.sha256(source).hexdigest() == digest:
            counts['sha_ok'] += 1
        else:
            issues.append('SHA-256 : %s' % new)
        if state.startswith('copie'):
            counts['copies'] += 1
            if source != target:
                issues.append('copie différente : %s' % new)
        elif state.startswith('renommage seul'):
            counts['renames'] += 1
            if rename(source) != target:
                issues.append('renommage seul démenti : %s' % new)
        else:
            counts['adapted'] += 1
    print(' '.join('%s=%d' % item for item in counts.items()))
    for issue in issues:
        print(issue)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
