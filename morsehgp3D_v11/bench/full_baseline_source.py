"""Explicit pinned old source fixture; builds a comparator, never inherits old qualification."""
import hashlib
from pathlib import Path, PurePosixPath
import subprocess
import tarfile
import time

import full_campaign as full

BASELINE = full.acceleration.BASELINE
ARCHIVE_SHA256 = 'd9f8765c7284887d54bc248223fac3d00b9602aedbba1b8566e4727bdfbc8eef'
ARTIFACT = Path(__file__).resolve().parents[1] / 'receipts/qualification_performance_20261003'
need, base = full.need, full.base


def extract(archive, manifest, destination):
    need(base.digest(archive) == ARCHIVE_SHA256 and manifest['archive_sha256'] == ARCHIVE_SHA256 and
         manifest['source_commit'] == BASELINE and archive.stat().st_size == manifest['archive_bytes'],
         'baseline archive differs from pinned Git source fixture')
    rows = manifest['files']
    expected = {r['path']: r for r in rows}
    need(len(expected) == len(rows) and 0 < len(rows) <= 2000, 'baseline manifest inventory')
    destination.mkdir(parents=True, exist_ok=False)
    with tarfile.open(archive, 'r:gz') as package:
        members = package.getmembers()
        paths = [m.name.rstrip('/') for m in members]
        need(len(set(paths)) == len(paths), 'duplicate baseline archive member')
        need({m.name for m in members if m.isfile()} == set(expected), 'baseline archive exact manifest')
        need(sum(m.size for m in members) <= 32 * 1024**2, 'baseline source fixture size')
        for member in members:
            path = PurePosixPath(member.name)
            need(not path.is_absolute() and '..' not in path.parts and path.parts[0] == 'morsehgp3D_v11' and
                 (member.isdir() or member.isfile()), 'baseline archive path/type')
            if member.isdir():
                continue
            data = package.extractfile(member).read()
            row = expected[member.name]
            need(len(data) == row['bytes'] == member.size and hashlib.sha256(data).hexdigest() == row['sha256'],
                 'baseline member hash')
            target = destination / member.name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
    return destination / 'morsehgp3D_v11'


def cache_entries(path):
    result = {}
    for line in path.read_text().splitlines():
        if ':' in line and '=' in line and not line.startswith(('#', '//')):
            name, value = line.split('=', 1)
            key = name.split(':', 1)[0]
            need(key not in result, 'duplicate CMake cache entry')
            result[key] = value
    return result


def checked_pin(record):
    exe, cache = Path(record['path']), Path(record['cache_path'])
    need(exe.is_file() and base.digest(exe) == record['sha256'] and exe.stat().st_size == record['bytes'],
         'current attempt binary differs from checked build')
    need(cache.is_file() and base.digest(cache) == record['cache_sha256'], 'current attempt cache changed')
    need(base.digest(Path(record['provenance_path'])) == record['provenance_sha256'], 'build provenance changed')
    manifest = Path(record['source_manifest_path'])
    need(base.digest(manifest) == record['source_manifest_sha256'], 'source manifest changed')
    for row in full.profiles.load(manifest)['files']:
        path = Path(record['source_root']) / row['path']
        need(path.is_file() and path.stat().st_size == row['bytes'] and base.digest(path) == row['sha256'],
             'source blob changed before or during attempt: ' + row['path'])
    return {key: record[key] for key in ('source_commit', 'source_manifest_sha256', 'configuration', 'path',
            'sha256', 'bytes', 'cache_sha256', 'provenance_sha256', 'targeted_qualification_sha256',
            'targeted_inventory_sha256') if key in record}


def build(work, out, current, timeout=180, threads=16):
    manifest_path = ARTIFACT / 'baseline_source_manifest.json'
    manifest = full.profiles.load(manifest_path)
    source = extract(ARTIFACT / manifest['archive'], manifest, work / 'baseline_source')
    build_dir = work / 'baseline_build'
    current_cache = cache_entries(Path(current['cache_path']))
    keys = ('CMAKE_CXX_COMPILER', 'CMAKE_CXX_FLAGS', 'CMAKE_CXX_FLAGS_RELEASE', 'MHGP11_MARCH')
    definitions = ['-D%s=%s' % (key, current_cache[key]) for key in keys]
    commands = [['cmake', '-S', str(source), '-B', str(build_dir), '-DCMAKE_BUILD_TYPE=Release',
                 '-DMHGP11_COORD_BITS=21', '-DBUILD_TESTING=ON', *definitions],
                ['cmake', '--build', str(build_dir), '--target', 'mhgp11_full_bench', '-j', str(threads)]]
    started = time.monotonic()
    for ordinal, argv in enumerate(commands):
        left = timeout - (time.monotonic() - started)
        need(left > 0, 'baseline build deadline')
        result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=left, check=False)
        (out / ('baseline_build_%d.stdout' % ordinal)).write_bytes(result.stdout)
        (out / ('baseline_build_%d.stderr' % ordinal)).write_bytes(result.stderr)
        base.save(out / ('baseline_build_%d.json' % ordinal), dict(argv=argv, exit_code=result.returncode,
                  timeout_seconds=left, wall_seconds=time.monotonic() - started))
        need(result.returncode == 0, 'baseline comparator build failed; first logs preserved')
    cache = build_dir / 'CMakeCache.txt'
    entries = cache_entries(cache)
    need(all(entries[key] == current_cache[key] for key in keys) and entries['MHGP11_COORD_BITS'] == '21',
         'baseline compiler/options differ from qualified current profile')
    exe = build_dir / 'mhgp11_full_bench'
    record = dict(source_commit=BASELINE, source_manifest_sha256=base.digest(manifest_path),
                  source_manifest_path=str(manifest_path), source_root=str(work / 'baseline_source'),
                  configuration='baseline_895680ff8_bits21', path=str(exe), sha256=base.digest(exe),
                  bytes=exe.stat().st_size, cache_path=str(cache), cache_sha256=base.digest(cache),
                  build_wall_seconds=time.monotonic() - started, build_commands=commands,
                  qualification_scope='comparator build only; no old gates rerun or inherited')
    provenance = out / 'baseline_build_provenance.json'
    base.save(provenance, record)
    record.update(provenance_path=str(provenance), provenance_sha256=base.digest(provenance))
    return record
