#!/usr/bin/env python3
"""Inert create-only export of closed private MEB captures, never qualification.

Use --run RUN/run.json=SHA and --source-pins RUN/source_pins.json=SHA once
per run, plus explicit --judgment PATH=SHA attachments. ROOT may supply --readme.
Recorded paths remain historical. No compiler, engine, judge or Git is invoked.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re

SCHEMA = 'mhgp7-private-filtered-meb-publication-v1'
DESTINATION = 'morsehgp3D_v7/receipts/meb_filtered_20260906'


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for raw in iter(lambda: stream.read(1048576), b''):
            value.update(raw)
    return value.hexdigest()


def encoded(value: object) -> bytes:
    return (json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n').encode()


def strict(raw: bytes) -> object:
    def pairs(items):
        result = {}
        for name, value in items:
            require(name not in result, 'duplicate JSON key')
            result[name] = value
        return result
    return json.loads(raw, object_pairs_hook=pairs,
                      parse_constant=lambda _: require(False, 'nonfinite JSON'))


def execute(args: argparse.Namespace) -> None:
    own, root = Path(__file__).resolve(), args.root.absolute()
    require(sha(own) == args.expected_publisher_sha256, 'reviewed publisher pin required')
    destination = root / DESTINATION
    require(root.is_dir() and not destination.exists() and not destination.is_symlink(), 'destination must be absent')
    watches, exports, excluded, runs, judgments, checks = {}, {}, {}, [], [], []

    def watch(path: Path, pin: str | None = None) -> None:
        require(path.is_absolute() and path.is_relative_to(root) and '..' not in path.parts
                and not path.is_relative_to(root / 'morsehgp3D_v7/audits'), 'input scope; no live auditor source')
        require(all(not p.is_symlink() for p in (path, *path.parents))
                and path.is_file() and path.stat().st_size <= 64 << 20, 'regular bounded input')
        stamp = {'sha256': sha(path), 'bytes': path.stat().st_size}
        require(pin is None or re.fullmatch('[0-9a-f]{64}', pin) is not None and stamp['sha256'] == pin, 'input pin')
        require(path not in watches or watches[path] == stamp, 'input changed')
        watches[path] = stamp
        require(sum(item['bytes'] for item in watches.values()) <= 1 << 30, 'total input bound')

    def argument(text: str) -> Path:
        name, separator, pin = text.rpartition('=')
        require(separator, 'explicit PATH=SHA256 required')
        path = Path(name)
        watch(path, pin)
        return path

    def metadata(path: Path) -> dict:
        require(path.stat().st_size <= 8 << 20, 'metadata bound')
        value = strict(path.read_bytes())
        require(type(value) is dict, 'metadata object required')
        return value

    def export(path: Path, name: str) -> None:
        require(name not in exports and name not in ('publication.json', 'SHA256SUMS'), 'export collision')
        watch(path)
        with path.open('rb') as stream:
            require(stream.read(4) != b'\x7fELF', 'ELF outside excluded bin/: ' + str(path))
        exports[name] = path

    pin_paths = [argument(text) for text in args.source_pins]
    require(len(set(pin_paths)) == len(pin_paths), 'duplicate source-pins argument')
    by_run = {path.parent: path for path in pin_paths}
    require(all(path.name == 'source_pins.json' for path in pin_paths), 'source-pins filename')
    seen = set()
    for text in args.run:
        receipt = argument(text)
        run_dir = receipt.parent
        require(receipt.name == 'run.json' and run_dir.parent == root / 'build'
                and re.fullmatch(r'v7_meb_filter_qualification_20260906_r[1-9][0-9]*', run_dir.name)
                and run_dir not in seen and run_dir in by_run, 'fresh run/source-pins scope')
        seen.add(run_dir)
        report, pins = metadata(receipt), metadata(by_run[run_dir])
        require(report['schema'] == 'mhgp7-private-filtered-meb-run-v1'
                and report['status'] in ('completed', 'failed') and report['public_status'] == 'not_claimed'
                and report['gcp'] == 'not_used', 'closed nonpromoted run required')
        require(hashlib.sha256(encoded(pins)).hexdigest() == report['source_map_sha256'], 'source map binding')
        for name, pin in pins.items():
            relative = Path(name)
            require(not relative.is_absolute() and '..' not in relative.parts, 'snapshot escape')
            watch(run_dir / 'snapshot' / relative, pin)
        run_files = {}
        for path in sorted(run_dir.rglob('*')):
            require(not path.is_symlink(), 'run symlink')
            if path.is_dir():
                continue
            require(path.is_file(), 'run special file')
            relative = path.relative_to(run_dir)
            watch(path)
            run_files[str(relative)] = watches[path]
            if relative.parts[0] == 'bin' or '__pycache__' in relative.parts or path.suffix in ('.pyc', '.pyo'):
                excluded[str(path.relative_to(root))] = watches[path]
            else:
                export(path, 'runs/' + run_dir.name + '/' + str(relative))
        runs.append(dict(argument=text, source_pins_argument=str(by_run[run_dir]) + '=' + watches[by_run[run_dir]]['sha256'],
                         original_status=report['status'], source_map_sha256=report['source_map_sha256'],
                         source_inventory=run_files, qualification_reinterpreted=False))
    require(seen == set(by_run) and bool(runs), 'one source-pins map for each run')
    for index, text in enumerate(args.judgment):
        path = argument(text)
        require(path.is_relative_to(root / 'build'), 'judgment must be a private captured file')
        value = metadata(path)
        name = f'judgments/{index:02d}_' + path.name
        export(path, name)
        judgments.append(dict(argument=text, exported_as=name, original_status=value.get('status'),
                              original_schema=value.get('schema'), association='ROOT_explicit_attachment_not_rejudged'))
    for text in args.checks:
        receipt = argument(text)
        value = metadata(receipt)
        require(receipt.name == 'receipt.json' and receipt.parent.is_relative_to(root / 'build')
                and value['schema'] == 'mhgp7-filtered-meb-pure-replays-v1'
                and value['status'] == 'completed' and value['engine_runs'] == 0
                and value['compiler_runs'] == 0 and value['normal_optimized_bytes_equal'] is True,
                'completed pure replay bundle required')
        require(any(value['run_path'] == str(Path(run['argument'].rpartition('=')[0]).parent)
                    and value['run_sha256'] == run['argument'].rpartition('=')[2]
                    and value['source_sha256'] == run['source_map_sha256'] for run in runs),
                'pure replay not bound to an attached run')
        names = value['artifacts']
        require({p.name for p in receipt.parent.iterdir() if p.is_file()} == set(names) | {'receipt.json'},
                'unexpected replay bundle inventory')
        prefix = 'checks/' + receipt.parent.name + '/'
        for name, pin in names.items():
            require(Path(name).name == name and name not in ('.', '..'), 'replay artifact path')
            watch(receipt.parent / name, pin)
            export(receipt.parent / name, prefix + name)
        export(receipt, prefix + 'receipt.json')
        checks.append(dict(argument=text, exported_prefix=prefix, original_status=value['status'],
                           association='source_and_run_pins_verified_not_rejudged'))
    require(bool(judgments) or bool(checks), 'explicit captured judgment attachment required')
    if args.readme is not None:
        export(argument(args.readme), 'README.md')
    export(own, 'protocol/publish.py')
    # Admission finishes before the first destination write. Recheck all bytes,
    # including excluded binaries; no executable is copied or launched.
    require(all(sha(path) == stamp['sha256'] for path, stamp in watches.items()), 'pre-copy input drift')
    for parent in destination.parents:
        require(not parent.is_symlink(), 'destination parent symlink')
    destination.mkdir(parents=False, exist_ok=False)
    for name, source in sorted(exports.items()):
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with source.open('rb') as incoming, target.open('xb') as outgoing:
            for raw in iter(lambda: incoming.read(1048576), b''):
                outgoing.write(raw)
        require(sha(target) == watches[source]['sha256'], 'copy differs: ' + name)
    require(all(sha(path) == stamp['sha256'] for path, stamp in watches.items()), 'post-copy input drift')
    for run in runs:
        run_dir = Path(run['argument'].rpartition('=')[0]).parent
        require({str(p.relative_to(run_dir)) for p in run_dir.rglob('*') if p.is_file()}
                == set(run['source_inventory']), 'run inventory changed during copy')
    report = dict(schema=SCHEMA, status='copied_not_requalified', public_status='not_claimed',
                  publisher_sha256=args.expected_publisher_sha256, runs=runs, judgments=judgments, checks=checks,
                  exclusions=excluded, recorded_paths='historical_not_rewritten',
                  excluded_only='run-root bin/ and Python bytecode caches',
                  engine_invoked=False, judge_invoked=False, gcp_used=False,
                  failed_runs_preserved=True, README_argument=args.readme,
                  copied_files={name: watches[path] for name, path in sorted(exports.items())})
    with (destination / 'publication.json').open('xb') as stream:
        stream.write(encoded(report))
    files = sorted(p for p in destination.rglob('*') if p.is_file())
    with (destination / 'SHA256SUMS').open('xb') as stream:
        stream.write(''.join(sha(path) + '  ' + str(path.relative_to(destination)) + '\n' for path in files).encode())
    print(json.dumps(dict(status=report['status'], destination=str(destination), files=len(files) + 1,
                          manifest_sha256=sha(destination / 'SHA256SUMS'), runs=len(runs), judgments=len(judgments))))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--root', type=Path, default=Path('/workspaces/E-HGP'))
    parser.add_argument('--expected-publisher-sha256')
    parser.add_argument('--run', action='append', default=[])
    parser.add_argument('--source-pins', action='append', default=[])
    parser.add_argument('--judgment', action='append', default=[])
    parser.add_argument('--checks', action='append', default=[])
    parser.add_argument('--readme')
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps(dict(status='prepared_not_executed', publisher_sha256=sha(Path(__file__)),
                              destination=str(args.root / DESTINATION), engine_invoked=False)))
        return
    execute(args)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(json.dumps(dict(status='failed', reason=str(error), partial_destination_preserved=True)))
        raise SystemExit(1)
