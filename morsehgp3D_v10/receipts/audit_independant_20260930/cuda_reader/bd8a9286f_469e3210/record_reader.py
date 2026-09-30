import contextlib
import copy
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
from unittest import mock

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    out = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(out)
    return out

reader = module('cuda_reader_audit', HERE / 'cuda_probe.py')
before = module('cuda_reader_before', HERE / 'cuda_probe_before.py')
base = dict(status='ok', device='manual GPU fixture', cc='12.0', sms=188, global_mem_gib=94.9,
            cuda_error='no error', i128_pairs=16777216, i128_equal_cases=2011255, i128_mismatches=0,
            timing_ok=True, loop64_gops=100.0, loop128_gops=50.0, i128_over_i64=2.0,
            h2d_gbps=20.0, d2h_gbps=20.0, launch_sync_us=5.0, launch_async_us=2.0,
            loops='modular_unsigned')
cases = [('manual_complete', 0, json.dumps(base), 0),
         ('status_only', 0, '{"status":"ok"}', 8),
         ('duplicate_status', 0, json.dumps(base)[:-1] + ',"status":"ok"}', 8)]
for name, field, value in [('mismatch', 'i128_mismatches', 1), ('wrong_type', 'sms', True),
                           ('missing_equal', 'i128_equal_cases', 0), ('negative_rate', 'loop64_gops', -1),
                           ('enormous_integer_rate', 'loop64_gops', 10**400),
                           ('enormous_integer_memory', 'global_mem_gib', 10**400)]:
    rec = copy.deepcopy(base)
    rec[field] = value
    cases.append((name, 0, json.dumps(rec), 8))
cases.extend([('overflow_float_lexeme', 0, json.dumps(base).replace('100.0', '1e400'), 8),
              ('nonstandard_nan', 0, json.dumps(base).replace('100.0', 'NaN'), 8),
              ('no_cuda', 2, '{"status":"no_cuda"}', 2)])
results = []
for name, code, raw, expected in cases:
    old = before.check_output(code, raw)
    try:
        got, why = reader.verdict(code, raw)
        result = dict(name=name, code=code, raw=raw, expected=expected, got=got, reasons=why,
                      old_reader_code=old, conforms=got == expected)
    except Exception as e:
        result = dict(name=name, code=code, raw=raw, expected=expected, exception=type(e).__name__,
                      detail=str(e), old_reader_code=old, conforms=False)
    results.append(result)

paths = []
for stage in ['compile_spawn_oserror', 'probe_spawn_oserror']:
    with tempfile.TemporaryDirectory(prefix='mhgp10-reader-pure-') as tmp:
        out, work = Path(tmp)/'out', Path(tmp)/'work'
        real_exists = os.path.exists
        def exists(p):
            return True if str(p).endswith('/bin/nvcc') else real_exists(p)
        compiled = reader.subprocess.CompletedProcess(['fake_nvcc'], 0, 'compile captured', '')
        error = PermissionError(13, 'manual simulated spawn failure')
        effects = [error] if stage == 'compile_spawn_oserror' else [compiled, error]
        run = mock.Mock(side_effect=effects)
        stdout = io.StringIO()
        with mock.patch.object(sys, 'argv', ['reader', '--out', str(out), '--work', str(work)]), \
             mock.patch.object(reader.glob, 'glob', return_value=[]), \
             mock.patch.object(reader.os.path, 'exists', side_effect=exists), \
             mock.patch.object(reader.subprocess, 'run', run), contextlib.redirect_stdout(stdout):
            try:
                result = dict(code=reader.main())
            except Exception as e:
                result = dict(exception=type(e).__name__, detail=str(e))
        result.update(name=stage, subprocess_calls=run.call_count, expected_attempt=True,
                      attempt_exists=(out/'attempt.json').exists(),
                      output_files=sorted(p.name for p in out.iterdir()), stdout=stdout.getvalue())
        paths.append(result)

# Independent results above, then a bounded replay of the developer's pure self-test.
self_stdout = io.StringIO()
with contextlib.redirect_stdout(self_stdout):
    self_code = reader._self_test()
print(json.dumps(dict(optimized=not __debug__, cases=results, simulated_paths=paths,
                     developer_self_test=dict(code=self_code, stdout=self_stdout.getvalue()),
                     real_cuda_processes=0), ensure_ascii=False, indent=2))
