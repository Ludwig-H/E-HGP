"""Read-only reader. External manifest SHA and complete inventory BEFORE replay."""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
FIXTURES = {'internal_k3.json', 'internal_k5.json', 'near_tie_1024_0.json',
            'near_tie_1024_1.json', 'triangle.json', 'triangle_default.json'}
WANTED = {'README.md', 'prototype.py', 'record.py', 'verify.py', 'receipt.json',
          'normal.stdout', 'normal.stderr', 'optimized.stdout', 'optimized.stderr',
          'input_sources.json', 'upstream_SHA256SUMS'} | {'fixtures/' + name for name in FIXTURES}
UPSTREAM_MANIFEST = '9ab1e991f360331a9bcfe91a84920bb1708d8370c44ac6a603fd90d161f1806e'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_archive(external_sha):
    need(len(external_sha) == 64 and all(c in '0123456789abcdef' for c in external_sha), 'bad external SHA')
    need(sha(ROOT / 'SHA256SUMS') == external_sha, 'manifest digest mismatch')
    pins = {}
    for line in (ROOT / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        need(name in WANTED and name not in pins and len(digest) == 64, 'unexpected manifest entry')
        need(not (ROOT / name).is_symlink() and sha(ROOT / name) == digest, 'entry changed: ' + name)
        pins[name] = digest
    need(set(pins) == WANTED, 'incomplete manifest')
    actual = {str(path.relative_to(ROOT)) for path in ROOT.rglob('*') if path.is_file() or path.is_symlink()}
    need(actual == WANTED | {'SHA256SUMS'}, 'actual inventory differs')
    need(sha(ROOT / 'upstream_SHA256SUMS') == UPSTREAM_MANIFEST, 'wrong upstream pin')
    provenance = json.loads((ROOT / 'input_sources.json').read_text())
    need(provenance['upstream_manifest_sha256'] == UPSTREAM_MANIFEST and
         provenance['native_invocations'] == 0 and provenance['copied_existing_exports_only'] is True,
         'wrong input provenance')
    source_manifest = dict(line.split('  ', 1)[::-1]
                           for line in (ROOT / 'upstream_SHA256SUMS').read_text().splitlines())
    need(len(source_manifest) == 37 and set(provenance['inputs']) == {'fixtures/' + n for n in FIXTURES},
         'wrong copied inventory')
    for name, row in provenance['inputs'].items():
        need(row['sha256'] == pins[name] == source_manifest[name], 'input differs from closed upstream')
    receipt = json.loads((ROOT / 'receipt.json').read_text())
    need(receipt['status'] == 'CAPTURE_PASS' and receipt['native_invocations'] == 0 and receipt['GCP_used'] is False,
         'capture status/scope')
    source_names = WANTED - {'receipt.json', 'normal.stdout', 'normal.stderr',
                             'optimized.stdout', 'optimized.stderr'}
    need(receipt['sources_before'] == receipt['sources_after'] == {n: pins[n] for n in source_names},
         'before/after source pins')
    need(receipt['upstream_inputs_after'] == {n: pins[n] for n in provenance['inputs']} and
         receipt['upstream_manifest_after'] == UPSTREAM_MANIFEST, 'upstream after-capture pins')
    need(receipt['started_unix_ns'] > 0 and receipt['ended_unix_ns'] >= receipt['started_unix_ns'], 'capture clock')
    need(datetime.fromisoformat(receipt['ended_utc']) >= datetime.fromisoformat(receipt['started_utc']), 'UTC clock')
    need(len(receipt['commands']) == 2, 'wrong commands count')
    for row, mode in zip(receipt['commands'], ('normal', 'optimized')):
        argv = [row['argv'][0], '-B'] + (['-O'] if mode == 'optimized' else []) + ['prototype.py']
        need(row['mode'] == mode and row['argv'] == argv and Path(argv[0]).name.startswith('python'), 'wrong command')
        need(row['returncode'] == 0 and row['ended_unix_ns'] >= row['started_unix_ns'] > 0, 'wrong command result')
        need(row['stdout'] == mode + '.stdout' and row['stderr'] == mode + '.stderr', 'wrong streams')
        need(row['stdout_sha256'] == pins[row['stdout']] and row['stderr_sha256'] == pins[row['stderr']], 'stream pins')
        need((ROOT / row['stderr']).read_bytes() == b'', 'capture stderr')
    need((ROOT / 'normal.stdout').read_bytes() == (ROOT / 'optimized.stdout').read_bytes(), 'normal/-O differ')
    return pins


def main():
    need(len(sys.argv) == 2, 'require external manifest SHA')
    pins = check_archive(sys.argv[1])
    mode = 'optimized' if sys.flags.optimize else 'normal'
    argv = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else []) + ['prototype.py']
    command = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=60)
    need(command.returncode == 0 and command.stderr == b'', 'live structural replay failed')
    need(command.stdout == (ROOT / (mode + '.stdout')).read_bytes(), 'live output differs')
    result = json.loads(command.stdout)
    need(result['status'] == 'STRUCTURAL_FRACTION_PASS' and result['longest_chain'] == 20000 and
         len(result['native_exports']) == 6 and result['native_invocations'] == 0 and result['GCP_used'] is False,
         'live scope or test floors')
    need(len(result['causal_mutants']) == 6 and
         sum(m['killed_by'] == 'coverage_disagreement' for m in result['causal_mutants']) == 5,
         'causal mutant floor')
    need(check_archive(sys.argv[1]) == pins, 'archive changed during read-only replay')
    print(json.dumps(dict(status='READ_ONLY_LIVE_PASS', manifest_entries=len(pins),
                          proof_invocations_now=1, native_invocations=0, GCP_used=False,
                          counts=result['counts']), sort_keys=True))


if __name__ == '__main__':
    main()
