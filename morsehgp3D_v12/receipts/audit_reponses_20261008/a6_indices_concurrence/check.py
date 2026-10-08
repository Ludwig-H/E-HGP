#!/usr/bin/env python3
"""Source hashes and two-line proposal application only; no compilation/model/engine."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def main():
    source = Path(sys.argv[1])
    c = json.loads((HERE / 'capture.json').read_text())
    bodies = {p: (source / p).read_bytes() for p in c['sources']}
    need({p: sha(b) for p, b in bodies.items()} == c['sources'], 'source snapshot changed')
    patch = HERE / 'proposition.patch'
    need(sha(patch.read_bytes()) == c['patch_sha256'], 'proposal changed')
    with tempfile.TemporaryDirectory(prefix='audit-a6-hints-') as tmp:
        p = Path(tmp) / 'morsehgp3D_v12/src/tower/forest_kernel.cpp'
        p.parent.mkdir(parents=True)
        p.write_bytes(bodies['src/tower/forest_kernel.cpp'])
        for check in (True, False):
            subprocess.run(['git', 'apply'] + (['--check'] if check else []) + [str(patch)],
                           cwd=tmp, check=True, capture_output=True)
        candidate = p.read_bytes()
    need(sha(candidate) == c['postimage_sha256'], 'proposal postimage')
    original = bodies['src/tower/forest_kernel.cpp'].decode()
    pairs = [
        ('return std::atomic_ref<u32>(leaves[p]).load(std::memory_order_relaxed);',
         'return std::atomic_ref<u32>(leaves[p]).load(std::memory_order_acquire);'),
        ('std::atomic_ref<u32>(leaves[p]).store(find_read(c, node), std::memory_order_relaxed);',
         'std::atomic_ref<u32>(leaves[p]).store(find_read(c, node), std::memory_order_release);'),
    ]
    for before, after in pairs:
        need(original.count(before) == 1, 'unique edit')
        original = original.replace(before, after)
    need(original.encode() == candidate, 'only the two memory-order edits')
    need(all((source / p).read_bytes() == b for p, b in bodies.items()), 'source mutated during read')
    print(json.dumps(dict(source_files=len(bodies), patch_applicable=True, changes=2,
        up_orders_unchanged=True, postimage_sha256=c['postimage_sha256'], native_runs=0,
        scope='hashes/application only; mathematical arguments in README'), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
