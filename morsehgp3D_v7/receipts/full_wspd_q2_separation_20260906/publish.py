#!/usr/bin/env python3
"""Preserve closed q2/separation observations without running an engine."""
import hashlib
import json
from pathlib import Path
import types

ROOT = Path('/workspaces/E-HGP')
PREP = ROOT / 'build/v7_wspd_q2_separation_20260906'
SOURCE = ROOT / 'build/v7_wspd_probe_20260906'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_wspd_q2_separation_20260906'
OLD = ROOT / 'morsehgp3D_v7/receipts/full_direct_scaling_20260906'
READER_SHA = '59e84530ca5468e324f79fae2f1e91ba6a7d776095b3f7784fe18f262642ad3b'
RUNNER_SHA = 'd381fc81c378382e610c61722863f6ae5764e4c3ef34ee7f94ac18f53a6c5fe7'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def main():
    if DEST.exists():
        raise ValueError('destination exists; no overwrite')
    raw = (PREP / 'read_results.py').read_bytes()
    if sha(raw) != READER_SHA:
        raise ValueError('reader changed')
    reader = types.ModuleType('closed_measurements_only')
    reader.__file__ = str(PREP / 'read_results.py')
    exec(compile(raw, reader.__file__, 'exec'), reader.__dict__)
    report = reader.results(SOURCE, OLD / 'n8000_s8_punlimited')
    build_raw = reader.checked(SOURCE / 'build.json', reader.BUILD_SHA)
    build = json.loads(build_raw)
    reader.checked(SOURCE / 'full_probe', reader.BINARY_SHA)
    payload = {'build.json': build_raw, 'read_results.py': raw,
        'results.json': encoded(report), 'README.md': (PREP / 'NOTE_RESULTATS.md').read_bytes(),
        'publish.py': Path(__file__).read_bytes()}
    for name in ('full_probe.d', 'snapshot_build.py'):
        payload[name] = (SOURCE / name).read_bytes()
    if sha(payload['full_probe.d']) != 'abaf20a31291de33e4da3f08e1b43ed1dd5ad8036db83b93b40ba7cc872c6063':
        raise ValueError('depfile changed')
    files = [path for path in (SOURCE / 'source_snapshot').rglob('*') if path.is_file()]
    if {str(path.relative_to(SOURCE / 'source_snapshot')) for path in files} != set(build['project_sources']):
        raise ValueError('snapshot inventory')
    for path in files:
        name = str(path.relative_to(SOURCE / 'source_snapshot'))
        payload['source_snapshot/' + name] = reader.checked(path, build['project_sources'][name])
    for name in reader.RUN_PINS:
        folder = SOURCE / name
        if {p.name for p in folder.iterdir()} != {'run.json', 'stdout.jsonl', 'stderr.txt'}:
            raise ValueError('run inventory')
        for path in folder.iterdir():
            if path.is_symlink():
                raise ValueError('symlink run')
            payload[name + '/' + path.name] = path.read_bytes()
    payload['bench/run_full_probe.py'] = reader.checked(OLD / 'bench/run_full_probe.py', RUNNER_SHA)
    payload['excluded_binary.json'] = encoded(dict(original=str(SOURCE / 'full_probe'),
        sha256=reader.BINARY_SHA, original_preserved=True, copied=False))
    payload['OLD_REFERENCE.json'] = encoded(dict(package='../full_direct_scaling_20260906',
        SHA256SUMS='120d60b9f363a665af874dccb1f10dffba7c6278272b8eb3f88897b5ae0feee8',
        old_s8_run_sha256=reader.OLD_RUN_SHA, old_binary_sha256=reader.OLD_BINARY_SHA,
        scope='historical_triplet_8k_16k_32k; q2 paired comparison uses only its s8/8k observation'))
    DEST.mkdir()
    for name, content in sorted(payload.items()):
        target = DEST / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as out:
            out.write(content)
        if target.read_bytes() != content:
            raise ValueError('copy mismatch:' + name)
    if reader.results(DEST, OLD / 'n8000_s8_punlimited') != report:
        raise ValueError('copied results mismatch')
    manifest = ''.join(sha(content) + '  ' + name + '\n' for name, content in sorted(payload.items())).encode()
    with (DEST / 'SHA256SUMS').open('xb') as out:
        out.write(manifest)
    print(json.dumps(dict(status='copied', files=len(payload) + 1, SHA256SUMS=sha(manifest),
        results_sha256=sha(payload['results.json']), engine_invoked=False), sort_keys=True))


if __name__ == '__main__':
    main()
