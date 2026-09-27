#!/usr/bin/env python3
# Explicit protocol port of b_q34_resident_session_20260927 at 03decc16c.
# The pinned lifecycle helper, guards, fixed target and cooperative join are unchanged.
"""Build a reproducible private snapshot from Git objects. No cloud calls."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
import common as c


def git(*args, stdin=None):
    result = subprocess.run(['git', '-C', str(c.ROOT), *args], input=stdin, capture_output=True, check=False,
                            env=dict(__import__('os').environ, GIT_OPTIONAL_LOCKS='0', GIT_NO_REPLACE_OBJECTS='1'))
    c.need(result.returncode == 0, 'read-only git failed: ' + result.stderr.decode(errors='replace'))
    return result.stdout


def collect(commit):
    import re
    c.need(type(commit) is str and re.fullmatch('[0-9a-f]{40}', commit), 'full commit ID required')
    full = git('rev-parse', '--verify', commit + '^{commit}').decode().strip()
    tree = git('rev-parse', '--verify', full + '^{tree}').decode().strip()
    entries = {}
    for entry in git('ls-tree', '-r', '-z', '--full-tree', full, '--', *c.SOURCE_PATHS, c.DATA_SOURCE).split(b'\0'):
        if not entry:
            continue
        meta, _, raw_name = entry.partition(b'\t')
        mode, kind, oid = meta.decode().split()
        c.need(mode in ('100644', '100755') and kind == 'blob', 'regular source only')
        entries[raw_name.decode()] = oid
    # Only source/header/protocol files are needed; no audit receipts or data
    # can enter through a broad historical folder.
    entries = {name: oid for name, oid in entries.items() if name == c.DATA_SOURCE or
               name.endswith(('.hpp', '.h', '.cpp', '.cu', '.cuh', '.py', '/CMakeLists.txt'))}
    blobs = {}
    ids = sorted(set(entries.values()))
    stream = git('cat-file', '--batch', stdin=('\n'.join(ids) + '\n').encode())
    offset = 0
    for oid in ids:
        end = stream.index(b'\n', offset)
        got, kind, size = stream[offset:end].decode().split()
        size = int(size)
        raw = stream[end + 1:end + 1 + size]
        c.need(got == oid and kind == 'blob' and len(raw) == size and
               hashlib.sha1(b'blob ' + str(size).encode() + b'\0' + raw).hexdigest() == oid and
               stream[end + 1 + size:end + 2 + size] == b'\n', 'Git blob framing/hash')
        blobs[oid] = raw
        offset = end + 2 + size
    c.need(offset == len(stream) and c.DATA_SOURCE in entries, 'complete Git stream/input')
    files = {name: blobs[oid] for name, oid in entries.items()}
    files[c.DATA] = files.pop(c.DATA_SOURCE)
    files[c.PROVENANCE] = (json.dumps(dict(schema=c.SCHEMA, scope='S2_only_no_FULL',
        protocol_source='commit', commit=full, tree=tree), sort_keys=True) + '\n').encode()
    return files


def verify_committed(path, manifest):
    files, provenance = c.unpack_readonly(path, manifest)
    c.need(files == collect(provenance['commit']), 'snapshot not reproducible from committed objects')
    # The executing wrapper/validator/worker must be the published versions.
    for name in ('common.py', 'package.py', 'session.py', 'worker.py', 'compile_contract.py'):
        key = c.PREFIX + '/' + name
        c.need(key in files and files[key] == (c.HERE / name).read_bytes(), 'executing protocol differs: ' + name)
    return provenance


def build(commit, output):
    files = collect(commit)
    output.mkdir(mode=0o700, parents=True, exist_ok=False)
    archive_path = output / 'snapshot.tar.gz'
    with archive_path.open('xb') as stream:
        with gzip.GzipFile(filename='', mode='wb', fileobj=stream, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode='w') as archive:
                for name, raw in sorted(files.items()):
                    member = tarfile.TarInfo(name)
                    member.size, member.mode, member.mtime = len(raw), 0o444, 0
                    archive.addfile(member, io.BytesIO(raw))
    manifest = {name: hashlib.sha256(raw).hexdigest() for name, raw in files.items()}
    c.save(output / 'source_manifest.json', manifest)
    provenance = verify_committed(archive_path, manifest)
    result = dict(schema=c.SCHEMA, status='prepared_not_executed', GCP_used=False,
        scope='S2_only_no_FULL', provenance=provenance, snapshot_sha256=c.sha(archive_path),
        manifest_sha256=c.sha(output / 'source_manifest.json'),
        worker_sha256=manifest[c.PREFIX + '/worker.py'], helper_sha256=c.HELPER_PIN)
    c.save(output / 'PACKAGE.json', result)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--commit', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.commit, args.output), sort_keys=True))
