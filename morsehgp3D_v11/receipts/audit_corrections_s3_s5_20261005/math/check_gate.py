#!/usr/bin/env python3
"""Bounded portable counter-check of the frozen S3 Fraction judge.

No native execution: synthetic JSON outputs exercise its comparisons. Real oracle
S1 cases are D2/E5, weak triangle, passenger, and the new 12-site witness K1..12.
"""
import contextlib
import copy
import hashlib
import io
import json
import pathlib
import sys

sys.dont_write_bytecode = True
HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / 'snapshot/morsehgp3D_v11/tests/tower'))
import attach_fraction as A  # noqa: E402


def require(test, message):
    if not test:
        raise RuntimeError(message)


fixtures = A.test_supports.FIXTURES
clouds = [(name, fixtures[name][0], (k,)) for name, k in
          [('d2_audit', 2), ('triangle_equilateral', 2), ('passagere', 1)]]
clouds.insert(1, ('e5_audit', [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)], (2,)))
clouds.append(A.WITNESS_K10)
preflight = A.mhgp11_gate.Gate('counter_oracle')
cases, compared, excluded = A.oracle_cases(preflight, 21, clouds)
require(preflight.failures == 0 and compared == 5 and excluded == 0 and len(cases) == 16,
        'bounded oracle cases differ')
witness_cases = [c for c in cases if c[0] == A.WITNESS_K10[0]]
witness_counts = A.count(witness_cases, 1, 0)
require((witness_counts['orders'], witness_counts['balls'], witness_counts['nodes'], witness_counts['high']) ==
        (12, 208, 135, 7), 'new CMake coverage delta differs')
require((951 + witness_counts['orders'], 15062 + witness_counts['balls'], 12441 + witness_counts['nodes']) ==
        (963, 15270, 12576), 'engraved u21 coverage differs from the previous suite plus witness')

central = {}
for name, _points, _ids, k, doc in witness_cases:
    if k not in (9, 10, 11, 12):
        continue
    found = [b for b in doc['balls'] if b['center'] == ['10', '10', '10'] and b['level'] == '200']
    require(len(found) == 1, 'central ball not unique')
    b = found[0]
    central[k] = {key: b[key] for key in A.BALL_KEYS}
    expected = {
        9: ('interne', 6, 4, 1, []),
        10: ('fusion', 4, 4, 4, [0, 1, 2, 3]),
        11: ('naissance', 0, 0, 0, []),
        12: ('naissance', 0, 0, 0, []),
    }[k]
    require((b['role'], b['node'], b['strict_traces'], b['components'], b['prior']) == expected,
            'central K%d attachment differs from the new native unit expectations' % k)
    require((b['p'], b['m'], b['qmin']) == (8, 4, 2), 'central shape differs')
    require(len(doc['nodes']) == {9: 7, 10: 5, 11: 1, 12: 1}[k], 'high-order node count differs')
    require(sum(n['kind'] == 1 for n in doc['nodes']) == {9: 6, 10: 4, 11: 1, 12: 1}[k],
            'high-order birth count differs')
    if k in (9, 10):
        require(doc['nodes'][b['node']]['children'] == list(range(6 if k == 9 else 4)),
                'high-order plateau children differ')


def target(name, k, level):
    ci = next(i for i, c in enumerate(cases) if c[0] == name and c[3] == k)
    bi = next(i for i, b in enumerate(cases[ci][4]['balls']) if b['level'] == level)
    return ci, bi


d2 = target('d2_audit', 2, '1681/25')
d2_ball = cases[d2[0]][4]['balls'][d2[1]]
require((d2_ball['node'], d2_ball['prior'], d2_ball['strict_traces']) == (6, [3, 4, 5], 3),
        'D2 closed attachment/prior differs')
e5 = target('e5_audit', 2, '83886/3563')
require(cases[e5[0]][4]['balls'][e5[1]]['prior'] == [5, 6, 8], 'E5 off-window connection differs')
weak = target('triangle_equilateral', 2, '8/3')
passing = target('passagere', 1, '4')
require(cases[passing[0]][4]['balls'][passing[1]]['role'] == 'fusion' and
        cases[passing[0]][4]['balls'][passing[1]]['components'] == 1, 'passenger fixture differs')

