#!/usr/bin/env python3
"""Portable bounded check of the norm-5 K12 witness, using frozen WIP S6a Python primitives.

This checks one ball and derived counts. It does not build a K12 tree, run native code,
or qualify the entire S1 oracle at 24 sites. No assert: identical under python -O.
"""
import hashlib
import itertools
import json
import os
import pathlib
import sys
from math import comb

HERE = pathlib.Path(__file__).resolve().parent
REFERENCE = pathlib.Path(os.environ.get('MHGP11_L1_REFERENCE', HERE / 'snapshot/morsehgp3D_v11/reference'))
sys.path.insert(0, str(REFERENCE))
import test_supports  # noqa: E402 -- its private loader avoids the package/B layer

S = test_supports.STAGE['supports']
points = test_supports.SPHERE_NORM5
oracle = S.Supports(points)
ball = oracle.ball_of((points.index((0, 1, 2)), points.index((4, 3, 2))))


def require(test, message):
    if not test:
        raise RuntimeError(message)


require((ball.p, ball.m, ball.q, ball.level, ball.center) ==
        (0, 24, 2, 5, (2, 2, 2)), 'unexpected central ball')
arities = [sum(len(q) == a for q in ball.supports) for a in (2, 3, 4)]
require(arities == [12, 24, 792], 'Q_b arities differ')

# Every 12-part containing both members of one antipodal pair contains a Q2
# support and has MEB radius exactly 5. A strict 12-part must therefore choose
# precisely one member of each of the 12 pairs. Check all 4096 such choices
# against *all* remaining Q3/Q4 supports, not only minimal arity.
pairs = [q for q in ball.supports if len(q) == 2]
require(sorted(i for pair in pairs for i in pair) == list(range(24)), 'pairs do not partition the shell')
require(all(tuple(points[i][d] + points[j][d] for d in range(3)) == (4, 4, 4)
            for i, j in pairs), 'pair is not antipodal')
support_masks = [sum(1 << i for i in q) for q in ball.supports if len(q) >= 3]
strict_masks = []
for picks in itertools.product((0, 1), repeat=12):
    mask = sum(1 << pair[pick] for pair, pick in zip(pairs, picks))
    if not any(mask & support == support for support in support_masks):
        strict_masks.append(mask)
require(len(strict_masks) == 116, 'strict K12 count differs')
n12 = comb(24, 12) - len(strict_masks)
require(n12 == 2704040, 'N12 differs')
# For j >= 13, every j-part contains an antipodal pair by pigeonhole, so N_j=C(24,j).
n13 = comb(24, 13)
incidences = sum(comb(24 - len(q), 13 - len(q)) for q in ball.supports)
require((n13, incidences) == (2496144, 149954688), 'K12 cofaces/incidences differ')

# The new budget must refuse before any exact MEB in the bounded lemma-F routine.
# This only proves that primitive's preflight, not a global refusal before all S1 stages.
meb_calls = [0]
def trap_meb(*_args, **_kwargs):
    meb_calls[0] += 1
    raise RuntimeError('MEB reached after expected budget refusal')
oracle.definition.meb = trap_meb
refusals = {}
for name in ('_minimal_nonseparable', '_lemma_f'):
    try:
        getattr(oracle, name)(ball)
    except S.BudgetRefusal as exc:
        refusals[name] = {'type': type(exc).__name__, 'message': str(exc)}
    else:
        raise RuntimeError(name + ' unexpectedly accepted a 24-site shell')
require(meb_calls[0] == 0 and len(refusals) == 2, 'budget preflight differs')

result = {
    'phase': 'exploration_v11_hors_registre', 'public_status': 'not_claimed',
    'source': 'frozen WIP S6a Python primitives; source_manifest.json',
    'native_executed': False, 'whole_S1_24_qualified': False, 'K12_tree_built': False,
    'sites': 24, 'support_arities': arities, 'support_count': len(ball.supports),
    'one_per_antipodal_pair_candidates': 4096, 'strict_traces_K12': len(strict_masks),
    'N12': n12, 'cofaces_K12': n13, 'support_coface_incidences_K12': incidences,
    'strict_masks_sha256': hashlib.sha256(json.dumps(sorted(strict_masks), separators=(',', ':')).encode()).hexdigest(),
    'budget_refusals': refusals, 'meb_calls_during_budget_checks': meb_calls[0],
}
print(json.dumps(result, sort_keys=True, separators=(',', ':')))
