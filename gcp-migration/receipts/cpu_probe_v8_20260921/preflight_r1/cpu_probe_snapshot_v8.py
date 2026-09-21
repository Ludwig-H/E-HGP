#!/usr/bin/env python3
"""Build a fresh local v8 source/data package; never calls GCP or ships ELF.

The caller supplies the CLOSED source hash map and an explicit case plan.
No automatic inventory discovery can silently qualify mobile sources.
Data are named copies with declared hashes, never symlinks into audits/.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import tarfile
import cpu_probe_session_v8 as session
import cpu_probe_worker_v8 as worker


def closed_sources(path):
    before = worker.strict_json(path.read_text())
    closing_path = path.parent / 'COMPLETION.json'
    after = worker.strict_json(closing_path.read_text())
    worker.need(before.get('schema') == 'mhgp8_wspd_q34_compiled_mutants_v1' and
                after.get('status') == 'passed' and after.get('closing_errors') == [] and
                after.get('manifest_sha256') == worker.sha(path), 'closed tranche31 authority')
    sources = before['source_sha256']
    worker.need(type(sources) is dict and len(sources) == 196 and
                after.get('source_sha256_after') == sources, 'closed 196 source identity')
    for row in after['records']:
        worker.need(Path(row['path']).name == row['path'] and
                    worker.sha(path.parent / row['path']) == row['sha256'], 'authority record hash')
    return {name: pin for name, pin in sources.items() if name.startswith('morsehgp3D_v8/')}


def build(root, source_manifest, plan_path, data, output, closed_capture=False):
    root = root.resolve()
    worker.need(not output.exists() and not output.is_symlink(), 'fresh package directory')
    manifest_input_bytes = source_manifest.read_bytes()
    manifest_input_pin = hashlib.sha256(manifest_input_bytes).hexdigest()
    closing_path = source_manifest.parent / 'COMPLETION.json'
    closing_pin = worker.sha(closing_path) if closed_capture else None
    sources = closed_sources(source_manifest) if closed_capture else worker.strict_json(manifest_input_bytes)
    worker.need(type(sources) is dict and sources, 'explicit closed source hash map required')
    names, manifest = {}, dict(sources)
    for name, pin in sources.items():
        worker.need(worker.safe_name(name) and name.startswith('morsehgp3D_v8/'), 'source prefix')
        path = root / name
        worker.need(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(root) and
                    worker.sha(path) == pin, 'source pin mismatch: ' + name)
        names[name] = path
    helper = root / worker.HELPER
    worker.need(worker.sha(helper) == worker.HELPER_SHA, 'helper pin')
    names[worker.HELPER] = helper
    manifest[worker.HELPER] = worker.HELPER_SHA
    for name, original, pin in data:
        destination = 'data/' + name
        worker.need(worker.safe_name(destination) and destination.endswith('.u16le') and
                    '/' not in name and destination not in names, 'distinct flat u16le name')
        path = Path(original).resolve()
        worker.need(path.is_file() and not Path(original).is_symlink() and worker.sha(path) == pin,
                    'input hash/type mismatch')
        names[destination] = path
        manifest[destination] = pin
    names[worker.PLAN] = plan_path.resolve()
    manifest[worker.PLAN] = worker.sha(plan_path)
    worker.validate_manifest(manifest)
    worker.validate_plan(worker.strict_json(plan_path.read_text()), manifest)
    output.mkdir(mode=0o700)
    archive_path = output / 'snapshot.tar.gz'
    # Files only, immutable modes; extraction may create parent directories.
    # No builds, symlinks, devices, special members or audit trees are added.
    with tarfile.open(archive_path, 'x:gz') as archive:
        for name, path in sorted(names.items()):
            raw = path.read_bytes()
            worker.need(hashlib.sha256(raw).hexdigest() == manifest[name], 'input changed during snapshot')
            member = tarfile.TarInfo(name)
            member.size, member.mode, member.mtime = len(raw), 0o444, 0
            archive.addfile(member, io.BytesIO(raw))
    for name, path in names.items():
        worker.need(worker.sha(path) == manifest[name], 'input changed before snapshot closure')
    worker.need(worker.sha(source_manifest) == manifest_input_pin, 'source authority changed during snapshot')
    if closed_capture:
        worker.need(worker.sha(closing_path) == closing_pin, 'source closure changed during snapshot')
    session.validate_snapshot(archive_path, manifest)
    session.save(output / 'source_manifest.json', manifest)
    record = dict(status='prepared_not_executed', public_status='not_claimed',
                  source_manifest_input=str(source_manifest.resolve()), source_manifest_input_sha256=manifest_input_pin,
                  closed_source_authority=closed_capture, completion_sha256=closing_pin,
                  snapshot_sha256=worker.sha(archive_path), manifest_sha256=worker.sha(output / 'source_manifest.json'),
                  controller_sha256=worker.sha(Path(session.__file__)), worker_sha256=worker.sha(Path(worker.__file__)),
                  inputs={name: dict(original_path=str(path), sha256=manifest[name]) for name, path in names.items()},
                  GCP_used=False, GPU_executed=False, FULL_executed=False)
    session.save(output / 'PACKAGE.json', record)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent.parent)
    authority = parser.add_mutually_exclusive_group(required=True)
    authority.add_argument('--source-manifest', type=Path)
    authority.add_argument('--authority-capture', type=Path,
                           help='closed tranche31 mutant capture with 196 source pins')
    parser.add_argument('--plan', type=Path, required=True)
    parser.add_argument('--data', nargs=3, action='append', default=[], metavar=('NAME', 'PATH', 'SHA256'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = args.source_manifest if args.source_manifest else args.authority_capture / 'MANIFEST.json'
    print(json.dumps(build(args.root, source, args.plan, args.data, args.output,
                           closed_capture=args.authority_capture is not None), sort_keys=True))


if __name__ == '__main__':
    main()
