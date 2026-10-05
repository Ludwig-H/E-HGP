#!/usr/bin/env python3
"""Bounded static call-path replay; no source import, CLI, native or cloud invocation."""
import ast
import hashlib
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parent
manifest = json.loads((ROOT / 'source_manifest.json').read_text())
for name, meta in manifest['files'].items():
    if hashlib.sha256((ROOT / 'sources' / name).read_bytes()).hexdigest() != meta['sha256']:
        raise SystemExit('SHA mismatch: ' + name)
def read(name):
    return (ROOT / 'sources' / 'morsehgp3D_v11' / name).read_text()
mutants = json.loads(read('tests/mutants/cli.json'))
mutant = next(m for m in mutants['mutants'] if m['id'] == 'sp_masque_16379')
compute = read(mutant['fichier'])
if compute.count(mutant['cherche']) != 1 or mutant['remplace'] != '':
    raise SystemExit('mutation is not the one-line deletion analyzed')
changed = compute.replace(mutant['cherche'], mutant['remplace'])
start = compute.index('FullParams order_params()')
end = compute.index('}  // namespace mhgp11::api_detail', start)
if mutant['cherche'] not in compute[start:end] or 'params.concurrent_orders = true;' not in compute[:start]:
    raise SystemExit('FullParams/current order_params shape mismatch')
checks = {
    'supports_route_is_full': 'inline constexpr SupportsRoute kSupportsRoute = SupportsRoute::full_tower;' in read('src/api/internal.hpp'),
    'public_supports_uses_fixed_route': 'return api_detail::compute_supports(session, cloud, wanted->k, api_detail::kSupportsRoute, report);' in compute,
    'full_route_uses_full_params': '? build_order_full(std::move(domain.value()), k, budget, api_detail::full_params(),' in compute,
    'order_route_uses_order_params': ': build_order(std::move(domain.value()), k, budget, api_detail::order_params(),' in compute,
    'default_order_route_is_order_tree': 'api_detail::SupportsRoute route = api_detail::SupportsRoute::order_tree,' in compute,
    'points_use_default_order_route': 'Result<OrderTree> tree = order_tree(session, view, k, report, true);' in compute,
    'plat_uses_default_order_route': 'Result<OrderTree> tree = order_tree(session, view, wanted.k, report, true);' in compute,
    'build_order_refuses_concurrent': 'if (params.concurrent_orders || params.parallel_verticals || params.reuse_regular_verticals)' in read('src/tower/order_tree.cpp'),
    'points_gate_registered': 'mhgp11_python_gate(mhgp11_cli_points 0 cli_points.py' in read('tests/cli/tests.cmake'),
    'points_gate_demands_success': "result.code == 0 and line.get('status') == 'ok' and line.get('output') == output" in read('tests/cli/cli_points.py'),
    'points_gate_has_admitted_floor': "gate.check(cases >= 40, 'cas admis : %d' % cases)" in read('tests/cli/cli_points.py'),
    'construction_includes_cli': 'cli' in mutants['construction'],
}
outputs = []
for call in ast.walk(ast.parse(read('tests/cli/cli_supports_oracle.py'))):
    if isinstance(call, ast.Call) and isinstance(call.func, ast.Attribute) and call.func.attr == 'publish':
        if len(call.args) < 6 or not isinstance(call.args[5], ast.Constant):
            raise SystemExit('non-constant oracle publication kind')
        outputs.append(call.args[5].value)
checks['supports_oracle_only_publishes_full_and_supports'] = set(outputs) == {'full', 'supports'}
if not all(checks.values()):
    raise SystemExit('call-path pattern mismatch: ' + repr(checks))
api = json.loads(read('tests/mutants/api.json'))
route = next(m for m in api['mutants'] if m['id'] == 'voie_supports_order_tree')
facts = json.loads((ROOT / 'log_facts.json').read_text())
survivors = [x for x in facts['cli_failure']['excerpt'] if 'SURVIT' in x]
if len(survivors) != 1 or 'sp_masque_16379' not in survivors[0]:
    raise SystemExit('archived CLI survivor does not match')
out = {
    'pin': manifest['pin'], 'mutation': mutant, 'checks': checks,
    'selected_gate_reaches': sorted(set(outputs)),
    'bounded_route_model': {
        'supports': {'parameters': 'full_params', 'mutation_reached': False},
        'full': {'parameters': 'full_params', 'mutation_reached': False},
        'points': {'parameters': 'order_params', 'mutation_reached': True, 'mutated_outcome': 'parameter_out_of_range'},
        'plat': {'parameters': 'order_params', 'mutation_reached': True, 'mutated_outcome': 'parameter_out_of_range'}},
    'existing_points_gate': 'mhgp11_cli_points',
    'existing_separate_supports_route_mutant': route,
    'archived_survivor': survivors[0],
    'proposed_gate_retarget_executed': False,
    'verdict': 'mutant_equivalent_for_current_supports_gate_but_not_for_points_or_plat',
    'engine_defect_established': False}
print(json.dumps(out, sort_keys=True, indent=2))
