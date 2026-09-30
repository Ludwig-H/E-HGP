"""Exact closed-form countercheck under an excluded root, not a geometric test."""
from fractions import Fraction as F
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('base_exit_oracle', ROOT/'check.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def judge(path, factor=1):
    base.require(factor in (1, 3), 'fixture factor')
    target_mcs = 2 if factor == 1 else 5
    expected_case = 'under_root' if factor == 1 else 'under_root_tripled'
    rows = [base.load(line) for line in path.read_text().splitlines()]
    base.require(len(rows) == 4, 'under-root row floor')
    seen, mismatches, flips, observations = set(), 0, 0, []
    for row in rows:
        mcs, z = row['mcs'], row['z']
        key = mcs, z
        base.require(type(mcs) is int and mcs in (1, target_mcs) and type(z) is int and z in (1, 2)
                     and key not in seen and row['case'] == expected_case
                     and row['allow_single'] is False and row['api_valid'] is True, 'case scope')
        seen.add(key)
        L = lambda beta: base.lam(beta, z)
        birth = (F(0), L(1600), L(1600), L(25), L(25))
        masses = tuple(factor*n for n in (7, 5, 2, 3, 2))
        parents = (None, 0, 0, 1, 1)
        # Root -> R(5),C(2); R -> A(3),B(2). Branch A is the only dynamic collapse.
        a_current = L(1)+L(4)+L(9)-3*L(25)
        collapse = mcs == target_mcs
        # At beta4 the triple cohort drops 6->3; mcs5 is the same threshold event as 2->1/mcs2.
        base.require(not collapse or factor < mcs <= 2*factor, 'minimum-size threshold event')
        a_truth = (2*L(4)+L(9)-3*L(25)) if collapse else a_current
        native_expected = tuple(factor*s for s in
            (7*L(1600), 5*(L(25)-L(1600)), 2*(L(100)-L(1600)),
             a_current, 2*(L(16)-L(25))))
        truth = (*native_expected[:3], factor*a_truth, native_expected[4])
        base.require(len(row['clusters']) == 5 and len(row['labels']) == 7*factor and
                     len(row['point_cluster']) == len(row['point_lambda']) == 7*factor, 'output shape')
        for c, actual in enumerate(row['clusters']):
            base.require(actual['id'] == c and actual['parent'] == parents[c] and actual['mass'] == masses[c]
                         and base.near(actual['birth'], birth[c]) and
                         base.near(actual['stability'], native_expected[c]), 'native body unexpected')
        current_selected = [2, 3, 4]  # the excluded global root cannot be chosen
        truth_selected = [1, 2] if collapse and z == 1 else current_selected
        base.require(row['selected'] == current_selected, 'unexpected native selection')
        repeat = lambda items: [value for value in items for _ in range(factor)]
        current_labels, truth_labels = repeat([1, 1, 1, 2, 2, 0, 0]), repeat([0, 0, 0, 0, 0, 1, 1])
        base.require(row['labels'] == current_labels, 'unexpected native labels')
        current_exit = (L(1), L(4), L(9), L(16), L(16), L(100), L(100))
        exact_exit = (L(4) if collapse else L(1), *current_exit[1:])
        current_exit, exact_exit = repeat(current_exit), repeat(exact_exit)
        base.require(all(base.near(a,b) for a,b in zip(row['point_lambda'],current_exit)),
                     'unexpected native exits')
        different = collapse
        flip = truth_selected != current_selected
        mismatches += different
        flips += flip
        observations.append(dict(mcs=mcs,z=z,differs=different,EOM_flip=flip,
            native_stabilities=list(map(str,native_expected)),exact_stabilities=list(map(str,truth)),
            native_selected=current_selected,exact_selected=truth_selected,
            native_labels=current_labels,exact_labels=truth_labels if flip else current_labels,
            native_point_lambda=list(map(str,current_exit)),exact_point_lambda=list(map(str,exact_exit))))
    base.require(seen == {(m,z) for m in (1,target_mcs) for z in (1,2)} and mismatches == 2 and flips == 1,
                 'under-root nonvacuity')
    return dict(status='EXPECTED_API_DIFFERENCE_CONFIRMED', rows=4, factor=factor, points=7*factor,
                exact_positive_controls=2,
                differing_rows=2,EOM_flips=1,comparisons=observations,native_executions=0,
                scope='fixed API tree under excluded root; no MEB-realizability claim')


if __name__ == '__main__':
    base.require(len(sys.argv) in (2, 3), 'usage: check_under_root.py native.stdout [factor]')
    print(json.dumps(judge(Path(sys.argv[1]), 1 if len(sys.argv) == 2 else int(sys.argv[2])), sort_keys=True, indent=2))
