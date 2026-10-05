#!/usr/bin/env python3
"""Bounded arithmetic check of the S10 WIP guard; no native execution."""
from hashlib import sha256
from itertools import product
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
m = json.loads((HERE / 'SOURCE.json').read_text())
sources = {}
def need(ok, message):
    if not ok:
        raise RuntimeError(message)
for path, expected in m['files'].items():
    blob = (HERE / 'snapshot' / path).read_bytes()
    need(sha256(blob).hexdigest() == expected, 'source SHA256: ' + path)
    sources[path.rsplit('/', 1)[-1]] = blob.decode()

bits = int(re.search(r'kMaxRootBits = (\d+);', sources['internal.hpp']).group(1))
need('kMaxRoot = u128{1} << kMaxRootBits;' in sources['internal.hpp'], 'root limit declaration')
score = sources['score.cpp']
guard = 'if (rt > kMaxRoot || (!sq && (cache.at(tree.plateau_m[p]) > kMaxRoot || cache.at(tree.plateau_q[p]) > kMaxRoot)))'
need(guard in score, 'all used roots checked before conversion')
need(score.index(guard) < score.index('e_lo = static_cast<i128>(rt);'), 'guard before square conversion')
need(score.index(guard) < score.index('const i128 sum ='), 'guard before date arithmetic')
body = score[score.index(guard):score.index('if (sq) {', score.index(guard))]
need('out.open[p] = 1;' in body and 'continue;' in body, 'oversized root bypasses conversion')

limit = 1 << bits
low, high = -(1 << 127), (1 << 127) - 1
fits = lambda x: low <= x <= high
need(fits(limit) and fits(limit + 1), 'square endpoints admitted at limit')
corners = []
for t, big_m, q in product((0, limit), repeat=3):
    s = t + big_m - q
    endpoints = (s - 1, s + 2)
    need(fits(t + big_m) and fits(s), 'all signed intermediate sums admitted')
    need(all(fits(e) for e in endpoints), 'three-root endpoint margins admitted')
    corners.append(endpoints)
need(min(a for a, b in corners) == -limit - 1 and max(b for a, b in corners) == 2*limit + 2,
     'exact extremal interval')

old_R = (1 << 127) - 1
need(old_R > limit, 'published counterexample now goes open before conversion')
need(limit + 1 > limit, 'first root beyond the guard also goes open')
roots = [1 << 126, 3 << 125, 3 << 126]
def signed128(x):
    return ((x + (1 << 127)) % (1 << 128)) - (1 << 127)
old_open = [signed128(r) < (1 << 40) for r in roots]
need(all(fits(signed128(r) + 1) for r in roots), 'huge test avoids the original overflowing addition')
need(old_open == [False, False, True], 'huge test already had one open plateau under modular conversion')
new_open = [r > limit for r in roots]
need(new_open == [True, True, True], 'guard now opens all three huge plateaus')
need('stats.unbracketed >= 1' in sources['head_test.cpp'], 'huge gate currently permits old open count')
print(json.dumps(dict(pin=m['pin'], scope='WIP guard arithmetic only; native huge gate not executed',
    native_executed=False, root_limit_bits=bits, signed_intermediate_safe=True,
    endpoint_corners=len(corners), minimum_endpoint=str(-limit-1), maximum_endpoint=str(2*limit+2),
    original_R_open_before_cast=True, first_beyond_limit_open=True,
    huge_roots=[str(r) for r in roots], huge_old_modular_conversion_open=old_open,
    huge_new_open=new_open, huge_gate_does_not_prove_original_UB_killed=True),
    sort_keys=True,separators=(',', ':')))
