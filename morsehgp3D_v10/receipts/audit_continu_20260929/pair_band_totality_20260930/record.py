"""Freeze a private receipt, keeping every qualified execution and source pins."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from datetime import datetime, timezone

BASE = Path(__file__).resolve().parent
SHARED = [Path('/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues/mmc.py'),
          Path('/workspaces/E-HGP/build/v10-verrou-points/revision_cible/statistique_cible/MEMO.md'),
          Path('/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues/MEMO.md'),
          Path('/workspaces/E-HGP/build/v10-verrou-points/revision_cible/statistique_cible/paires_k2.py')]
SOURCES = ['README.md', 'functions_snapshot.py', 'check.py', 'record.py', 'read.py']


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pins():
    return {str(p): sha(p) for p in SHARED}


def main():
    if {p.name for p in BASE.iterdir()} != set(SOURCES):
        raise RuntimeError('unexpected initial inventory; retain old capture in old package')
    if (BASE / 'capture.json').exists() or (BASE / 'manifest.json').exists():
        raise RuntimeError('receipt already frozen')
    before = pins()
    source_before = {name: sha(BASE / name) for name in SOURCES}
    lines = SHARED[0].read_text().splitlines(keepends=True)
    excerpt = '\n\n'.join(''.join(lines[a:b]).strip() for a, b in [(227, 233), (332, 359)]) + '\n'
    if excerpt.encode() != (BASE / 'functions_snapshot.py').read_bytes():
        raise RuntimeError('shared function excerpts do not match snapshot')
    capture = {'schema': 1, 'scope': 'hard-alpha totality counterexample and actual pair scales; not native/GCP/MMt',
               'started_utc': datetime.now(timezone.utc).isoformat(), 'shared_before': before,
               'source_before': source_before, 'runs': []}
    try:
        for mode, flags in [('normal', ['-B']), ('optimized', ['-B', '-O'])]:
            cmd = [sys.executable] + flags + [str(BASE / 'check.py')]
            run = subprocess.run(cmd, text=True, capture_output=True, timeout=20)
            capture['runs'].append({'mode': mode, 'argv': cmd, 'returncode': run.returncode,
                                    'stdout': run.stdout, 'stderr': run.stderr})
    finally:
        capture['shared_after'] = pins()
        capture['source_after'] = {name: sha(BASE / name) for name in SOURCES}
        capture['finished_utc'] = datetime.now(timezone.utc).isoformat()
        (BASE / 'capture.json').write_text(json.dumps(capture, sort_keys=True, indent=2) + '\n')
    valid = (capture['shared_before'] == capture['shared_after'] and
             capture['source_before'] == capture['source_after'] and len(capture['runs']) == 2 and
             all(r['returncode'] == 0 and r['stderr'] == '' for r in capture['runs']) and
             capture['runs'][0]['stdout'] == capture['runs'][1]['stdout'])
    manifest = {'schema': 1, 'capture_valid': valid,
                'files': {name: sha(BASE / name) for name in SOURCES + ['capture.json']}}
    (BASE / 'manifest.json').write_text(json.dumps(manifest, sort_keys=True, indent=2) + '\n')
    if not valid:
        raise RuntimeError('qualified receipt failed; failure retained')
    print(json.dumps({'status': 'PASS', 'manifest_sha256': sha(BASE / 'manifest.json'),
                      'runs': len(capture['runs'])}, sort_keys=True))


if __name__ == '__main__':
    main()
