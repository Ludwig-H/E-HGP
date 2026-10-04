#!/usr/bin/env python3
"""Replays only frozen decision AST, with injected distributions and stdlib fakes.

No NumPy import, resampling, product import, fit, native execution or cloud access.
Gate checks are AST/source checks, not execution of its geometric oracle.
"""
import ast
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'source/morsehgp3D_v11'
PIN = '723cf6e436f6cb6f524501cbd9a51bf0e17fe6fe'
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


claims_path = SOURCE / 'bench/points_flat_claims.py'
claims_ast = ast.parse(claims_path.read_bytes())
body = [n for n in claims_ast.body if
        isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id in ('ALPHA', 'MARGIN') for t in n.targets)
        or isinstance(n, ast.FunctionDef) and n.name in ('holm', 'h_l1_claim', 'h_l2_claim')]
need(len(body) == 5, 'exact constants and three source claim functions')
claim_helper = ast.Module(body=body, type_ignores=[])
summary_path = SOURCE / 'bench/points_flat_summary.py'
summary_ast = ast.parse(summary_path.read_bytes())
decisions = [n for n in summary_ast.body if isinstance(n, ast.FunctionDef) and n.name == 'lidar_decision']
need(len(decisions) == 1, 'one exact source decision function')
need(any(isinstance(n, ast.ImportFrom) and n.module == 'points_flat_claims' and
         {a.name for a in n.names} == {'holm', 'h_l1_claim', 'h_l2_claim'} for n in summary_ast.body),
     'real summary binds the tested three claim functions')
decision_helper = ast.Module(body=decisions, type_ignores=[])

fixtures = {
    'tail_3_percent_ci_below_margin': (Samples([-0.021] * 300 + [-0.019] * 9700), False),
    'tail_3_percent_ci_at_margin': (Samples([-0.020] * 300 + [-0.019] * 9700), False),
    'ci_above_margin': (Samples([-0.019] * 10000), True),
    'holm_rejects': (Samples([-0.021] * 600 + [-0.019] * 9400), False),
    'missing_data': (Samples(), False),
}
reports = {}
for name, (samples, expected_claim) in fixtures.items():
    env = {'np': NP}

    def lidar_pairs(results, line, ref, k, mcs, field):
        need(results == [] and ref == 'R0' and mcs == 20, 'injected pairs interface')
        return [] if k == 10 and field == 'iou' and not samples else [(1, 0.0)]

    def cluster_bootstrap(pairs, seed):
        if seed.endswith('|L2|10'):
            return (sum(samples) / len(samples) if samples else float('nan'), samples)
        return (-0.1, Samples([-0.1] * 10000)) if '|L1|' in seed else (0.001, Samples([0.001] * 10000))

    env.update(lidar_pairs=lidar_pairs, cluster_bootstrap=cluster_bootstrap)
    exec(compile(claim_helper, str(claims_path), 'exec'), env)
    exec(compile(decision_helper, str(summary_path), 'exec'), env)
    rows = env['lidar_decision']([], 'injected-not-resampled')
    need(set(rows) == {'H_L1_k5', 'H_L2_k5', 'H_L1_k10', 'H_L2_k10'}, 'four primary rows')
    bad = rows['H_L2_k10']
    ci_ok = bad['ci95'] is not None and bad['ci95'][0] > -0.02
    need(bad['claimed'] == expected_claim, 'expected new H_L2 claim')
    need(bad['claimed'] == (bad['p_holm'] < 0.05 and ci_ok), 'both exact prereg criteria')
    need(bad['p_holm'] >= bad['p_noninferiority'], 'monotone p adjustment')
    for key in ('H_L1_k5', 'H_L1_k10', 'H_L2_k5'):
        need(rows[key]['claimed'], 'other primary positive controls retained')
        need(rows[key]['p_holm'] < 0.05, 'other primary Holm condition retained')
    if name.startswith('tail_3_percent'):
        need(bad['p_holm'] < 0.05 and not ci_ok and not bad['claimed'], 'original false-claim cases now refused')
    report = {k: bad[k] for k in ('claimed', 'ci95', 'p_noninferiority', 'p_holm')}
    report['samples_count'] = len(samples)
    report['preregistered_ci_condition'] = ci_ok
    reports[name] = report

gate_path = SOURCE / 'bench/points_flat_gate.py'
gate_ast = ast.parse(gate_path.read_bytes())
assigns = {t.id: n.value for n in gate_ast.body if isinstance(n, ast.Assign)
           for t in n.targets if isinstance(t, ast.Name)}
