"""Contrat entre build_scene.py (rôles de couleur, instant clé) et player/player.js (thèmes).

Sans navigateur ; tient sous ``python3 -O``.

    python3 -m unittest discover -s Zoltan/demos/tools -p 'test_*.py'
"""
from __future__ import annotations

import itertools
import json
import re
import sys
import unittest
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import build_scene  # noqa: E402


def player_source():
    return (ROOT / 'player' / 'player.js').read_text(encoding='utf-8')


class ColorRoles(unittest.TestCase):
    def test_builder_writes_roles_not_colors(self):
        self.assertEqual(build_scene.COLORS, ['obj0', 'obj1', 'obj2'])
        self.assertEqual((build_scene.FUSION, build_scene.NEUTRAL), ('fusion', 'text'))
        src = (ROOT / 'tools' / 'build_scene.py').read_text(encoding='utf-8')
        self.assertIsNone(re.search(r"'#[0-9a-fA-F]{6}'", src), 'couleur hexadécimale dans build_scene.py')

    def test_each_theme_has_one_color_per_object_role(self):
        js = player_source()
        themes = re.findall(r'\n    (dark|light): \{(.*?)\n    \},', js, re.S)
        self.assertEqual(sorted(name for name, _ in themes), ['dark', 'light'])
        keys = None
        for name, body in themes:
            objects = re.search(r'objects: \[([^\]]*)\]', body)
            self.assertIsNotNone(objects, name)
            self.assertEqual(len(re.findall(r"'#[0-9a-f]{6}'", objects.group(1))), len(build_scene.COLORS), name)
            found = set(re.findall(r'\b([A-Za-z]+): ', body))
            keys = found if keys is None else keys
            self.assertEqual(found, keys, f'{name} : jetons différents de ceux de l\'autre thème')
        self.assertIn("role === 'fusion'", js)
        self.assertIn("role === 'text'", js)
        self.assertIn('obj[0-2]', js)

    def test_local_scenes_carry_roles_and_key_time(self):
        scenes = sorted(ROOT.glob('*/0*_*/data/scene_*.js'))  # catégorie/démo/data
        if not scenes:
            self.skipTest('aucune scène locale (lancer tools/build_scene.py)')
        roles = {'obj0', 'obj1', 'obj2', 'fusion', 'text'}
        for path in scenes:
            text = path.read_text(encoding='utf-8')
            scene = json.loads(text[text.index('{'):text.rindex('}') + 1])
            self.assertTrue(all(c['color'] in roles for c in scene['captions']), path.name)
            self.assertTrue(all(c.get('kind') for c in scene['captions']), path.name)
            self.assertFalse(any('color' in o for o in scene['objects']), path.name)
            t = scene['timing']
            self.assertTrue(0 < t['key'] < t['summary'], path.name)


# Simulation du daltonisme (Machado, Oliveira, Fernandes 2009, sévérité 1, en RGB linéaire)
MACHADO = {
    'deutéranopie': np.array([[0.367322, 0.860646, -0.227968], [0.280085, 0.672501, 0.047413], [-0.011820, 0.042940, 0.968881]]),
    'protanopie': np.array([[0.152286, 1.052583, -0.204868], [0.114503, 0.786281, 0.099216], [-0.003882, -0.048116, 1.051998]]),
}


def hex_rgb(h):
    return np.array([int(h[i:i + 2], 16) for i in (1, 3, 5)], float) / 255


def rgba_over(css, under):
    """Couleur CSS (#rrggbb ou rgba(r,g,b,a)) composée sur ``under``, comme Canvas2D."""
    m = re.fullmatch(r'rgba\((\d+),(\d+),(\d+),([0-9.]+)\)', css.replace(' ', ''))
    if not m:
        return hex_rgb(css)
    a = float(m.group(4))
    return a * np.array([int(m.group(i)) for i in (1, 2, 3)], float) / 255 + (1 - a) * under


