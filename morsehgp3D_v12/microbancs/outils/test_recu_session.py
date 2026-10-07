#!/usr/bin/env python3
"""Porte de recu_session.py (CST-0219, CST-0221, CST-0224) sur des sessions synthetiques, adresse fictive seulement.

Cas : publication conforme ; nom de fichier porteur de l'adresse (expurge) ; deux fichiers d'un meme dossier ou de deux
dossiers dont les noms se confondent apres expurgation ; un fichier qui deviendrait le dossier ancetre d'un autre
(residu de CST-0224) ; destination occupee. Chaque refus doit rendre son code (2 ou 3) SANS laisser de fichier : une
destination absente au depart reste absente, une destination vide reste vide, une destination occupee reste intacte.

Usage : python3 test_recu_session.py ; codes 0 conforme, 1 ecart. Bibliotheque standard seule, aucun assert (tient
sous python3 -O).
"""
import io
import json
import os
import sys
import tempfile
from contextlib import redirect_stderr, redirect_stdout

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import recu_session  # noqa: E402

ACCOUNT = 'audit@example.invalid'
CASES = {
    'conforme': ({'commands.tsv': b'ok\n', 'cmd/001/files/r.json': b'{"x": 1}\n'}, 0),
    'nom_expurge': ({ACCOUNT + '.json': b'1'}, 0),
    'collision_plate': ({'run-' + ACCOUNT + '.json': b'1', 'run-<compte>.json': b'2'}, 3),
    'collision_imbriquee': ({ACCOUNT + '/one.json': b'1', '<compte>/one.json': b'2'}, 3),
    'collision_fichier_dossier': ({ACCOUNT: b'1', '<compte>/child.json': b'2'}, 3),
}


def tree(path):
    out = []
    for root, _dirs, files in os.walk(path):
        for name in files:
            out.append(os.path.relpath(os.path.join(root, name), path))
    return sorted(out)


def make_session(base, files):
    session = os.path.join(base, 'session')
    results = os.path.join(session, 'results', 'extracted', 'results')
    os.makedirs(results)
    with open(os.path.join(session, 'receipt.json'), 'w', encoding='utf-8') as out:
        json.dump({'recovery_command': ACCOUNT, 'state': 'ok'}, out)
    with open(os.path.join(session, 'preflight.json'), 'w', encoding='utf-8') as out:
        json.dump({'gcloud_account': ACCOUNT}, out)
    for name, data in files.items():
        path = os.path.join(results, name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as out:
            out.write(data)
    return session


def call(session, dest):
    sink = io.StringIO()
    with redirect_stdout(sink), redirect_stderr(sink):
        try:
            return recu_session.main(['recu_session.py', '--session', session, '--dest', dest, '--include', '*',
                                      '--include', '*/*', '--include', '*/*/*', '--include', '*/*/*/*'])
        except Exception as error:  # une exception est un ecart : jamais de recu partiel
            return 'exception %s' % error.__class__.__name__


def main():
    gaps = []
    runs = 0
    for name, (files, expected) in sorted(CASES.items()):
        for existing in (False, True):
            with tempfile.TemporaryDirectory(prefix='recu_test_') as base:
                session = make_session(base, files)
                dest = os.path.join(base, 'recu')
                if existing:
                    os.mkdir(dest)
                code = call(session, dest)
                runs += 1
                present = os.path.exists(dest)
                content = tree(dest) if present else None
                if code != expected:
                    gaps.append('%s (destination %s) : code %r au lieu de %d'
                                % (name, 'vide' if existing else 'absente', code, expected))
                    continue
                if expected != 0:
                    want = [] if existing else None
                    if content != want:
                        gaps.append('%s : reste %r apres refus (attendu %r)' % (name, content, want))
                    continue
                leaked = [p for p in content if ACCOUNT in p]
                for p in content:
                    with open(os.path.join(dest, p), 'rb') as handle:
                        if ACCOUNT.encode() in handle.read():
                            leaked.append(p)
                if leaked or 'SHA256SUMS' not in content or 'receipt.json' not in content:
                    gaps.append('%s : recu incomplet ou fuite %r' % (name, leaked or content))
    with tempfile.TemporaryDirectory(prefix='recu_test_') as base:
        session = make_session(base, CASES['conforme'][0])
        dest = os.path.join(base, 'occupee')
        os.mkdir(dest)
        with open(os.path.join(dest, 'sentinelle'), 'wb') as out:
            out.write(b'garde')
        code = call(session, dest)
        runs += 1
        if code != 2 or tree(dest) != ['sentinelle']:
            gaps.append('destination occupee : code %r, contenu %r' % (code, tree(dest)))
    for gap in gaps:
        print(gap, file=sys.stderr)
    if gaps:
        return 1
    print('recu_session_test_ok appels=%d cas=%d' % (runs, len(CASES) + 1))
    return 0


if __name__ == '__main__':
    sys.exit(main())
