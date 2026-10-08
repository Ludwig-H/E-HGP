#!/usr/bin/env python3
"""Relecture du protocole/source local ; aucune commande native ou distante."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tarfile


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--session', type=Path, required=True)
    a = ap.parse_args()
    cap_path = Path(__file__).with_name('capture.json')
    cap = json.loads(cap_path.read_text())
    plan_path = a.session / 'package/plan.json'
    raw = plan_path.read_bytes()
    need(sha(raw) == cap['plan_sha256'], 'plan modifié')
    plan = json.loads(raw)
    for command, expected in zip(plan['commands'], cap['commands'], strict=True):
        need(command['name'] == expected['name'] and
             command['timeout_seconds'] == expected['timeout_seconds'], 'commandes')
        argv = command['argv']
        keys = {x[0] for x in expected['selected_options']}
        actual = [[v, True if v in ('--essai', '--sequentiel', '--recouvert') else argv[i+1]]
                  for i, v in enumerate(argv) if v in keys]
        need(actual == expected['selected_options'], 'options')
    def opt(n, key):
        argv = plan['commands'][n]['argv']
        return argv[argv.index(key)+1]
    b1 = PurePosixPath(opt(1, '--travail')) / 'b21cuda'
    b2 = PurePosixPath(opt(2, '--travail'))
    need(b1 != b2 and b1 not in b2.parents and b2 not in b1.parents, 'builds non isolés')
    need(opt(1, '--donnees') == opt(2, '--donnees') == opt(4, '--donnees'), 'entrées différentes')
    need(opt(1, '--archive-v12set') == opt(2, '--archive-v12set'), 'archives différentes')
    helper = a.repo / cap['helper']['path']
    need(sha(helper.read_bytes()) == cap['helper']['sha256'], 'lecteur source modifié')
    python = [sys.executable] + (['-O'] if sys.flags.optimize else [])
    subprocess.run(python + [str(helper), '--repo', str(a.repo), '--package',
                   str(a.session/'package/package.tar.gz'), '--plan', str(plan_path),
                   '--capture', str(cap_path)], check=True, stdout=subprocess.DEVNULL)
    seen = set()
    with tarfile.open(a.session/'package/package.tar.gz') as archive:
        for member in archive:
            if member.name not in cap['source_files']:
                continue
            need(member.isfile() and member.name not in seen, 'membre source')
            body = archive.extractfile(member).read()
            expected = cap['source_files'][member.name]
            need(len(body) == expected['bytes'] and sha(body) == expected['sha256'], 'source changée')
            original = subprocess.check_output(['git', '-C', str(a.repo), 'show',
                                                cap['source_git']+':'+member.name])
            need(body == original, 'source hors commit')
            seen.add(member.name)
    need(seen == set(cap['source_files']), 'source absente')
    print(json.dumps(dict(source_git=cap['source_git'], sources_exactes=cap['files_exact'],
                         fichiers_cibles=len(seen), commandes=5, builds_isoles=True,
                         execution_native=False, qualification_resultats=False), sort_keys=True))


if __name__ == '__main__':
    main()
