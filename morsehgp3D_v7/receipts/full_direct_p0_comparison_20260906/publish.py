#!/usr/bin/env python3
"""Preserve the closed P0/P∞ pair and its descriptive comparison; no engine."""
import hashlib
import json
from pathlib import Path
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_direct_p0_comparison_20260906'
SOURCE = ROOT / 'build/v7_direct_scaling_20260906'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_direct_p0_comparison_20260906'
READER_SHA = '59db71300fd721c7b75238d4d02dd921b053cf82f84240d92ee7343eaca1ab3e'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def main():
    if DEST.exists():
        raise ValueError('destination exists; no overwrite')
    raw = (BASE / 'compare.py').read_bytes()
    if sha(raw) != READER_SHA:
        raise ValueError('reader changed')
    reader = types.ModuleType('comparison_only')
    reader.__file__ = str(BASE / 'compare.py')
    exec(compile(raw, reader.__file__, 'exec'), reader.__dict__)
    report = reader.compare(SOURCE)
    payload = {'compare.py': raw, 'comparison.json': encoded(report), 'publish.py': Path(__file__).read_bytes()}
    for token in reader.PINS:
        name = 'n8000_s8_p' + token
        directory = SOURCE / name
        if {p.name for p in directory.iterdir()} != {'run.json', 'stdout.jsonl', 'stderr.txt'}:
            raise ValueError('unexpected run inventory')
        for path in directory.iterdir():
            if path.is_symlink():
                raise ValueError('symlink capture')
            payload[name + '/' + path.name] = path.read_bytes()
    payload['README.md'] = (BASE / 'NOTE_RESULTATS.md').read_bytes() + b'\nRecalcul : `python3 compare.py`. Integrite : `sha256sum -c SHA256SUMS`.\nLes six bruts sont inchanges ; le paquet triplet precedent reste intact.\n'
    payload['SOURCE_REFERENCE.json'] = encoded(dict(
        build_receipt='../full_probe_no_quotas_20260906/build_r1/receipt.json',
        build_receipt_sha256='25ccae8eb8466280568090963314c9a67a579bc5d81aa43871830a13a6fd7e9d',
        source_snapshot='../full_probe_no_quotas_20260906/source_snapshot',
        source_map_sha256='72a147fec063382013b4c61a1990ad163cd99dbe9051a8c170c417797b519fbe',
        binary_sha256=reader.BINARY_SHA, runner='../full_direct_scaling_20260906/bench/run_full_probe.py',
        runner_sha256='d381fc81c378382e610c61722863f6ae5764e4c3ef34ee7f94ac18f53a6c5fe7',
        product_meb_helper_sha256='f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3'))
    DEST.mkdir()
    for name, content in sorted(payload.items()):
        path = DEST / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as out:
            out.write(content)
        if path.read_bytes() != content:
            raise ValueError('copy mismatch')
    if reader.compare(DEST) != report:
        raise ValueError('copied comparison mismatch')
    manifest = ''.join(sha(content) + '  ' + name + '\n' for name, content in sorted(payload.items())).encode()
    with (DEST / 'SHA256SUMS').open('xb') as out:
        out.write(manifest)
    print(json.dumps(dict(status='copied', files=len(payload) + 1, SHA256SUMS=sha(manifest),
        comparison_sha256=sha(payload['comparison.json']), engine_invoked=False), sort_keys=True))


if __name__ == '__main__':
    main()