documents = []
for _name, _points, _ids, _k, oracle in cases:
    got = A.project(oracle)
    got.update(format='hgp11_attach_probe', version=1)
    for mine, theirs in zip(got['balls'], oracle['balls']):
        mine['s_star'] = next(s for s in theirs['supports'] if len(s) == theirs['qmin'])
    documents.append(got)


def judge(documents, workers):
    saved_run = A.mhgp11_gate.run
    def fake_run(argv, timeout, stdin):
        require(argv == ['SYNTHETIC_JSON_ONLY', '--workers=%d' % workers] and timeout == 600,
                'unexpected process request')
        require(stdin == A.requests(cases, workers > 1), 'request ordering differs')
        output = '\n'.join(json.dumps(d, sort_keys=True) for d in documents) + '\n'
        return A.mhgp11_gate.Completed(0, 0, False, output, '')
    A.mhgp11_gate.run = fake_run
    gate = A.mhgp11_gate.Gate('counter_compare')
    capture = io.StringIO()
    try:
        with contextlib.redirect_stdout(capture):
            A.compare_run(gate, 'SYNTHETIC_JSON_ONLY', workers, cases)
            code = gate.finish(floor=1)
    finally:
        A.mhgp11_gate.run = saved_run
    return {'code': code, 'checks': gate.checks, 'failures': gate.failures,
            'first_failure': next((s for s in capture.getvalue().splitlines() if s.startswith('ECHEC ')), None)}


controls = {str(w): judge(copy.deepcopy(documents), w) for w in (1, 3)}
require(all(x['code'] == 0 and x['failures'] == 0 for x in controls.values()), 'synthetic exact control rejected')
mutations = {}
for name in ('weak_event_omitted', 'D2_open_owner', 'D2_branch_omitted', 'E5_branch_omitted',
             'passenger_labeled_internal', 'K10_compressed_for_strict', 'K12_not_birth', 'postorder_wrong',
             'support_star_invalid'):
    changed = copy.deepcopy(documents)
    if name == 'weak_event_omitted':
        changed[weak[0]]['balls'].pop(weak[1])
    elif name == 'D2_open_owner':
        changed[d2[0]]['balls'][d2[1]]['node'] = 3
    elif name == 'D2_branch_omitted':
        changed[d2[0]]['balls'][d2[1]]['prior'] = [3, 4]
    elif name == 'E5_branch_omitted':
        changed[e5[0]]['balls'][e5[1]]['prior'] = [5, 6]
    elif name == 'passenger_labeled_internal':
        changed[passing[0]]['balls'][passing[1]]['role'] = 'interne'
    elif name == 'K10_compressed_for_strict':
        ci, bi = target(A.WITNESS_K10[0], 10, '200')
        changed[ci]['balls'][bi]['strict_traces'] = 6
    elif name == 'K12_not_birth':
        ci, bi = target(A.WITNESS_K10[0], 12, '200')
        changed[ci]['balls'][bi]['role'] = 'interne'
    elif name == 'postorder_wrong':
        changed[weak[0]]['nodes'][0]['post'] += 1
    elif name == 'support_star_invalid':
        changed[weak[0]]['balls'][weak[1]]['s_star'] = [[99, 99, 99]] * 3
    mutations[name] = judge(changed, 3)
    require(mutations[name]['code'] == 1 and mutations[name]['failures'] > 0, name + ' was not rejected')

result = {
    'scope': 'new Fraction comparator and exact bounded oracle expectations; synthetic probe output only',
    'phase': 'exploration_v11_hors_registre', 'public_status': 'not_claimed',
    'native_executed': False, 'FULL_verticals_tested': False, 'whole_963_order_gate_run': False,
    'clouds': compared, 'orders': len(cases), 'witness_K1_to_K12_counts': witness_counts,
    'central_K9_to_K12': central, 'D2': A.project(cases[d2[0]][4])['balls'][d2[1]],
    'control_W1_W3': controls, 'synthetic_output_mutations': mutations,
    'canonical_cases_sha256': hashlib.sha256(json.dumps([c[4] for c in cases], sort_keys=True,
                                                       separators=(',', ':')).encode()).hexdigest(),
}
print(json.dumps(result, sort_keys=True, separators=(',', ':')))
