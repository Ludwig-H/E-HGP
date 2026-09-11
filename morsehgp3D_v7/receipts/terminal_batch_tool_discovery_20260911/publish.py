#!/usr/bin/env python3
"""Record local Python checks and publish one compact create-only packet."""
import difflib
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
DEST = ROOT / 'morsehgp3D_v7/receipts/terminal_batch_tool_discovery_20260911'
PARENT = ROOT / 'morsehgp3D_v7/receipts/terminal_batch_worker_20260911'
PARENT_PIN = '8758776c41abbbe08009a55c6e97bc10f7d040ccaadb4d5bcc5a6d93d5bf16b2'
OLD_PIN = '043197e4c73d92ff8845ddb5836c11fbb9ed9bdbd9a609edf7ce4a946d2c0fb3'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def main():
    if sha(PARENT / 'manifest.json') != PARENT_PIN:
        raise ValueError('historical packet changed')
    mapping = json.loads((PARENT / 'storage_map.json').read_text())
    old = PARENT / mapping['snapshot/worker.py']
    if sha(old) != OLD_PIN:
        raise ValueError('historical 043197 source changed')
    selected = {
        'sources/gcp-migration/terminal_batch_worker_v7.py': ROOT / 'gcp-migration/terminal_batch_worker_v7.py',
        'sources/gcp-migration/selftest_terminal_batch_worker_v7.py': ROOT / 'gcp-migration/selftest_terminal_batch_worker_v7.py',
        'sources/gcp-migration/selftest_terminal_batch_tool_preflight_v7.py': ROOT / 'gcp-migration/selftest_terminal_batch_tool_preflight_v7.py',
        'sources/README_TERMINAL_BATCH_V7.md.source': ROOT / 'gcp-migration/README_TERMINAL_BATCH_V7.md',
        'origins/worker_043197.py.source': old,
        'sources/gcp-migration/full_probe_worker_v7.py': PARENT / mapping['snapshot/snapshot/morsehgp3D_v7/bench/session_support/full_probe_worker_v7.py'],
        'sources/morsehgp3D_v7/bench/nvcc_strict_host.py': PARENT / mapping['snapshot/snapshot/morsehgp3D_v7/bench/nvcc_strict_host.py'],
        'README.md': BASE / 'PACKAGE_README.md', 'verify.py': BASE / 'verify.py', 'publish.py': Path(__file__)}
    before = {str(path): sha(path) for path in selected.values()}
    DEST.mkdir()
    for name, source in selected.items():
        path = DEST / name
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, path)
    save(DEST / 'capture/sources_before.json', before)
    save(DEST / 'origins/provenance.json', dict(parent_manifest_sha256=PARENT_PIN, logical_worker='snapshot/worker.py',
         worker_sha256=OLD_PIN, old_snapshots_unchanged=True, new_GPU_snapshot_created=False))
    new = selected['sources/gcp-migration/terminal_batch_worker_v7.py']
    difference = ''.join(difflib.unified_diff(old.read_text().splitlines(keepends=True), new.read_text().splitlines(keepends=True),
                                             fromfile='worker_043197.py', tofile='worker_aaf8bcc1.py'))
    with (DEST / 'worker.diff').open('x') as stream:
        stream.write(difference)
    rows = []
    for prefix, test in (('pure', 'selftest_terminal_batch_worker_v7.py'), ('mock', 'selftest_terminal_batch_tool_preflight_v7.py')):
        for mode, flags in (('normal', []), ('optimized', ['-O'])):
            name = prefix + '_' + mode
            argv = ['python3', '-B', *flags, str(ROOT / 'gcp-migration' / test)]
            save(DEST / ('capture/' + name + '.intent.json'), dict(argv=argv, cwd=str(ROOT), expected_exit=0))
            with (DEST / ('capture/' + name + '.stdout')).open('xb') as stdout, \
                    (DEST / ('capture/' + name + '.stderr')).open('xb') as stderr:
                command = subprocess.run(argv, cwd=ROOT, stdout=stdout, stderr=stderr, timeout=30, check=False)
            row = dict(name=name, argv=argv, cwd=str(ROOT), exit_code=command.returncode,
                       stdout_sha256=sha(DEST / ('capture/' + name + '.stdout')),
                       stderr_sha256=sha(DEST / ('capture/' + name + '.stderr')))
            rows.append(row)
            save(DEST / ('capture/' + name + '.command.json'), row)
            if command.returncode != 0 or (DEST / ('capture/' + name + '.stderr')).read_bytes():
                raise ValueError('Python qualification failed: ' + name)
    after = {str(path): sha(path) for path in selected.values()}
    save(DEST / 'capture/sources_after.json', after)
    if before != after or sha(PARENT / 'manifest.json') != PARENT_PIN:
        raise ValueError('source changed while recording')
    save(DEST / 'capture/receipt.json', dict(status='passed', commands=rows, source_stable=True,
         source_mapping={str(path): name for name, path in selected.items()}, GCP_used=False, compiled=False,
         new_GPU_snapshot_created=False))
    files = {path.relative_to(DEST).as_posix(): dict(sha256=sha(path), bytes=path.stat().st_size)
             for path in sorted(DEST.rglob('*')) if path.is_file()}
    save(DEST / 'manifest.json', dict(schema='mhgp7-terminal-tool-discovery-v1', files=files))
    print(json.dumps(dict(status='closed', output=str(DEST), manifest_sha256=sha(DEST / 'manifest.json')), sort_keys=True))


if __name__ == '__main__':
    main()
