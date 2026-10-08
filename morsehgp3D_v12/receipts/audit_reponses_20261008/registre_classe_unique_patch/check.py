#!/usr/bin/env python3
"""Pins et application du patch en copie temporaire ; aucune compilation ni execution du moteur."""
import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def verify(repo):
    pins = json.loads((HERE / 'pins.json').read_text())
    patch = (HERE / 'proposition.patch').read_bytes()
    need(sha(patch) == pins['patch_sha256'], 'patch different')
    target = pins['patch_path']
    sources = {}
    for pin in pins['sources']:
        body = git(repo, 'show', pins['base_commit'] + ':' + pin['path'])
        compared = git(repo, 'show', pins['comparison_commit'] + ':' + pin['path'])
        need(sha(body) == pin['sha256'] and len(body) == pin['bytes'], 'source ' + pin['path'])
        need(sha(compared) == pin['comparison_sha256'], 'source comparee ' + pin['path'])
        need((body == compared) == pin['equal_at_comparison_commit'], 'egalite source ' + pin['path'])
        sources[pin['path']] = body
    proof = pins['proof']
    body = git(repo, 'show', proof['commit'] + ':' + proof['path'])
    need(sha(body) == proof['sha256'], 'preuve publiee differente')
    applications = []
    for commit in (pins['base_commit'], pins['comparison_commit']):
        with tempfile.TemporaryDirectory(prefix='audit-r-apply-') as temp:
            root = Path(temp)
            path = root / target
            path.parent.mkdir(parents=True)
            before = git(repo, 'show', commit + ':' + target)
            need(sha(before) == pins['before_sha256'], 'base du patch')
            path.write_bytes(before)
            subprocess.run(['git', 'apply', '--check', str(HERE / 'proposition.patch')], cwd=root,
                           check=True, capture_output=True)
            subprocess.run(['git', 'apply', str(HERE / 'proposition.patch')], cwd=root,
                           check=True, capture_output=True)
            after = path.read_bytes()
            need(sha(after) == pins['after_sha256'], 'resultat du patch')
            files = [p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()]
            need(files == [target], 'fichier supplementaire')
            applications.append({'commit': commit, 'apply_check': True, 'after_sha256': sha(after)})
    return {'sources_pinned': len(sources),
            'sources_equal': sum(p['equal_at_comparison_commit'] for p in pins['sources']),
            'sources_different': [p['path'] for p in pins['sources'] if not p['equal_at_comparison_commit']],
            'proof_pin_verified': True, 'applications': applications,
            'native_compiled': False, 'native_executed': False, 'performance_measured': False,
            'scope': 'Application textuelle et pins seulement ; qualification CSR/memoire/concurrence a jouer sur G4.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path('.'))
    args = parser.parse_args()
    try:
        result = verify(args.repo)
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as exc:
        parser.exit(2, 'REFUS: ' + str(exc) + '\n')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
