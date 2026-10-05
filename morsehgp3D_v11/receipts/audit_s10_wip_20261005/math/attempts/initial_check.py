#!/usr/bin/env python3
"""Exact integer witness for the abstract S10 API; no native execution."""
from fractions import Fraction
from hashlib import sha256
import json
from math import isqrt
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
manifest = json.loads((HERE / 'SOURCE.json').read_text())

def need(ok, message):
    if not ok:
        raise RuntimeError(message)

sources = {}
for path, expected in manifest['files'].items():
    blob = (HERE / 'source' / path).read_bytes()
    need(sha256(blob).hexdigest() == expected, 'source SHA256: ' + path)
    sources[path.rsplit('/', 1)[-1]] = blob.decode()
need('root(u32 rank, u128& out)' in sources['head.hpp'], 'public u128 root interface')
need('hors de u128' in sources['head.hpp'], 'declared root bound')
need('e_hi = static_cast<i128>(rt) + 1;' in sources['score.cpp'], 'causal signed addition')
need('detail::bracket_plateaus(tree, levels, params.z, budget, brackets)' in sources['select.cpp'],
     'abstract flat_sites reaches bracket_plateaus')

# Keep catalogue rank 0 at zero; the sole point-tree plateau uses positive rank 1.
R = (1 << 127) - 1
radius = Fraction(R, 1 << 64)
level = radius * radius
root = isqrt((level.numerator << 128) // level.denominator)
need(root == R, 'exact root is floor(2^64 sqrt(level))')
need(Fraction(root * root, 1 << 128) == level, 'root identity exact')
need(0 <= R < 1 << 128, 'root fits declared u128 domain')
i128_min, i128_max = -(1 << 127), (1 << 127) - 1
need(i128_min <= R <= i128_max, 'conversion itself fits i128')
upper = R + 1
need(upper > i128_max, 'source upper endpoint overflows signed i128')

# Four distinct IDs; two root blocks, each born with two engaged sites.
# This is the forest shape of F14e, at a single positive plateau.
tree = dict(plateau_t=[1], plateau_m=[0], plateau_q=[0],
            block_plateau=[0, 0], block_parent=[(1 << 32)-1, (1 << 32)-1],
            site_block=[0, 0, 1, 1], site_plateau=[0, 0, 0, 0], site_ids=[0, 1, 2, 3])
need(len(tree['site_ids']) == 4 and len(set(tree['site_ids'])) == 4, 'distinct four sites')
need(all(tree['site_block'].count(b) == 2 for b in range(2)), 'two large root blocks for mcs2')
need(all(p == 0 for p in tree['site_plateau'] + tree['block_plateau']), 'one atomic positive plateau')
need(tree['plateau_m'] == tree['plateau_q'], 'simple radius branch')
need(0 < level and 1 <= 1 <= 3, 'positive level and valid exponent')

print(json.dumps(dict(pin=manifest['pin'], native_executed=False,
    scope='integer boundary model and public abstract API shape; no execution of flat_sites',
    mcs=2, z=1, tree_view=tree,
    levels={'0': '0', '1': str(level)},
    root=str(R), u128_fit=True, i128_conversion_fit=True,
    mathematical_e_hi=str(upper), i128_max=str(i128_max),
    e_hi_i128_fit=False, source_signed_addition_overflows=True,
    catalogue_u24_failure_claimed=False), sort_keys=True, separators=(',', ':')))
