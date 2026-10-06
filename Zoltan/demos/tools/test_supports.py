#!/usr/bin/env python3
"""Tests des vidéos « hiérarchie des supports » (tools/supports_scene.py, player/supports.js).

    python3 -O -m unittest discover -s Zoltan/demos/tools -p 'test_*.py'

- Oracle : un arbre d'ordre k fait à la main (deux objets, un mur), dont on connaît les chaînes, les IoU, la fusion et
  l'effondrement.
- Contrat des scènes locales (data/supports_k*.js, non versionnées ; sautées si absentes) : supports triés par niveau,
  chaînes emboîtées, suivi cohérent, pauses dans le balayage, résultats publiés égaux à la scène.
Aucune assertion Python nue : les tests tiennent sous python3 -O.
"""
import json
import math
from pathlib import Path
import sys
import unittest

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import supports_scene as sp  # noqa: E402

VIDEOS = HERE.parent / 'videos_hgp_hdbscan'


class ToyTree:
    """Arbre d'ordre k minimal, au format des attributs de mhgp11_formats.SupportsFile utilisés par analyse().

    Sites 0-2 : objet A ; 3-5 : objet B ; 6-9 : mur (fond, classe bâtiment). Nœuds en postordre :
      0 naissance A (boules : triangle 0-1-2, rayon 10 mm) ;
      1 naissance mur (tétraèdre 6-7-8-9, 12 mm) ;
      2 fusion A + mur (arête 2-6, 20 mm) ;
      3 naissance B (triangle 3-4-5, 15 mm) ;
      4 racine : fusion (A + mur) + B (arête 5-6, 30 mm)."""

    def __init__(self):
        self.N, self.root = 5, 4
        self.parent = [2, 2, 4, 4, 0xFFFFFFFF]
        self.children = [[], [], [0, 1], [], [2, 3]]
        self.post = [0, 1, 2, 3, 4]
        self.size = [1, 1, 3, 1, 5]
        self.ball_count = [1, 1, 1, 1, 1]
        self.first_ball = [0, 1, 2, 3, 4]
        self.node_of = [0, 1, 2, 3, 4]
        self.B = self.S = 5
        self.ball_at = [0, 1, 2, 3, 4]
        self.support_count = [1] * 5
        self.supports = [(0, 1, 2), (6, 7, 8, 9), (2, 6), (3, 4, 5), (5, 6)]
        self.arity = [len(q) for q in self.supports]
        self.radius_mm = [10, 12, 20, 15, 30]
        self.point_id = list(range(10))

    def points_of(self, s):
        return self.supports[s]

    def level(self, b):
        from fractions import Fraction
        return Fraction(self.radius_mm[b] ** 2)


class Oracle(unittest.TestCase):
    def setUp(self):
        self.f = ToyTree()
        self.gt = np.array([0, 0, 0, 1, 1, 1, -1, -1, -1, -1])
        self.void = np.zeros(10, dtype=bool)
        self.raw = np.array([11] * 6 + [50] * 4)  # vélos, puis bâtiment

    def test_chains_tracks_fusion_and_collapse(self):
        a = sp.analyse(self.f, self.gt, self.void, self.raw, 2)
        self.assertEqual(a['chains'], [[0, 2, 4], [3, 4]])
        self.assertEqual(a['best'], [1.0, 1.0])
        rows = {o: [(round(r * 1000), round(iou, 6), st) for r, iou, st, _, _ in a['tracks'][o]] for o in (0, 1)}
        # A : complet à 10 mm ; à 20 mm le mur l'absorbe (3 sites sur 7) ; à 30 mm réuni à B
        self.assertEqual(rows[0], [(10, 1.0, sp.MATCHED), (20, round(3 / 7, 6), sp.FRAGMENT), (30, 0.3, sp.FUSED)])
        self.assertEqual(rows[1], [(15, 1.0, sp.MATCHED), (30, 0.3, sp.FUSED)])
        self.assertEqual([(round(x['r'] * 1000), x['objects'], x['before']) for x in a['fusions']],
                         [(30, [0, 1], [True, True])])
        chute = [c for c in a['chutes'] if not c['fusion']]
        self.assertEqual([(round(c['r'] * 1000), c['objet'], c['fond'], c['classe']) for c in chute],
                         [(20, 0, 4, 'building')])

    def test_pauses_follow_duel_rules(self):
        a = sp.analyse(self.f, self.gt, self.void, self.raw, 2)
        m = dict(tracks=a['tracks'], fusions=a['fusions'], chutes=a['chutes'], best=a['best'])
        t = sp.schedule(m, 2, a['level'])
        roles = [(round(p['r'] * 1000), p['roles']) for p in t['pauses']]
        self.assertEqual(roles, [(10, ['best:hgp:0']), (15, ['best:hgp:1', 'sep:hgp:0+1']), (20, ['chute:hgp:0']),
                                 (30, ['fusion:hgp:0+1'])])
        texts = [''.join(x[0] for x in b['parts']) for p in t['pauses'] for b in p['badges']]
        self.assertIn('✗ A fusionne avec le bâtiment · IoU 1,00 → 0,43', texts)
        self.assertIn('✓ A et B réunis, chacun retrouvé avant', texts)


