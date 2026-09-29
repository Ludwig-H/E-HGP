"""Short, isolated replay of the existing corrected consumer gate, never old gates."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import tempfile
import time
from datetime import datetime, timezone

OUT = Path(__file__).resolve().parent
FIX = Path('/workspaces/E-HGP/build/v10-fixes/pool')
SRC = FIX / 'src/morsehgp3D_v10'
BIN = FIX / 'build/mhgp10_fault'
ROOT = OUT.parents[3]
TARGET = OUT / 'receipt.json'
if TARGET.exists():
    raise SystemExit('Existing capture: refuse overwrite')
TMP = Path(tempfile.mkdtemp(prefix='mhgp10-consumer-r2.', dir='/tmp'))

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

sources = sorted(p for p in (SRC / 'src').rglob('*')
                 if p.is_file() and p.suffix in ('.cpp', '.hpp', '.def'))
sources += [SRC / 'tests/unit/fault_main.cpp', Path(__file__)]
metadata = [FIX / 'build/CMakeFiles/mhgp10_fault.dir/link.txt',
            FIX / 'build/CMakeFiles/mhgp10_fault.dir/flags.make',
            FIX / 'build/CMakeFiles/mhgp10_core.dir/link.txt',
            FIX / 'build/CMakeFiles/mhgp10_core.dir/flags.make']
tracked = sources + metadata + [BIN, ROOT / 'src/sched/pool.cpp']
before = {str(p): sha(p) for p in tracked}
receipt = dict(status='running', scope='existing_corrected_binary_not_integrated',
               utc_start=datetime.now(timezone.utc).isoformat(),
               temporary_snapshot=str(TMP), sources_before=before,
               source_build_relation='Existing binary, not rebuilt here; live sources and build metadata copied and hashed before/after, not a retroactive build attestation',
               commands=[])

def save():
    TARGET.write_text(json.dumps(receipt, indent=2) + '\n')

save()
try:
    for p in sources:
        rel = p.relative_to(SRC) if p.is_relative_to(SRC) else Path('capture_record.py')
        dest = TMP / 'source' / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(p, dest)
        if sha(dest) != before[str(p)]:
            raise RuntimeError('Snapshot source differs: ' + str(p))
    exe = TMP / 'mhgp10_fault'
    shutil.copy2(BIN, exe)
    if sha(exe) != before[str(BIN)]:
        raise RuntimeError('Snapshot executable differs')
    receipt['snapshot_binary_before'] = sha(exe)
    for p in metadata:
        name = p.parent.name + '_' + p.name
        shutil.copy2(p, OUT / name)

    # These are developer-owned completed artifacts, only observed and frozen;
    # the replay below is separate and does not turn them into our execution.
    observed = {}
    for name in ('asan_fault.stdout', 'asan_fault.stderr', 'asan_fault.code',
                 'tsan_fault.stdout', 'tsan_fault.stderr', 'tsan_fault.code',
                 'tsan_unit.stdout', 'tsan_unit.stderr', 'tsan_unit.code',
                 'tsan_unit_repetitions.stderr'):
        p = FIX / 'apres/sanitizers' / name
        h = sha(p)
        dest = OUT / 'developer_observed' / name
        dest.parent.mkdir(exist_ok=True)
        shutil.copy2(p, dest)
        observed[name] = dict(source=str(p), before=h, copy=sha(dest), after=sha(p))
        if len(set(observed[name][k] for k in ('before', 'copy', 'after'))) != 1:
            raise RuntimeError('Observed artifact changed: ' + str(p))
    receipt['developer_artifacts_observed_only'] = observed

    begin = time.monotonic()
    argv = [str(exe), 'entry_points']
    process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               cwd=TMP, start_new_session=True)
    timed_out = False
    try:
        stdout, stderr = process.communicate(timeout=35)
    except subprocess.TimeoutExpired:
        timed_out = True
        os.killpg(process.pid, signal.SIGKILL)
        stdout, stderr = process.communicate()
    (OUT / 'entry_points.stdout').write_bytes(stdout)
    (OUT / 'entry_points.stderr').write_bytes(stderr)
    receipt['commands'].append(dict(argv=argv, code=process.returncode,
        timed_out=timed_out, elapsed_s=time.monotonic()-begin,
        stdout_sha256=hashlib.sha256(stdout).hexdigest(),
        stderr_sha256=hashlib.sha256(stderr).hexdigest()))
    if process.returncode or timed_out:
        raise RuntimeError('Consumer gate did not complete successfully')
    text = stdout.decode()
    pattern = (r'fault_(catalogue|tower) allocations (\d+) essais (\d+) injections (\d+) '
               r'\(hors appelant (\d+)\) refus_memory_budget (\d+) absorbees (\d+)')
    rows = {}
    for m in re.finditer(pattern, text):
        rows[m[1]] = dict(zip(('allocations', 'runs', 'injections', 'off_main', 'refused', 'absorbed'),
                             (int(v) for v in m.groups()[1:])))
    if set(rows) != {'catalogue', 'tower'} or not text.endswith('fault_ok\n') or 'ECHEC' in text:
        raise RuntimeError('Gate output malformed or incomplete')
    for row in rows.values():
        if not (row['runs'] >= 60 and row['injections'] == row['runs'] and
                row['off_main'] > 0 and row['refused'] >= 40 and
                row['refused'] + row['absorbed'] == row['injections']):
            raise RuntimeError('Consumer coverage or accounting too weak')
    receipt['independent_output_check'] = rows
    receipt['snapshot_binary_after'] = sha(exe)
    if receipt['snapshot_binary_before'] != receipt['snapshot_binary_after']:
        raise RuntimeError('Snapshot binary changed')
    receipt['status'] = 'completed'
except BaseException as error:
    receipt['status'] = 'failed'
    receipt['error'] = str(error)
    raise
finally:
    receipt['sources_after'] = {str(p): sha(p) for p in tracked}
    receipt['source_closure'] = receipt['sources_before'] == receipt['sources_after']
    if not receipt['source_closure']:
        receipt['status'] = 'invalid_source_drift'
    receipt['utc_end'] = datetime.now(timezone.utc).isoformat()
    save()
print(json.dumps({k: v for k, v in receipt.items()
                  if k not in ('sources_before', 'sources_after', 'developer_artifacts_observed_only')}, indent=2))
