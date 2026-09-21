#!/usr/bin/env python3
"""Package exact spatial inputs and tranche34 sources locally; never call GCP.

Explicit port of cpu_probe_snapshot_v8.py (SHA4d05cd7c...), replacing its
tranche31/196-source authority and prefix inputs. Native r2 binaries are
checked locally but never transported. All seven pieces and raw ID maps
are archived for each supplied scene; the execution plan may select a
strict subset, retaining the true whole-file sizes. No GPU/FULL claim.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import tarfile
import q34_spatial_session_v8 as session
import q34_spatial_worker_v8 as worker

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / 'build/v8_q4_seed_cells_r2_20260921'


def build(root, prepared, plan_path, output):
    root, output, plan_path = Path(root).resolve(), Path(output).absolute(), Path(plan_path).resolve()
    worker.need(root == ROOT and not output.exists() and not output.is_symlink(), 'fixed source root / fresh output')
    validator = worker.load_validator(root)
    native = validator.authority(BUILD, True)
    names, generated = {}, {}
    source_pins = dict(native['source_sha256'])
    for name in (*validator.PROTOCOL_SOURCES, *worker.PROTOCOL_NAMES, worker.HELPER):
        source_pins[name] = worker.sha(root / name)
    worker.need(source_pins[worker.HELPER] == worker.HELPER_SHA, 'pinned legacy command helper')
    for name, pin in source_pins.items():
        path = root / name
        worker.need(worker.safe_name(name) and path.is_file() and not path.is_symlink() and
                    path.resolve().is_relative_to(root) and worker.sha(path) == pin, 'source pin/type: ' + name)
        names[name] = path
    names[worker.PLAN] = plan_path
    for destination, original in ((worker.AUTHORITY_MANIFEST, 'MANIFEST.json'),
                                  (worker.AUTHORITY_COMPLETION, 'COMPLETION.json')):
        names[destination] = validator.AUTHORITY / original
    scenes, seen = [], set()
    worker.need(prepared, 'at least one prepared whole scene required')
    for scene, directory in prepared:
        worker.need(type(scene) is str and re.fullmatch(r'[A-Za-z0-9_-]+', scene) and scene not in seen,
                    'distinct canonical scene name')
        seen.add(scene)
        directory = Path(directory).resolve()
        validator.preparation.read(directory)
        original = worker.strict_json((directory / 'MANIFEST.json').read_bytes())
        for entry in directory.iterdir():
            names['data/' + scene + '/' + entry.name] = entry
        names['data/' + scene + '/RAW.bin'] = Path(original['raw']['path'])
        scenes.append(dict(scene=scene, original_directory=str(directory)))
    generated[worker.PREPARATIONS] = validator.preparation.canonical_json(
        dict(schema='mhgp8_q34_spatial_preparations_v1', scenes=scenes))
    manifest = {name: worker.sha(path) for name,path in names.items()}
    manifest.update({name: hashlib.sha256(raw).hexdigest() for name,raw in generated.items()})
    worker.need(all(manifest.get(name) == pin for name,pin in source_pins.items()), 'source changed during preparation')
    worker.validate_manifest(manifest)
    def read_bytes(name):
        return generated[name] if name in generated else names[name].read_bytes()
    cases = worker.validate_plan(worker.strict_json(read_bytes(worker.PLAN)), manifest)
    worker.validate_authority(manifest, read_bytes)
    worker.validate_data(cases, read_bytes)
    worker.validate_preparations(manifest, cases, read_bytes, validator)
    output.mkdir(parents=True, mode=0o700)
    record = dict(status='failed', public_status='not_claimed', native_sources=216,
        protocol_sources=len(source_pins)-216, native_authority_sha256=worker.AUTHORITY_PINS,
        cases=cases, scenes=scenes, GCP_used=False, GPU_executed=False, FULL_executed=False)
    error = None
    try:
        archive_path = output / 'snapshot.tar.gz'
        with tarfile.open(archive_path, 'x:gz') as archive:
            for name in sorted(manifest):
                raw = read_bytes(name)
                worker.need(hashlib.sha256(raw).hexdigest() == manifest[name], 'input changed during snapshot: ' + name)
                member = tarfile.TarInfo(name)
                member.size, member.mode, member.mtime = len(raw), 0o444, 0
                archive.addfile(member, io.BytesIO(raw))
        session.save(output / 'source_manifest.json', manifest)
        session.validate_snapshot(archive_path, manifest)
        worker.need(all(worker.sha(path) == manifest[name] for name,path in names.items()), 'snapshot input closure')
        validator.authority(BUILD, True)
        record.update(status='prepared_not_executed', snapshot_sha256=worker.sha(archive_path),
            manifest_sha256=worker.sha(output / 'source_manifest.json'),
            controller_sha256=worker.sha(session.__file__), worker_sha256=worker.sha(worker.__file__),
            inputs={name: dict(original_path=str(path), sha256=manifest[name]) for name,path in names.items()},
            native_artifact_sha256=native['artifact_sha256'])
    except BaseException as cause:
        error = cause
        record['error'] = type(cause).__name__ + ': ' + str(cause)
    session.save(output / 'PACKAGE.json', record)
    if error is not None:
        raise error
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--prepared', nargs=2, action='append', required=True, metavar=('SCENE', 'DIRECTORY'))
    parser.add_argument('--plan', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.root, args.prepared, args.plan, args.output), sort_keys=True, allow_nan=False))


if __name__ == '__main__':
    main()
