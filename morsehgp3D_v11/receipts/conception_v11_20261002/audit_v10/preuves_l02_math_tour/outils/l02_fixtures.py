#!/usr/bin/env python3
"""Fixtures degenerees gravees pour l'audit L02 : juge complet (naissances, multifusions, verticales, attaches, Euler)
sur chaque fixture, a tous les ordres K <= min(n, 10)."""
import os
import sys
import tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from l02_judge import judge_cloud

C = [(5, 0), (4, 3), (3, 4), (0, 5), (-3, 4), (-4, 3), (-5, 0), (-4, -3), (-3, -4), (0, -5), (3, -4), (4, -3)]
FIX = {
    'E5': [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)],
    'carre_cote_2': [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)],
    'rectangle_2x4_et_centre': [(0, 0, 0), (4, 0, 0), (0, 2, 0), (4, 2, 0), (2, 1, 0)],
    'cube_2': [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)],
    'cube_2_et_centre': [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)] + [(1, 1, 1)],
    'octaedre': [(4, 2, 2), (0, 2, 2), (2, 4, 2), (2, 0, 2), (2, 2, 4), (2, 2, 0)],
    'octaedre_et_centre': [(4, 2, 2), (0, 2, 2), (2, 4, 2), (2, 0, 2), (2, 2, 4), (2, 2, 0), (2, 2, 2)],
    'ball_anchors_v7_5pts': [(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2), (0, 0, 0)],
    'continuation_avec_gain_de_couverture': [(3 + 5, 4 + 5, 0), (0 + 5, 5 + 5, 0), (-3 + 5, 4 + 5, 0), (0 + 5, -5 + 5, 0)],
    'cercle_25_12pts': [(x + 5, y + 5, 0) for x, y in C],
    'cercle_25_12pts_et_centre': [(x + 5, y + 5, 0) for x, y in C] + [(5, 5, 0)],
    'triangle_rectangle': [(0, 0, 0), (4, 0, 0), (0, 3, 0)],
    'alignes_5': [(0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 0, 0), (4, 0, 0)],
    'contre_exemple_th5_plan': [(0, 100, 0), (200, 100, 0), (101, 10, 0), (130, 15, 0), (103, 400, 0)],
    'coins_u18': [(0, 0, 0), (262143, 0, 0), (0, 262143, 0), (0, 0, 262143), (262143, 262143, 262143), (131071, 131072, 5)],
}


def main():
    exe = sys.argv[1]
    bad = 0
    with tempfile.TemporaryDirectory(dir=os.environ.get('L02_TMP')) as tmp:
        for name, P in FIX.items():
            n = len(P)
            K = min(n, 10)
            err, cnt, code = judge_cloud(exe, P, K, tmp)
            bad += err is not None
            print('%-40s n=%2d K=%2d : %s | naissances %d (pop > k : %d) fusions %d (>=3 parents : %d) verticales %d '
                  'attaches %d Euler %d | continuations %d, gains de couverture sans fusion %d, niveaux a >= 2 evenements %d'
                  % (name, n, K, 'CONFORME' if err is None else 'ECART ' + err, cnt['births'], cnt['births_pop_gt_k'],
                     cnt['merges'], cnt['arity_ge3'], cnt['verticals'], cnt['points'], cnt['euler'], cnt['continuations'],
                     cnt['cover_growth_no_merge'], cnt['levels_with_2plus_events']), flush=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
