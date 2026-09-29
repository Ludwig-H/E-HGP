"""Two tiny read-only engine gates on an existing frozen catalogue binary.

Writes an independent receipt under --out (must not already exist).
No build, GCP, git, workload over four/five points, or large thread count.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import struct
import subprocess
import tempfile
import time

FIX = Path('/workspaces/E-HGP/build/v10-fixes/entrees_cli')
BIN = FIX / 'build-final/mhgp10_catalogue'
ROOT = Path('/workspaces/E-HGP/build/v9-open-worktree')
SRC = FIX / 'src/morsehgp3D_v10'
EXPECTED_BIN = '8ec42a51b2ecee2904c4cf594caa67b12a86f5666a28a8ade7d49b7f030c6e27'

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    args = ap.parse_args()
    out = Path(args.out).resolve()
    if out.exists():
        raise SystemExit('Refusing to overwrite an existing capture')
    before = sha(BIN)
    if before != EXPECTED_BIN:
        raise SystemExit('Existing binary changed; explicit review required')
    out.mkdir(parents=True)
    paths = [SRC / s for s in (
        'src/cloud/u32le_input.hpp', 'cli/mhgp10_catalogue.cpp',
        'cli/mhgp10_tower.cpp', 'src/catalogue/generator.cpp',
        'src/catalogue/catalogue.hpp', 'tests/regression/test_cli_input_frontiers.py')]
    paths += [FIX / 'RECU_entrees_cli.md', FIX / 'entrees_cli.patch',
              FIX / 'build-final/CMakeFiles/mhgp10_catalogue.dir/flags.make']
    sources = []
    for i, path in enumerate(paths):
        dest = out / 'sources' / ('%02d_%s' % (i, path.name))
        dest.parent.mkdir(exist_ok=True)
        shutil.copyfile(path, dest)
        sources.append({'path': str(path), 'snapshot': str(dest.relative_to(out)),
                        'sha256_before': sha(path), 'snapshot_sha256': sha(dest)})
    r = {'kind': 'preintegration_interfaces_counter_audit',
         'binary_original': str(BIN), 'binary_sha256_before': before,
         'runner_sha256': sha(__file__), 'sources': sources,
         'product_has_common_reader': (ROOT / 'morsehgp3D_v10/src/cloud/u32le_input.hpp').exists(),
         'commands': []}
    pts = [(0, 0, 0), (8, 0, 0), (0, 8, 0), (0, 0, 8)]
    payload = b''.join(struct.pack('<III', *p) for p in pts)
    (out / 'four_points.u32le').write_bytes(payload)
    failures = []
    def require(ok, label):
        if not ok:
            failures.append(label)
    with tempfile.TemporaryDirectory(prefix='mhgp10-interface-gates-') as tmp:
        frozen = Path(tmp) / 'mhgp10_catalogue'
        shutil.copy2(BIN, frozen)
        r['binary_frozen_sha256_before'] = sha(frozen)
        require(sha(frozen) == EXPECTED_BIN, 'frozen_hash')
        def run(name, options, stream=None):
            dump = out / (name + '.dump')
            fd = None
            inp = str(out / 'four_points.u32le')
            if stream is not None:
                fd, wr = os.pipe()
                try:
                    if os.write(wr, stream) != len(stream):
                        raise RuntimeError('Incomplete test fixture pipe write')
                finally:
                    os.close(wr)
                inp = '/proc/self/fd/%d' % fd
            argv = [str(frozen), inp] + options + ['--dump=' + str(dump)]
            started = time.monotonic()
            rec = {'case': name, 'argv': argv, 'input_kind': 'pipe' if fd is not None else 'regular',
                   'payload_sha256': hashlib.sha256(stream if stream is not None else payload).hexdigest(),
                   'payload_size': len(stream if stream is not None else payload)}
            if fd is not None:
                rec['pipe_verified_by_fstat'] = stat.S_ISFIFO(os.fstat(fd).st_mode)
            try:
                p = subprocess.run(argv, capture_output=True, text=True, timeout=5,
                                   pass_fds=() if fd is None else (fd,))
                rec.update(returncode=p.returncode, stdout=p.stdout, stderr=p.stderr)
            except subprocess.TimeoutExpired as e:
                rec.update(timeout=True, stdout=str(e.stdout), stderr=str(e.stderr))
                failures.append(name + '_timeout')
            finally:
                if fd is not None:
                    os.close(fd)
            rec['wall_s'] = time.monotonic() - started
            try:
                rec['json'] = json.loads(rec['stdout'])
            except (ValueError, KeyError):
                rec['json'] = {}
            rec['dump_sha256'] = sha(dump) if dump.exists() else None
            r['commands'].append(rec)
            return rec

        base = run('baseline', ['--k=2', '--threads=1', '--leaf=8'])
        require(base.get('returncode') == 0 and base['json'].get('status') == 'ok', 'baseline')
        for name, opts in (
            ('k_suffix', ['--k=2not_an_integer', '--threads=1', '--leaf=8']),
            ('leaf_wrap', ['--k=2', '--threads=1', '--leaf=4294967304']),
            ('threads_wrap', ['--k=2', '--threads=4294967297', '--leaf=8']),
        ):
            a = run(name, opts)
            require(a.get('returncode') == 0 and a['json'].get('status') == 'ok' and
                    a['json'].get('K') == 2 and a['json'].get('threads') == 1 and
                    a['dump_sha256'] == base['dump_sha256'], name + '_finding')
        bad = run('nonnumeric_control', ['--k=not_an_integer', '--threads=1', '--leaf=8'])
        require(bad.get('returncode') == 2 and bad['json'].get('reason') == 'parameter_out_of_range',
                'nonnumeric_control')
        for name, data, reason in (
            ('pipe_complete', payload, None),
            ('pipe_suffix_1', payload + b'\x01', 'size_mismatch'),
            ('pipe_suffix_11', payload + bytes(range(11)), 'size_mismatch'),
            ('pipe_complete_outside_u18', payload + struct.pack('<III', 262144, 1, 2),
             'coordinate_out_of_domain'),
        ):
            a = run(name, ['--k=2', '--threads=1', '--leaf=8'], data)
            require(a['pipe_verified_by_fstat'], name + '_pipe_kind')
            if reason is None:
                require(a.get('returncode') == 0 and a['dump_sha256'] == base['dump_sha256'], name)
            else:
                require(a.get('returncode') == 2 and a['json'].get('reason') == reason and
                        a['dump_sha256'] is None, name)
        r['binary_frozen_sha256_after'] = sha(frozen)
    r['binary_sha256_after'] = sha(BIN)
    for item in r['sources']:
        item['sha256_after'] = sha(item['path'])
        require(item['sha256_before'] == item['sha256_after'] == item['snapshot_sha256'],
                'source_changed:' + item['path'])
    require(r['binary_sha256_after'] == r['binary_frozen_sha256_after'] == EXPECTED_BIN,
            'binary_closing_hash')
    r.update(capture_status='closed' if not failures else 'inconclusive',
             failures=failures, numeric_gate='finding_reproduced', stream_gate='pass',
             scope='catalogue Release only; neither integrated product nor sanitizer qualification')
    (out / 'receipt.json').write_text(json.dumps(r, indent=2) + '\n')
    print(json.dumps({'capture_status': r['capture_status'], 'commands': len(r['commands']),
                      'failures': failures, 'numeric_gate': r['numeric_gate'], 'stream_gate': r['stream_gate']}))
    return 0 if not failures else 1

if __name__ == '__main__':
    raise SystemExit(main())