def local_scenes():
    return sorted(VIDEOS.glob('*/*/data/supports_k*.js'))


@unittest.skipUnless(local_scenes(), 'aucune scène de supports locale (lancer tools/supports_scene.py)')
class LocalScenes(unittest.TestCase):
    def test_scene_contract(self):
        for path in local_scenes():
            text = path.read_text(encoding='utf-8')
            scene = json.loads(text[text.index('=') + 1:].strip().rstrip(';'))
            self.assertEqual(scene['schema'], 'ehgp.zoltan.supports.v1', path)
            sp_ = scene['supports']
            lv, ar, sites = sp_['level'], sp_['arity'], sp_['sites']
            n = len(scene['gt'])
            self.assertTrue(all(a <= b for a, b in zip(lv, lv[1:])), path)
            self.assertTrue(set(ar) <= {2, 3, 4} and sum(ar) == len(sites), path)
            self.assertTrue(min(sites) >= 0 and max(sites) < n, path)
            c = scene['meta']['counts']
            self.assertEqual([ar.count(2), ar.count(3), ar.count(4)], [c['q2'], c['q3'], c['q4']], path)
            self.assertEqual(len(lv), c['supports'], path)
            for chain in scene['chains']:
                for (l0, p0, s0), (l1, p1, s1) in zip(chain, chain[1:]):
                    self.assertTrue(l0 <= l1 and p1 - s1 < p0 <= p1, path)  # chaque nœud contient le précédent
            for o, track in enumerate(scene['tracks']):
                self.assertTrue(all(a[0] < b[0] for a, b in zip(track, track[1:])), path)
                self.assertEqual(scene['best'][o], max(row[1] for row in track), path)
            t = scene['timing']
            self.assertTrue(all(t['sweep'] <= p['t0'] < p['t1'] <= t['summary'] for p in t['pauses']), path)
            self.assertTrue(any(p['t0'] <= t['key'] <= p['t1'] for p in t['pauses']), path)
            k = scene['meta']['k']
            res = json.loads((path.parents[1] / ('resultats_supports_k%d.json' % k)).read_text(encoding='utf-8'))
            self.assertEqual((res['best'], res['counts']), (scene['best'], c), path)
            duel = json.loads((path.parents[1] / ('resultats_duel_k%d.json' % k)).read_text(encoding='utf-8'))
            # même scène, même ordre : les meilleurs IoU des supports suivent la hiérarchie de points
            gap = max(abs(x - y) for x, y in zip(scene['best'], duel['methods']['hgp']['best']))
            self.assertLess(gap, 0.15, path)
            self.assertTrue(math.isfinite(t['duration']) and t['duration'] < 120, path)


if __name__ == '__main__':
    unittest.main()