lines = ast.literal_eval(assigns['LINES'])
need(lines == (('eom', 1), ('eom', 2), ('eom', 3), ('leaf', 1)), 'z2 present among oracle comparison lines')
need(ast.literal_eval(assigns['MCS']) == (2, 3, 4), 'small oracle mcs scope')
compare = next(n for n in gate_ast.body if isinstance(n, ast.FunctionDef) and n.name == 'compare_cloud')
need(any(isinstance(n, ast.For) and ast.unparse(n.target) == '(method, z)' and
         isinstance(n.iter, ast.Name) and n.iter.id == 'LINES' for n in ast.walk(compare)), 'actual compare loop uses LINES')
oracle_calls = [n for n in ast.walk(compare) if isinstance(n, ast.Call) and isinstance(n.func, ast.Name)
                and n.func.id == 'oracle_labels']
need(len(oracle_calls) == 1 and [ast.unparse(a) for a in oracle_calls[0].args] ==
     ['side', 'points', 'k', 'mcs', 'z', 'method'], 'same z passed to independent oracle adapter')
facts = {}
for n in ast.walk(gate_ast):
    if isinstance(n, ast.Call) and isinstance(n.func, ast.Name) and n.func.id == 'fact' and n.args \
            and isinstance(n.args[0], ast.Constant) and n.args[0].value in ('F4_z2', 'F4b_z1', 'F4b_z2'):
        flat = n.args[1]
        need(isinstance(flat, ast.Call) and isinstance(flat.func, ast.Name) and flat.func.id == 'flat', 'fixture selects real flat adapter')
        expected = ast.literal_eval(assigns[n.args[2].id]) if isinstance(n.args[2], ast.Name) else ast.literal_eval(n.args[2])
        facts[n.args[0].value] = {'flat_args': [ast.unparse(a) for a in flat.args], 'expected_groups': expected}
need(set(facts) == {'F4_z2', 'F4b_z1', 'F4b_z2'}, 'all three z2-related fixtures present')
for name, z in (('F4b_z1', 1), ('F4b_z2', 2)):
    args = facts[name]['flat_args']
    need(args == ['NINE_B', "'f4b'", '2', '3', str(z), "'eom'"], 'same nine-site fixture and k/mcs, z varies')
    groups = facts[name]['expected_groups']
    need(sorted(v for group in groups for v in group) == list(range(9)), 'expected partition covers each site once')
need(facts['F4b_z1']['expected_groups'] != facts['F4b_z2']['expected_groups'], 'z2 expectation distinguishes z1')
cmake = (SOURCE / 'tests/tower/tests.cmake').read_text()
need('mhgp11_python_gate(mhgp11_tower_points_flat_claims 0' in cmake and
     'LINE "points_flat_claims_verdict conforme checks10" LABELS fast TIMEOUT 30)' in cmake,
     'stdlib helper selftest registered with exact expected line')
doc = (SOURCE / 'docs/SORTIE_PLATE.md').read_text()
need('à z = 16' in doc and 'toute EOM, à tout z' in doc, 'all-z overclaim withdrawn with counterexample scope')
need('n\'est pas établie ; il ne prouve pas sa nullité' in doc, 'non-significance no longer claimed as null contribution')
need('À rejouer sur G4 avant les mesures' in doc and 'aucune tête C++ n\'existe encore' in doc,
     'source fix distinguished from G4 and native head')
campaign_ast = ast.parse((SOURCE / 'bench/points_flat_campaign.py').read_bytes())
oracle_m05 = next(n for n in campaign_ast.body if isinstance(n, ast.FunctionDef) and n.name == 'oracle_m05')
need([a.arg for a in oracle_m05.args.args] == ['cond', 'first', 'ids', 'truth'], 'campaign oracle takes truth and already-built condensed tree')
need(not any(isinstance(n, ast.Import) and any(a.name == 'points_flat_oracle' for a in n.names)
             or isinstance(n, ast.ImportFrom) and n.module == 'points_flat_oracle' for n in campaign_ast.body),
     'campaign does not import correction oracle module')
out = {'pin': PIN, 'checks': checks, 'verdict': 'conforme',
       'scope': 'exact frozen decision AST with injected distributions; gate wiring and fixture expectations are source checks',
       'native_or_fit_executed': False, 'z2_geometric_expected_groups_reproved': False,
       'real_P08_results_judged': False, 'fixtures': reports, 'z2_source_fixtures': facts,
       'sources_sha256': {str(p.relative_to(HERE)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sorted(SOURCE.rglob('*')) if p.is_file()},
       'documentation_clarification': 'campaign oracle_m05 is a truth-dependent optimum on the condensed tree; it is distinct from points_flat_oracle correction comparisons'}
print(json.dumps(out, indent=2, sort_keys=True, allow_nan=False))
