import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

Q = Path(__file__).resolve().parent
SRC = Q / 'final_source'
OUT = Q / 'closed_results'
OUT.mkdir(exist_ok=False)
commands = []
paths = [p for d in ('src', 'cli', 'cmake', 'tests', 'reference', 'bench/frontier')
         for p in (SRC / d).rglob('*') if p.is_file() and '__pycache__' not in str(p)]
paths += [SRC / 'CMakeLists.txt']
def hashes():
    return {str(p.relative_to(SRC)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)}
before = hashes()
def run(name, argv, expected=0):
    start = time.time()
    done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
    (OUT / (name + '.stdout')).write_bytes(done.stdout)
    (OUT / (name + '.stderr')).write_bytes(done.stderr)
    commands.append({'name': name, 'argv': argv, 'returncode': done.returncode,
                     'expected': expected, 'elapsed_s': time.time() - start})
    if done.returncode != expected:
        (OUT / 'failed_execution.json').write_text(json.dumps(commands, indent=2) + '\n')
        raise RuntimeError(name + ': unexpected exit ' + str(done.returncode))
run('rank_release', [str(Q / 'final_build/mhgp10_rank_search')])
for mode in ('mutant_mid', 'mutant_product'):
    run(mode, [str(Q / mode / 'check')], expected=1)
for flag in ([], ['-O']):
    suffix = 'normal' if not flag else 'optimized'
    for test in ('test_cover_band.py', 'test_dev_quotas.py'):
        run(test[:-3] + '_' + suffix, [sys.executable, '-B'] + flag + [str(SRC / 'tests/points' / test)])
run('ctest_final', ['ctest', '--test-dir', str(Q / 'final_build'), '-R',
                   '^(mhgp10_rank_search|mhgp10_cover_band_structural|mhgp10_dev_quotas|mhgp10_points_cover|mhgp10_regression_multiplicity_refusal)$',
                   '--output-on-failure'])
run('exporter_compile_final', ['g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                              '-DMHGP10_FRONTIER_ENGINE_COMMIT="d679ae29d-plus-rank-fix-final"',
                              '-I' + str(SRC / 'src'), str(Q / 'native_foundations/export_frontier.cpp'),
                              str(Q / 'final_build/libmhgp10_core.a'), '-pthread',
                              '-o', str(Q / 'native_foundations/export_frontier_final')])
for flag in ([], ['-O']):
    suffix = 'normal' if not flag else 'optimized'
    run('native_final_' + suffix, [sys.executable, '-B'] + flag +
        [str(SRC / 'tests/points/test_cover_band_native.py'), '--foundations=' + str(Q / 'native_foundations'),
         '--exporter=' + str(Q / 'native_foundations/export_frontier_final'),
         '--out=' + str(OUT / ('native_' + suffix))])
a = json.loads((OUT / 'native_normal/receipt.json').read_text())
b = json.loads((OUT / 'native_optimized/receipt.json').read_text())
for i, (x, y) in enumerate(zip(a['clouds'], b['clouds'])):
    for key in ('export_sha256', 'cloud_sha256', 'point0_date', 'point0_expected', 'examined', 'selected'):
        if x[key] != y[key]:
            raise RuntimeError('normal/-O mismatch on case' + str(i) + ': ' + key)
after = hashes()
if before != after:
    raise RuntimeError('source closure changed during tests')
report = {'status': 'PASS', 'source_before': before, 'source_after': after,
          'commands': commands, 'native_normal_optimized_equal': True,
          'engine_binary_sha256': hashlib.sha256((Q / 'native_foundations/export_frontier_final').read_bytes()).hexdigest(),
          'GCP_used': False, 'profile': 'quantized_u18_input_only',
          'scope': 'targeted development regression, no FULL/G4 contract or statistical qualification'}
(OUT / 'execution.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
manifest = []
for p in sorted(OUT.rglob('*')):
    if p.is_file():
        manifest.append(hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + str(p.relative_to(OUT)))
(OUT / 'SHA256SUMS').write_text('\n'.join(manifest) + '\n')
print(json.dumps({'status': 'PASS', 'commands': len(commands), 'source_files': len(before)}, sort_keys=True))
