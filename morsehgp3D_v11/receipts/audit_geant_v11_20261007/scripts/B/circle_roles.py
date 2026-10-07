import sys
sys.path.insert(0, '/workspaces/E-HGP/morsehgp3D_v11/reference')
from hgp11_ref.supports import Supports
def circle(n, perturbed):
    d = n * n + 1
    b = ((n + 1) ** 2, 2 * n * n, 0) if perturbed else (d, 2 * d, 0)
    return [(2 * d, d, 0), b, (0, d, 0), (d, 0, 0)]
for pert in (False, True):
    out = Supports(circle(3, pert)).canonical(2)
    for b in out['balls'] if isinstance(out, dict) and 'balls' in out else []:
        print('perturbe' if pert else 'cercle  ', {k: b[k] for k in b if k in ('level', 'role', 'supports', 'components', 'center')})
