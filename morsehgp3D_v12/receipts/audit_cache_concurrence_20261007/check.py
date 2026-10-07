#!/usr/bin/env python3
"""Deux petits entrelacements, ancien/corrige/mutant ; reconstruction temporaire seulement."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def command(args, **kwargs):
    r = subprocess.run(args, capture_output=True, text=True, timeout=30, **kwargs)
    need(r.returncode == 0, 'commande refusee : ' + r.stderr[-1000:])
    return r.stdout


def main():
    here = Path(__file__).resolve().parent
    repo = here.parents[2]
    capture = json.loads((here / 'capture.json').read_text())
    base = capture['base']
    records = []
    with tempfile.TemporaryDirectory(prefix='mhgp12-cache-concurrency-') as td:
        temp = Path(td)
        paths = command(['git', 'ls-tree', '-r', '--name-only', base, '--', 'morsehgp3D_v12/src/core'], cwd=repo)
        for rel in paths.splitlines():
            destination = temp / rel
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(command(['git', 'show', base + ':' + rel], cwd=repo))
        old_patch = command(['git', 'show', capture['old_snapshot_receipt'] +
                             ':morsehgp3D_v12/receipts/audit_reprise_20261007/buffer/buffer_capture.patch'], cwd=repo)
        command(['git', 'apply', '-'], input=old_patch, cwd=temp)
        source = temp / 'morsehgp3D_v12/src'
        cpp = source / 'core/buffer.cpp'
        need(hashlib.sha256(cpp.read_bytes()).hexdigest() == capture['old_buffer_sha256'], 'ancienne source')
        old = temp / 'old.cpp'; old.write_bytes(cpp.read_bytes())
        command(['git', 'apply', str(here / 'delta.patch')], cwd=temp)
        for suffix in ['cpp', 'hpp']:
            rel = 'morsehgp3D_v12/src/core/buffer.' + suffix
            need(hashlib.sha256((temp / rel).read_bytes()).hexdigest() == capture['source_sha256'][rel], 'nouvelle source')
        anchor = '    if (account.held.load(std::memory_order_relaxed) > account.limit - x) return false;'
        body = cpp.read_text()
        need(body.count(anchor) == 1, 'ancre mutant')
        mutant = temp / 'mutant.cpp'; mutant.write_text(body.replace(anchor, '    return false;'))
        for version, current in [('old', old), ('corrected', cpp), ('no_reread', mutant)]:
            exe = temp / version
            command(['g++', '-std=c++20', '-O2', '-pthread', '-DMHGP12_COORD_BITS=21', '-I', str(source),
                     str(here / 'race.cpp'), str(current), '-o', str(exe)])
            for mode in ['cached', 'small']:
                observed = json.loads(command([str(exe), mode]))
                expect = version == 'corrected' or (version == 'no_reread' and mode == 'cached')
                need(observed['admitted'] == observed['first_ok'] == 1, 'admission ou premier tampon')
                need(observed['second_ok'] == int(expect), 'second tampon : ' + version + '/' + mode)
                need(observed['held'] == observed['used'] <= 1048576 and observed['evicted'] == 1, 'budget physique')
                records.append(dict(version=version, mode=mode, observation=observed))
    print(json.dumps(dict(source_sha256=capture['source_sha256'], observations=records,
                          compiler=command(['g++', '--version']).splitlines()[0],
                          scope='six_bounded_processes_no_sanitizer_no_pipeline_no_GCP'), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
