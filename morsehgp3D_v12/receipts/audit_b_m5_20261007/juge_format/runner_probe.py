"""Vrai main MES-M5, seuls les appels externes et leurs fichiers de sortie sont doubles.

Les fichiers d'entree et sorties externes sont synthetiques, jamais passes au moteur.
Ce temoin ne qualifie ni GPU ni geometrie ; il audite uniquement l'admission des preuves.
"""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import sys
import tempfile
import types
from unittest.mock import patch

FIXTURES = ['coquille24_k2_l16', 'coquille48_k5_l24', 'coquille48_k5_l8_m8',
            'coquille48_u32_k5_l24', 'bord_u32_k2_l5', 'uniforme_u32_k3_l8']


def check(value, message):
    if not value:
        raise RuntimeError(message)


def load(root):
    path = root / 'morsehgp3D_v12/microbancs/mes_m5_parcours/scripts/g4_traversal_bench.py'
    spec = importlib.util.spec_from_file_location('audit_m5_runner', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def one(module, root, scenario):
    with tempfile.TemporaryDirectory(prefix='ehgp-m5-judge-audit-') as temp:
        folder = Path(temp)
        out, work, data = [folder / n for n in ('out', 'work', 'data')]
        data.mkdir()
        for frame in ('ng00', 'ng01', 'ng02'):
            for suffix in ('.u32le', '.ids.u32le'):
                (data / ('lidar_' + frame + suffix)).write_bytes(bytes(12))
        library = folder / 'libmhgp11.a'
        library.write_bytes(b'synthetic-library-never-executed')
        selected = [FIXTURES[3]] if scenario == 'missing_five_fixtures' else FIXTURES
        seen = []

        class Session:
            def __init__(self, destination):
                self.logs = destination / 'logs'
                self.logs.mkdir(parents=True)
                self.refusals, self.steps = [], []

            def run(self, name, argv, timeout):
                argv = [str(a) for a in argv]
                seen.append(name)
                self.steps.append({'step': name, 'code': 0})
                code, stdout = 0, ''
                if name == 'm5_build':
                    build = work / 'b_m5'
                    build.mkdir(parents=True)
                    for tool in ('dump', 'identity', 'bench'):
                        (build / ('mhgp12_traversal_' + tool)).write_bytes(b'synthetic-binary-never-executed')
                    (build / 'mhgp12_v11_traversal_timing').write_bytes(b'synthetic-timing-never-executed')
                elif name == 'fixtures':
                    dest = work / 'fixtures'
                    dest.mkdir()
                    for n in selected:
                        (dest / (n + '.bin')).write_bytes(b'synthetic-external-reference')
                    (dest / 'fixtures.json').write_text(json.dumps({'fixtures': [
                        {'name': n, 'present': True, 'controls_ok': True} for n in selected]}))
                elif name == 'identity_host':
                    target = Path(argv[argv.index('--json') + 1])
                    dumps = argv[argv.index('--json') + 2:]
                    rows = [{'phase': 'unit', 'ok': True}]
                    rows += [{'phase': 'identity', 'dump': p, 'identity': True, 'nodes_equal': True, 'status': 0,
                              'mutants': {m: {'killed': True} for m in module.MUTANTS}} for p in dumps]
                    target.write_text('\n'.join(json.dumps(row) for row in rows))
                    if scenario == 'host_exit_2':
                        code = 2
                elif name == 'device_fixtures':
                    dumps = [argv[i + 1] for i, v in enumerate(argv) if v == '--dump']
                    target = Path(argv[argv.index('--json') + 1])
                    target.write_text(json.dumps({'cases': [
                        {'dump': p, 'identity': True, 'mutants': {m: {'killed': True} for m in module.MUTANTS}}
                        for p in dumps]}))
                    if scenario == 'device_fixture_exit_7':
                        code = 7
                elif name.startswith('v11_'):
                    passes = 1 if scenario == 'one_cold_v11_pass' else 10
                    stdout = json.dumps({'kmax': int(argv[3]), 'leaf_size': int(argv[4]), 'workers': 48,
                                         'passes': passes, 'traversal_ns': [60000000] * passes,
                                         'catalogue_nodes': 1, 'catalogue_filter_tests': 3})
                elif name.startswith('gpu_'):
                    source = argv[argv.index('--dump') + 1]
                    k, leaf = [int(v) for v in re.search(r'_k(\d+)_l(\d+)', source).groups()]
                    n = int(argv[argv.index('--reps') + 1])
                    raw = 60.0 if scenario in ('false_gpu_median', 'honest_slow_gpu') else 6.0
                    reported = 60.0 if scenario == 'honest_slow_gpu' else 6.0
                    case = {'dump': source, 'kmax': k, 'leaf_size': leaf, 'total_ms': [raw] * n,
                            'median_ms': {'total': reported, 'resident': 5.0}, 'timed_allocations': 0,
                            'identity': True}
                    if scenario == 'wrong_dump':
                        case['dump'] += '.wrong'
                    Path(argv[argv.index('--json') + 1]).write_text(json.dumps({'cases': [case]}))
                elif name != 'm5_configure' and not name.startswith('sanitizer_'):
                    raise RuntimeError('external stage unexpected: ' + name)
                self.steps[-1]['code'] = code
                return code, stdout, ''

        def parallel(session, commands, jobs, timeout):
            result = {}
            for name, argv in commands:
                Path(argv[5]).write_bytes(b'synthetic-external-reference')
                result[name] = (0, json.dumps({'nodes': 1, 'filter_tests': 3}))
            return result

        fake = types.SimpleNamespace(Session=Session, now=lambda: 'synthetic-time',
                                     sha256_file=lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest(),
                                     find_nvcc=lambda _: '/fake/cuda/bin/nvcc',
                                     environment=lambda *a: {'gpu_apps': ''})
        argv = ['g4_traversal_bench.py', '--out', str(out), '--work', str(work), '--data', str(data),
                '--repo', str(root), '--v11-lib', str(library), '--extra', '', '--jobs', '1', '--cmake', '/fake/cmake']
        if scenario == 'reduced_grid_one_process':
            argv += ['--frames', 'ng00', '--configs', '5:24', '--processes', '1']
        with patch.object(module, 'load_m2', return_value=fake), patch.object(module, 'run_parallel', side_effect=parallel), \
                patch.object(module.shutil, 'which', return_value='/fake/tool'), patch.object(sys, 'argv', argv), \
                patch.object(module.subprocess, 'Popen', side_effect=RuntimeError('external execution forbidden')), \
                contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            code = module.main()
        report = json.loads((out / 'report.json').read_text())
        gpu = json.loads(next((out / 'runs').glob('*_gpu.json')).read_text())['cases'][0]
        v11 = json.loads(next((out / 'runs').glob('*_v11.json')).read_text())
        return {'main_exit': code, 'verdict': report['verdict'], 'refused_count': len(report['refused']),
                'refused_first': report['refused'][:3], 'rejected_count': len(report['rejected']),
                'rejected_first': report['rejected'][:3], 'cases': len(report['cases']), 'fixtures': len(selected),
                'processes_required': report['rule']['processes_required'],
                'ratio_gm': list(report['stats'].values())[0]['ratio_gm'] if report['stats'] else None,
                'host_code': report['identity_host']['code'], 'device_fixture_code': report['device_fixtures']['code'],
                'example_gpu_raw_median': module.median(gpu['total_ms']), 'example_gpu_raw_samples': len(gpu['total_ms']),
                'example_gpu_declared_median': gpu['median_ms']['total'], 'example_v11_passes': v11['passes'],
                'example_v11_samples': len(v11['traversal_ns'])}


def run(root):
    module = load(root)
    scenarios = ['baseline', 'reduced_grid_one_process', 'missing_five_fixtures', 'host_exit_2',
                 'device_fixture_exit_7', 'one_cold_v11_pass', 'false_gpu_median', 'honest_slow_gpu', 'wrong_dump']
    expected = {s: ('rejete' if s == 'honest_slow_gpu' else 'refuse' if s == 'wrong_dump' else 'adopte')
                for s in scenarios}
    results = {}
    for scenario in scenarios:
        row = one(module, root, scenario)
        check(row['main_exit'] == 0 and row['verdict'] == expected[scenario], 'unexpected ' + scenario + ': ' + str(row))
        results[scenario] = row
    check(results['reduced_grid_one_process']['cases'] == 1 and
          results['reduced_grid_one_process']['processes_required'] == 1, 'reduced manifest not exercised')
    check(results['missing_five_fixtures']['fixtures'] == 1, 'partial fixtures not exercised')
    check(results['host_exit_2']['host_code'] == 2, 'host refusal not injected')
    check(results['device_fixture_exit_7']['device_fixture_code'] == 7, 'device failure not injected')
    check(results['false_gpu_median']['example_gpu_raw_median'] == 60 and
          results['false_gpu_median']['example_gpu_declared_median'] == 6, 'false median not injected')
    check(results['honest_slow_gpu']['example_gpu_raw_median'] ==
          results['honest_slow_gpu']['example_gpu_declared_median'] == 60, 'positive slow control not honest')
    check(results['one_cold_v11_pass']['example_v11_samples'] == 1, 'single cold pass not injected')
    return results
