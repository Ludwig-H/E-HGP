#!/usr/bin/env python3
"""Only the frozen decision helper is executed; bootstrap distributions are injected.
No product import, numpy, fitting, resampling, native binary or cloud access.
"""
import ast
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'source/morsehgp3D_v11/bench/points_flat_summary.py'
checks = 0

def need(condition, message):
    global checks
    checks += 1
    if not condition:
        raise RuntimeError(message)

class Samples(list):
    def __ge__(self, threshold):
        return [v >= threshold for v in self]
    def __le__(self, threshold):
        return [v <= threshold for v in self]

class NP:
    @staticmethod
    def sum(values):
        return sum(values)
    @staticmethod
    def quantile(values, q):
        values = sorted(values)
        x = (len(values) - 1) * q
        lo = int(x)
        hi = min(lo + 1, len(values) - 1)
        return values[lo] + (values[hi] - values[lo]) * (x - lo)

raw = SOURCE.read_bytes()
module = ast.parse(raw, filename=str(SOURCE))
functions = [n for n in module.body if isinstance(n, ast.FunctionDef) and n.name in ('holm', 'lidar_decision')]
need(len(functions) == 2, 'exact two frozen functions')
helper = ast.Module(body=functions, type_ignores=[])
fixed = ast.parse(ast.unparse(helper))
replacement = ast.parse("rows[name]['claimed'] = adj < 0.05 and (not name.startswith('H_L2_') or (rows[name]['ci95'] is not None and rows[name]['ci95'][0] > -0.02))").body[0]
changes = 0
for fn in fixed.body:
    if fn.name == 'lidar_decision':
        loop = fn.body[-2]
        need(isinstance(loop, ast.For), 'frozen adjustment loop shape')
        need(ast.unparse(loop.body[-1]) == "rows[name]['claimed'] = adj < 0.05", 'only proposed change')
        loop.body[-1] = replacement
        changes += 1
need(changes == 1, 'one candidate expression changed')
ast.fix_missing_locations(fixed)

fixtures = {
    'tail_3_percent_ci_below_margin': Samples([-0.021] * 300 + [-0.019] * 9700),
    'tail_3_percent_ci_at_margin': Samples([-0.020] * 300 + [-0.019] * 9700),
    'ci_above_margin': Samples([-0.019] * 10000),
    'holm_rejects': Samples([-0.021] * 600 + [-0.019] * 9400),
    'missing_data': Samples(),
}
reports = {}
for name, samples in fixtures.items():
    env = {'np': NP}
    def lidar_pairs(results, line, ref, k, mcs, field):
        need(results == [] and ref == 'R0' and mcs == 20, 'injected pairs interface')
        return [] if k == 10 and field == 'iou' and not samples else [(1, 0.0)]
    def cluster_bootstrap(pairs, seed):
        if seed.endswith('|L2|10'):
            return (sum(samples) / len(samples) if samples else float('nan'), samples)
        return (-0.1, Samples([-0.1] * 10000)) if '|L1|' in seed else (0.001, Samples([0.001] * 10000))
    env.update(lidar_pairs=lidar_pairs, cluster_bootstrap=cluster_bootstrap)
    exec(compile(helper, str(SOURCE), 'exec'), env)
    old = env['lidar_decision']([], 'injected-not-resampled')
    exec(compile(fixed, '<single-expression-candidate>', 'exec'), env)
    candidate = env['lidar_decision']([], 'injected-not-resampled')
    bad = old['H_L2_k10']
    ci_ok = bad['ci95'] is not None and bad['ci95'][0] > -0.02
    expected = bad['p_holm'] < 0.05 and ci_ok
    need(candidate['H_L2_k10']['claimed'] == expected, 'candidate follows prereg CI and Holm')
    need(candidate['H_L2_k10']['p_holm'] == bad['p_holm'], 'unchanged p-value statistic')
    for key in ('H_L1_k5', 'H_L1_k10', 'H_L2_k5'):
        need(old[key] == candidate[key], 'other three primary rows unchanged')
        need(old[key]['claimed'], 'positive-control primary remains claimed')
    report = {'source_claimed': bad['claimed'], 'candidate_claimed': candidate['H_L2_k10']['claimed'],
              'ci95': bad['ci95'], 'preregistered_ci_condition': ci_ok,
              'p_noninferiority': bad['p_noninferiority'], 'p_holm': bad['p_holm'],
              'samples_count': len(samples), 'tail_at_or_below_margin': sum(samples <= -0.02)}
    if name.startswith('tail_3_percent'):
        need(bad['claimed'] and not ci_ok and not candidate['H_L2_k10']['claimed'], 'causal false claim reproduced')
    elif name == 'ci_above_margin':
        need(bad['claimed'] and ci_ok and candidate['H_L2_k10']['claimed'], 'true positive preserved')
    else:
        need(not bad['claimed'] and not candidate['H_L2_k10']['claimed'], 'negative control preserved')
    reports[name] = report
out = {'scope': 'frozen decision functions with injected distributions; no bootstrap or fit',
       'pin': 'c22be4e41c2f40d150c6ef06154ab420e3451022',
       'source_sha256': hashlib.sha256(raw).hexdigest(), 'checks': checks, 'fixtures': reports,
       'candidate': ast.unparse(replacement), 'observed_source_defect': True,
       'real_P08_results_judged': False}
print(json.dumps(out, indent=2, sort_keys=True, allow_nan=False))
