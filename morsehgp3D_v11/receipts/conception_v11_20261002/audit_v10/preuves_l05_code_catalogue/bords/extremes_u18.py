"""Extremes du domaine u18 (fixture F10 de GEN_v2, absente des portes du depot) : coins du cube [0, 262143]^3, points
quasi extremes et triangles de cotes proches de 2^18, contre l'oracle Fraction du depot (reference/hgp10_ref.py)."""
import os, random, sys, tempfile
sys.path.insert(0, '/tmp/v11-audit/l05_code_catalogue/src/morsehgp3D_v10/tests/oracle')
sys.path.insert(0, '/tmp/v11-audit/l05_code_catalogue/src/morsehgp3D_v10/reference')
import test_catalogue_oracle as T
exe = '/tmp/v11-audit/l05_code_catalogue/build-rel/mhgp10_catalogue'
L = 262143
rnd = random.Random(20261002)
corners = [(x, y, z) for x in (0, L) for y in (0, L) for z in (0, L)]
clouds = {
    'coins': corners,
    'coins+centre': corners + [(131071, 131071, 131071), (131072, 131072, 131072)],
    'coins+quasi': corners + [(1, 2, L - 3), (L, 0, 131000), (L - 1, L - 2, 5), (7, L - 1, L - 2), (131071, 0, L)],
    'aleatoire_extreme': sorted({tuple(rnd.choice((0, 1, 2, L - 2, L - 1, L, 131071, 131072)) for _ in range(3)) for _ in range(40)})[:18],
    'grand_triangle': [(0, 0, 0), (L, 0, 1), (1, L, 0), (131000, 131000, L), (L, L, L - 1), (5, 3, L)],
}
fails = checks = 0
with tempfile.TemporaryDirectory() as tmp:
    for name, P in clouds.items():
        for K in (1, 2, 3, 5):
            err = T.check(exe, P, K, tmp)
            checks += 1
            if err:
                fails += 1
                print('ECART', name, 'K=%d' % K, err)
            else:
                print('conforme', name, 'n=%d' % len(P), 'K=%d' % K, 'boules', sum(1 for b in T.R.catalogue(P, K) if b.qmin >= 2))
print('extremes_u18 : controles %d, ecarts %d' % (checks, fails))
