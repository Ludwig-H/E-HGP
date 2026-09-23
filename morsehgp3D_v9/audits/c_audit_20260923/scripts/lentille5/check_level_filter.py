# Exact check of the certified double filter of level order (level.hpp:
# level_approximation, kLevelFilterMargin). Python floats are IEEE binary64
# with round-to-nearest-even, like the C++ path under FE_TONEAREST without
# contraction. Exact values via fractions. No assert (valid under -O).
import random, sys
from fractions import Fraction
TWO64 = 18446744073709551616.0
def approx(num, den):
    w0, w1, w2 = num & (2**64-1), (num >> 64) & (2**64-1), num >> 128
    n = (float(w2) * TWO64 + float(w1)) * TWO64 + float(w0)
    d = float(den >> 64) * TWO64 + float(den & (2**64-1))
    return n / d
MARGIN = 1.0 - 2.0**-46
rng = random.Random(20260923)
worst = Fraction(0)
cases = 0
def rel(num, den):
    a = Fraction(approx(num, den)); e = Fraction(num, den)
    return abs(a - e) / e
def gen():
    kind = rng.randrange(6)
    if kind == 0:   # q4-like: num < 2^160, den = det^2 < 2^120
        det = rng.randrange(1, 2**60); return rng.randrange(1, 2**160), det*det
    if kind == 1:   # q3-like reduced: num < 2^113, den < 2^79
        return rng.randrange(1, 2**113), rng.randrange(1, 2**79)
    if kind == 2:   # adversarial words all ones
        b = rng.randrange(1, 192); num = 2**b - 1
        return num, rng.choice([1, 2**64-1, 2**64+1, 2**120-1, 2**119+1])
    if kind == 3:   # q2-like: D2/4
        return rng.randrange(1, 3*2**36), 4
    if kind == 4:   # near-halfway patterns in low words
        num = (rng.randrange(1, 2**64) << 128) | (2**63) << 64 | 2**63
        return num, (rng.randrange(1, 2**56) << 64) | (2**63)
    return rng.randrange(1, 2**192), rng.randrange(1, 2**127)
for _ in range(200000):
    num, den = gen()
    r = rel(num, den); cases += 1
    if r > worst: worst = r
bound = Fraction(1, 2**49)
print("cases", cases, "max_rel_err_log2", float(worst) and __import__('math').log2(float(worst)))
print("within_2^-49", worst < bound)
# Filter soundness on near-equal pairs: whenever approx says x<y*MARGIN, exact must agree.
bad = 0; decided = 0
for _ in range(200000):
    n1, d1 = gen()
    # perturb to create near ties
    k = rng.choice([0, 1, -1, 2**20, -2**20])
    n2, d2 = n1 * 3 + k, d1 * 3
    if n2 <= 0: continue
    x, y = approx(n1, d1), approx(n2, d2)
    for (a, b, ea, eb) in ((x, y, Fraction(n1, d1), Fraction(n2, d2)), (y, x, Fraction(n2, d2), Fraction(n1, d1))):
        if a < b * MARGIN:
            decided += 1
            if not (ea < eb): bad += 1
print("near_tie_decisions", decided, "unsound", bad)
sys.exit(0 if (worst < bound and bad == 0) else 1)
