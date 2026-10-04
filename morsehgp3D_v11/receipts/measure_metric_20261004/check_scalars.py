#!/usr/bin/env python3
"""Independent, bounded Fraction checks. No product/native/oracle imports or fit."""
from fractions import Fraction as F
import json


class Guards:
    def __init__(self):
        self.count = 0

    def check(self, condition, reason):
        self.count += 1
        if not condition:
            raise RuntimeError(reason)


def as_text(x):
    return str(x)


def cloud(n, radius):
    return [F(0), 2 * radius, 4 * radius] + [6 * radius + j * radius / n for j in range(n - 3)]


def norm2(c, site):
    return (c[0] - site) ** 2 + c[1] ** 2 + c[2] ** 2


def check_lens(g, sites):
    """Check the exact squared-norm convexity identity used by the R3 proof on tiny rational samples."""
    s1 = sites[1]
    for sj in sites[1:]:
        t = s1 / sj
        g.check(0 < t <= 1, 'interpolation coefficient')
        for c in ((F(0), F(0), F(0)), (sj / 2, sj / 3, -sj / 5),
                  (sj / 2, F(0), F(0)), (sj, -sj / 7, sj / 11)):
            left = norm2(c, s1)
            right = (1 - t) * norm2(c, 0) + t * norm2(c, sj) - t * (1 - t) * sj * sj
            g.check(left == right, 'R3 convexity identity')
            # Any radius containing both endpoint sites also contains s1.
            bound2 = max(norm2(c, 0), norm2(c, sj))
            g.check(left <= bound2, 'endpoint-ball implication')


def check_threshold(g, sites):
    s1, s2 = sites[1:3]
    g.check(0 < s1 < s2, 'first two positive sites')
    # Before s2/2 no pair lens containing 0 and a site >=s2 is nonempty.
    before = s2 / 2 - s2 / 64
    g.check(2 * before < s2, 'strict pre-qualification separation')
    for sj in sites[2:]:
        g.check(sj > 2 * before, 'all later pair lenses separated from B0')
    # At s2/2 the midpoint encloses 0,s1,s2, with the two extremes on the closed shell.
    center, birth = s2 / 2, s2 / 2
    g.check(all((site - center) ** 2 <= birth ** 2 for site in sites[:3]), 'closed triple witness')
    g.check(center ** 2 == birth ** 2 and (s2 - center) ** 2 == birth ** 2, 'closed shell endpoints')
    return birth


def main():
    g = Guards()
    examples = []
    for n in (4, 5, 6, 16, 64):
        for radius in (F(1), F(3, 2)):
            x = cloud(n, radius)
            g.check(len(x) == n and len(set(x)) == n, 'X cardinality/distinctness')
            g.check(x == sorted(x) and x[-1] == 7 * radius - 4 * radius / n, 'last background coordinate')
            for eta in (radius / 3, radius, 3 * radius / 2):
                y = sorted(x + [eta])
                g.check(len(y) == n + 1 and len(set(y)) == n + 1, 'Y cardinality/distinctness')
                g.check(y[:3] == [0, eta, 2 * radius], 'Y nearest two sites')
                tx, ty = check_threshold(g, x), check_threshold(g, y)
                g.check(tx == 2 * radius and ty == radius and tx - ty == radius, 'entry jump under A5')
                g.check(all(0 <= site < 7 * radius for site in y), 'common compact support')
                # Probability measures are used ONLY for the input metric, not the unit-count rule.
                diagonal = F(1, n + 1)
                excess = F(1, n * (n + 1))
                g.check(diagonal + excess == F(1, n), 'coupling old-source marginal')
                g.check(n * excess == F(1, n + 1), 'coupling new-target marginal')
                g.check(n * diagonal + n * excess == 1, 'total coupling mass')
                costs = {}
                for p in (1, 2, 4, 8):
                    cost = sum(excess * abs(site - eta) ** p for site in x)
                    bound = (7 * radius) ** p / (n + 1)
                    g.check(cost <= bound, 'Wp pth-power coupling upper bound')
                    g.check(radius ** p / bound == F(n + 1, 7 ** p), 'pth-power ratio lower bound')
                    costs[str(p)] = dict(actual_coupling_cost=as_text(cost), bound=as_text(bound))
                if n <= 6:
                    check_lens(g, x)
                    check_lens(g, y)
                if radius == 1 and eta == radius / 3:
                    examples.append(dict(n=n, radius=str(radius), eta=str(eta),
                                         entry_x=str(tx), entry_y=str(ty), costs=costs))
    # Scalar grid bounds only: do NOT instantiate 299k sites or invoke a native product.
    maximum = (1 << 21) - 1
    nmax = (maximum + 4) // 7
    g.check(nmax == 299593, 'max n for the explicit u21 family')
    g.check(7 * nmax - 4 <= maximum < 7 * (nmax + 1) - 4, 'sharp coordinate-domain scalar bound')
    for n in (4, 5, 6, 299000, nmax):
        g.check(0 < 1 < 2 * n < 4 * n < 6 * n <= 7 * n - 4 <= maximum,
                'u21 integer placement R=n eta=1')
        g.check((7 * n - 4) - 6 * n + 1 == n - 3, 'integer background cardinality')
    print(json.dumps(dict(status='PASS', guards=g.count, examples=examples,
                          scalar_u21=dict(bits=21, radius='N', eta=1, max_sites_x=nmax,
                                          max_sites_y=nmax + 1, last_coordinate=7 * nmax - 4,
                                          allocates_sites=False),
                          scope='independent Fraction identities and bounds; no native FULL, fit or GCP'),
                     sort_keys=True, indent=1))


if __name__ == '__main__':
    main()
