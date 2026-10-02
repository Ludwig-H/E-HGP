"""Temoin analytique collineaire, autonome, sans appel au produit/oracle."""
from fractions import Fraction
import json


def require(ok, text):
    if not ok:
        raise RuntimeError(text)


def pack(q):
    return [q.numerator, q.denominator]


rows = []
for m in (4, 256, 32768):
    before = (0, 2 * m, 4 * m)
    after = (0, 2 * m, 4 * m + 1)
    epsilon = Fraction(1, m)
    left = Fraction(before[1] - before[0], 2 * m)
    right_before = Fraction(before[2] - before[1], 2 * m)
    right_after = Fraction(after[2] - after[1], 2 * m)
    root_before = Fraction(before[2] - before[0], 2 * m)
    root_after = Fraction(after[2] - after[0], 2 * m)
    require(left == right_before == 1, "deux morceaux a la premiere entree X")
    require(left < right_after < root_after, "premiere entree Y unique")
    require(root_before == 2 and root_after - root_before == epsilon / 2,
            "fusion FULL deplacee de epsilon/2")
    require(root_before - left == 1, "date LCA saute de 1")
    require(max(after) < 2**18, "fixture native admissible u18")
    rows.append({"m": m, "x_grid": before, "y_grid": after,
                 "physical_step": pack(epsilon), "epsilon": pack(epsilon),
                 "full_births_x_radius": [pack(left), pack(right_before)],
                 "full_births_y_radius": [pack(left), pack(right_after)],
                 "full_root_x_radius": pack(root_before),
                 "full_root_y_radius": pack(root_after),
                 "middle_first_cover_x_radius": pack(left),
                 "middle_first_cover_y_radius": pack(left),
                 "middle_first_cover_x_count": 2,
                 "middle_first_cover_y_count": 1,
                 "middle_lca_x_radius": pack(root_before),
                 "middle_lca_y_radius": pack(left)})
print(json.dumps({"status": "PASS", "analytic_guards": 15, "cases": rows},
                 sort_keys=True, indent=2))