def lin(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def simulate(c, kind):
    if kind == 'normale':
        return c
    out = np.clip(lin(c) @ MACHADO[kind].T, 0, 1)
    return np.where(out <= 0.0031308, 12.92 * out, 1.055 * out ** (1 / 2.4) - 0.055)


def lab(c):
    xyz = lin(c) @ np.array([[0.4124564, 0.3575761, 0.1804375], [0.2126729, 0.7151522, 0.0721750],
                             [0.0193339, 0.1191920, 0.9503041]]).T / np.array([0.95047, 1.0, 1.08883])
    f = np.where(xyz > (6 / 29) ** 3, np.cbrt(xyz), xyz / (3 * (6 / 29) ** 2) + 4 / 29)
    return np.array([116 * f[1] - 16, 500 * (f[0] - f[1]), 200 * (f[1] - f[2])])


def ciede2000(c1, c2):
    return ciede2000_lab(lab(c1), lab(c2))


def ciede2000_lab(lab1, lab2):
    (L1, a1, b1), (L2, a2, b2) = lab1, lab2
    cm = (np.hypot(a1, b1) + np.hypot(a2, b2)) / 2
    g = 0.5 * (1 - np.sqrt(cm ** 7 / (cm ** 7 + 25 ** 7)))
    a1p, a2p = (1 + g) * a1, (1 + g) * a2
    c1p, c2p = np.hypot(a1p, b1), np.hypot(a2p, b2)
    h1p, h2p = np.degrees(np.arctan2(b1, a1p)) % 360, np.degrees(np.arctan2(b2, a2p)) % 360
    dh = 0.0 if c1p * c2p == 0 else (h2p - h1p + 180) % 360 - 180
    dHp = 2 * np.sqrt(c1p * c2p) * np.sin(np.radians(dh / 2))
    lpm, cpm = (L1 + L2) / 2, (c1p + c2p) / 2
    hs = h1p + h2p
    hpm = hs if c1p * c2p == 0 else (hs / 2 if abs(h1p - h2p) <= 180 else ((hs + 360) / 2 if hs < 360 else (hs - 360) / 2))
    tt = (1 - 0.17 * np.cos(np.radians(hpm - 30)) + 0.24 * np.cos(np.radians(2 * hpm))
          + 0.32 * np.cos(np.radians(3 * hpm + 6)) - 0.20 * np.cos(np.radians(4 * hpm - 63)))
    rt = -np.sin(np.radians(60 * np.exp(-((hpm - 275) / 25) ** 2))) * 2 * np.sqrt(cpm ** 7 / (cpm ** 7 + 25 ** 7))
    sl = 1 + 0.015 * (lpm - 50) ** 2 / np.sqrt(20 + (lpm - 50) ** 2)
    sc, sh = 1 + 0.045 * cpm, 1 + 0.015 * cpm * tt
    return float(np.sqrt(((L2 - L1) / sl) ** 2 + ((c2p - c1p) / sc) ** 2 + (dHp / sh) ** 2 + rt * (c2p - c1p) / sc * dHp / sh))


def contrast(c1, c2):
    y = [float(lin(c) @ np.array([0.2126, 0.7152, 0.0722])) for c in (c1, c2)]
    return (max(y) + 0.05) / (min(y) + 0.05)


def themes():
    """Jetons de THEMES lus dans player.js : {thème: {jeton: valeur}}."""
    out = {}
    for name, body in re.findall(r'\n    (dark|light): \{(.*?)\n    \},', player_source(), re.S):
        tok = dict(re.findall(r"\b([A-Za-z]+): '([^']+)'", body))
        tok['objects'] = re.findall(r"'(#[0-9a-f]{6})'", re.search(r'objects: \[([^\]]*)\]', body).group(1))
        out[name] = tok
    return out


class Palette(unittest.TestCase):
    def test_ciede2000_on_sharma_pairs(self):
        # Sharma, Wu, Dalal 2005, table 1 : paires 1, 7, 17 et 25
        pairs = [((50, 2.6772, -79.7751), (50, 0, -82.7485), 2.0425), ((50, 0, 0), (50, -1, 2), 2.3669),
                 ((50, 2.5, 0), (73, 25, -18), 27.1492), ((60.2574, -34.0099, 36.2677), (60.4626, -34.1751, 39.4387), 1.2644)]
        for l1, l2, want in pairs:
            self.assertAlmostEqual(ciede2000_lab(l1, l2), want, places=4)

    def test_signal_colors_are_distinct_for_colour_blind_viewers(self):
        for name, tok in themes().items():
            panel = hex_rgb(tok['panel'])
            roles = {'A': hex_rgb(tok['objects'][0]), 'B': hex_rgb(tok['objects'][1]), 'C': hex_rgb(tok['objects'][2]),
                     'fusion': hex_rgb(tok['fusion']), 'autre cluster': rgba_over(tok['alive'], panel)}
            for role in ('A', 'B', 'C', 'fusion'):
                self.assertGreaterEqual(contrast(roles[role], panel), 3.0, f'{name} : {role} sur le panneau')
            for (r1, c1), (r2, c2) in itertools.combinations(roles.items(), 2):
                for kind in ('normale', 'deutéranopie', 'protanopie'):
                    d = ciede2000(simulate(c1, kind), simulate(c2, kind))
                    self.assertGreaterEqual(d, 20.0, f'{name}, vision {kind} : {r1} et {r2} à ΔE00 {d:.1f}')

    def test_text_is_readable(self):
        for name, tok in themes().items():
            for bg in ('panel', 'bg'):
                self.assertGreaterEqual(contrast(hex_rgb(tok['text']), hex_rgb(tok[bg])), 7.0, name)
                self.assertGreaterEqual(contrast(hex_rgb(tok['dim']), hex_rgb(tok[bg])), 4.5, name)


class KeyTime(unittest.TestCase):
    CAPS = [
        {'t0': 0.0, 't1': 4.0, 'kind': 'intro'},
        {'t0': 5.0, 't1': 7.0, 'kind': 'match'},
        {'t0': 8.0, 't1': 10.0, 'kind': 'match'},
        {'t0': 11.0, 't1': 13.0, 'kind': 'absorb'},
        {'t0': 14.0, 't1': 16.0, 'kind': 'match'},
        {'t0': 17.0, 't1': 19.0, 'kind': 'merge'},
    ]

    def test_rules(self):
        self.assertEqual(build_scene.key_time(self.CAPS, 'merge', 30.0), 18.0)
        self.assertEqual(build_scene.key_time(self.CAPS, 'absorb', 30.0), 12.0)
        self.assertEqual(build_scene.key_time(self.CAPS, 'match_all', 30.0), 9.0)

    def test_fallbacks(self):
        no_merge = [c for c in self.CAPS if c['kind'] != 'merge']
        self.assertEqual(build_scene.key_time(no_merge, 'merge', 30.0), 12.0)
        intro = self.CAPS[:1]
        self.assertEqual(build_scene.key_time(intro, 'match_all', 30.0), 30.0)
        with self.assertRaises(ValueError):
            build_scene.key_time(self.CAPS, 'fusion', 30.0)


if __name__ == '__main__':
    unittest.main()
