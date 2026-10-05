"""Vidéos HGP contre HDBSCAN des bouts : tools/duel_scene.py et player/duel.js.

Sans navigateur ; tient sous ``python3 -O``. Les contrôles des scènes locales (data/duel_k*.js) ne tournent que si
elles existent ; le rejeu JavaScript, seulement si Node.js est présent.

    python3 -O -m unittest discover -s Zoltan/demos/tools -p 'test_*.py'
"""
from __future__ import annotations

import itertools
import json
import re
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import duel_scene as ds  # noqa: E402
from test_scene_contract import ciede2000, contrast, hex_rgb, rgba_over, simulate  # noqa: E402


def duel_source():
    return (ROOT / 'player' / 'duel.js').read_text(encoding='utf-8')


def tokens(source):
    out = {}
    for name, body in re.findall(r'\n    (dark|light): \{(.*?)\n    \},', source, re.S):
        tok = dict(re.findall(r"\b([A-Za-z]+): '([^']+)'", body))
        tok.update(dict((k, float(v)) for k, v in re.findall(r'\b([A-Za-z]+): ([0-9.]+),', body)))
        tok['objects'] = re.findall(r"'(#[0-9a-f]{6})'", re.search(r'objects: \[([^\]]*)\]', body).group(1))
        out[name] = tok
    return out


class Palette(unittest.TestCase):
    def test_shared_tokens_match_player(self):
        duel = tokens(duel_source())
        demo = tokens((ROOT / 'player' / 'player.js').read_text(encoding='utf-8'))
        self.assertEqual(sorted(duel), ['dark', 'light'])
        for name in duel:
            for key in ('bg', 'panel', 'frame', 'grid', 'text', 'dim', 'plate', 'fusion', 'objects'):
                self.assertEqual(duel[name][key], demo[name][key], f'{name} : {key} différent de player.js')

    def test_signal_colors(self):
        for name, tok in tokens(duel_source()).items():
            panel = hex_rgb(tok['panel'])
            roles = {'A': hex_rgb(tok['objects'][0]), 'B': hex_rgb(tok['objects'][1]), 'C': hex_rgb(tok['objects'][2]),
                     'fusion': hex_rgb(tok['fusion']), 'autre groupe': hex_rgb(tok['other'])}
            for role in ('A', 'B', 'C', 'fusion'):
                self.assertGreaterEqual(contrast(roles[role], panel), 3.0, f'{name} : {role} sur le panneau')
            self.assertGreaterEqual(contrast(hex_rgb(tok['ok']), panel), 3.0, f'{name} : coche sur le panneau')
            for (r1, c1), (r2, c2) in itertools.combinations(roles.items(), 2):
                for kind in ('normale', 'deutéranopie', 'protanopie'):
                    d = ciede2000(simulate(c1, kind), simulate(c2, kind))
                    self.assertGreaterEqual(d, 20.0, f'{name}, vision {kind} : {r1} et {r2} à ΔE00 {d:.1f}')
            # point seul (petit, pâle) et autre groupe (moyen, gris) : aussi distincts par la taille
            alone = rgba_over(tok['alone'], panel)
            self.assertGreaterEqual(ciede2000(alone, roles['autre groupe']), 8.0, name)
            self.assertGreater(tok['halo'], 0.1, name)
            self.assertLess(tok['halo'], 0.35, name)


def toy(objects_of_points, unions, objects):
    """Plateaux jouets : tous les points entrent à 0, puis des unions (niveau, a, b), une par plateau."""
    plateaus = [(0.0, [('enter', s) for s in range(len(objects_of_points))])]
    for level, a, b in unions:
        plateaus.append((level, [('union', a, b)]))
    obj = np.array(objects_of_points)
    void = np.zeros(len(obj), dtype=bool)
    levels = ds.strictly_increasing([lv for lv, _ in plateaus])
    return plateaus, levels, obj, void


class Oracles(unittest.TestCase):
    def test_good_fusion(self):
        plateaus, levels, obj, void = toy([0, 0, 1, 1], [(1000, 0, 1), (2000, 2, 3), (3000, 1, 2)], 2)
        track, best, fusions = ds.tracks(plateaus, levels, 4, obj, void, 2, [0, 2])
        self.assertEqual(best, [1.0, 1.0])
        self.assertEqual(fusions, [dict(r=3.0, objects=[0, 1], before=[True, True])])
        self.assertEqual([row[2] for row in track[0]], [ds.NONE, ds.MATCHED, ds.FUSED])
        events = ds.events_of('hgp', dict(tracks=track, fusions=fusions, best=best), 2)
        self.assertEqual(events, [(1.0, 'best:hgp:0'), (2.0, 'best:hgp:1'), (2.0, 'sep:hgp:0+1'), (3.0, 'fusion:hgp:0+1')])

    def test_bad_fusion(self):
        # A = {0, 1, 2}, B = {3, 4, 5} : 0-1 (A 2/3), puis 1-3 : A et B réunis avant que B soit retrouvé
        plateaus, levels, obj, void = toy([0, 0, 0, 1, 1, 1], [(1000, 0, 1), (2000, 1, 3), (3000, 4, 5)], 2)
        track, best, fusions = ds.tracks(plateaus, levels, 6, obj, void, 2, [0, 3])
        self.assertEqual(fusions, [dict(r=2.0, objects=[0, 1], before=[True, False])])
        self.assertAlmostEqual(best[1], 1 / 5)  # B dans {0, 1, 3} : 1 / (3 + 3 - 1)
        events = ds.events_of('hdbscan', dict(tracks=track, fusions=fusions, best=best), 2)
        self.assertEqual(events, [(1.0, 'best:hdbscan:0'), (2.0, 'fusion:hdbscan:0+1')])

    def test_collapse_into_background(self):
        # A = {0, 1, 2} complet à 1,2 ; le fond {3, 4, 5, 6} (bâtiment) se forme, puis A l'absorbe à 2 : IoU 1 -> 3/7
        plateaus, levels, obj, void = toy([0, 0, 0, -1, -1, -1, -1],
                                          [(1000, 0, 1), (1200, 1, 2), (1300, 3, 4), (1400, 4, 5), (1500, 5, 6),
                                           (2000, 2, 3)], 1)
        raw = np.array([11 | 7 << 16] * 3 + [50] * 4)  # vélo, puis bâtiment
        track, best, fusions = ds.tracks(plateaus, levels, 7, obj, void, 1, [0])
        chutes = ds.collapses(plateaus, levels, 7, obj, void, raw, 1, [0], track)
        self.assertEqual(len(chutes), 1)
        self.assertEqual((chutes[0]['r'], chutes[0]['fond'], chutes[0]['classe'], chutes[0]['fusion']),
                         (2.0, 4, 'building', False))
        self.assertAlmostEqual(chutes[0]['apres'], 3 / 7, places=6)
        events = ds.events_of('hgp', dict(tracks=track, fusions=fusions, best=best, chutes=chutes), 1)
        self.assertEqual(events, [(1.2, 'best:hgp:0'), (2.0, 'chute:hgp:0')])

    def test_hdbscan_plateaus_group_ties_full_distance(self):
        # niveau = distance d'atteignabilité mutuelle entière (même échelle que le rayon de HGP), plus de facteur 1/2
        tree = np.array([[0, 1, 4.0, 2], [2, 3, 4.0, 2], [4, 5, 10.0, 4]], dtype=np.float64)
        plateaus = ds.hdbscan_plateaus(tree, 4)
        self.assertEqual([lv for lv, _ in plateaus], [0.0, 4.0, 10.0])
        self.assertEqual(plateaus[1][1], [('union', 0, 1), ('union', 2, 3)])
        self.assertEqual(plateaus[2][1], [('union', 0, 2)])

    def test_levels_stay_strictly_increasing(self):
        out = ds.strictly_increasing([0.0, 1.0, 1.0, 1.0, 2.0])
        self.assertTrue(bool(np.all(np.diff(out) > 0)))
        self.assertEqual(out[-1], 0.002)

    def test_iou_text(self):
        self.assertEqual([ds.iou_text(v) for v in (0.502, 0.4999, 0.49, 0.51, 0.5, 0.963768)],
                         ['0,502', '0,4999', '0,49', '0,51', '0,50', '0,96'])


def local_scenes():
    return sorted(ROOT.glob('videos_hgp_hdbscan/*/*/data/duel_k*.js'))


def load(path):
    text = path.read_text(encoding='utf-8')
    return json.loads(text[text.index('{'):text.rindex('}') + 1])


class LocalScenes(unittest.TestCase):
    def setUp(self):
        self.scenes = local_scenes()
        if not self.scenes:
            self.skipTest('aucune scène locale (lancer tools/duel_scene.py)')

    def test_scene_contract(self):
        roles = {'ok', 'fusion', 'text', 'dim', 'obj0', 'obj1', 'obj2'}
        for path in self.scenes:
            scene = load(path)
            self.assertEqual(scene['schema'], 'ehgp.zoltan.duel.v3')
            k = scene['meta']['k']
            spec = json.loads((path.parents[1] / 'bout.json').read_text(encoding='utf-8'))
            self.assertEqual(scene['variante'], spec['variante'], path)
            n = len(scene['gt'])
            for name in ('hgp', 'hdbscan'):
                m = scene['methods'][name]
                self.assertEqual(m['best'], spec['orders'][str(k)][name], f'{path} {name}')
                self.assertTrue(bool(np.all(np.diff(m['levels']) > 0)), f'{path} {name}')
                self.assertTrue(all(0 <= s < n and scene['gt'][s] == o for o, s in enumerate(m['seeds'])), path)
            t = scene['timing']
            times = [row[0] for row in t['schedule']]
            self.assertEqual(times, sorted(times), path)
            self.assertTrue(all(t['sweep'] <= p['t0'] < p['t1'] <= t['summary'] for p in t['pauses']), path)
            self.assertTrue(any(p['t0'] <= t['key'] <= p['t1'] for p in t['pauses']), path)
            # HDBSCAN balaie d'abord, HGP attend à rmin ; puis HGP balaie et HDBSCAN le suit au même r
            self.assertTrue(all(p['t1'] <= t['switch'] for p in t['pauses'] if p['method'] == 'hdbscan'), path)
            self.assertTrue(all(p['t0'] > t['switch'] for p in t['pauses'] if p['method'] == 'hgp'), path)
            self.assertTrue(all(row[2] == t['rmin'] for row in t['schedule'] if row[0] <= t['switch']), path)
            after = [row for row in t['schedule'] if row[0] > t['switch'] + 1.6]
            self.assertTrue(after and all(row[1] == row[2] for row in after), path)
            self.assertEqual(max(row[1] for row in t['schedule'] if row[0] <= t['switch']), t['rmax'], path)
            self.assertEqual(max(row[2] for row in t['schedule']), t['rmax'], path)
            self.assertTrue(all(all(r.split(':')[1] == p['method'] for r in p['roles']) for p in t['pauses']), path)
            self.assertTrue(all(not p['badges']['hgp'] for p in t['pauses'] if p['method'] == 'hdbscan'), path)
            key = next(p for p in t['pauses'] if p['t0'] <= t['key'] <= p['t1'])
            if spec['issues'][str(k)] == 'win':  # l'affiche montre HGP et HDBSCAN au même r
                self.assertEqual(key['method'], 'hgp', path)
            for p in t['pauses']:
                for side in ('hgp', 'hdbscan'):
                    for b in p['badges'][side]:
                        self.assertIn(b['border'], roles, path)
                        self.assertTrue(all(part[1] in roles for part in b['parts']), path)
            # l'issue publiée est celle des meilleurs IoU de la scène ; une vidéo par ordre gagnant, sinon k = 5
            ok = {name: min(scene['methods'][name]['best']) > 0.5 for name in ('hgp', 'hdbscan')}
            issue = {(True, False): 'win', (False, True): 'loss', (False, False): 'both_fail',
                     (True, True): 'both_ok'}[(ok['hgp'], ok['hdbscan'])]
            self.assertEqual(spec['issues'][str(k)], issue, path)
            wins = [o for o in ('5', '10') if spec['issues'][o] == 'win']
            self.assertEqual(spec['ordres_video'], wins or ['5'], path)
            self.assertIn(str(k), spec['ordres_video'], path)

    def test_javascript_replay_matches_python(self):
        node = shutil.which('node')
        if node is None:
            self.skipTest('Node.js absent')
        script = r'''
const fs = require('fs');
global.window = globalThis;
require(process.argv[1]);
const text = fs.readFileSync(process.argv[2], 'utf8');
const scene = JSON.parse(text.slice(text.indexOf('{'), text.lastIndexOf('}') + 1));
const S = DuelPlayer.prepare(scene);
const out = {};
const levels = JSON.parse(process.argv[3]);
for (const key of ['hgp', 'hdbscan']) {
  out[key] = levels.map((r) => {
    const h = DuelPlayer.hierarchyAt(S, S.methods[key], r);
    return Array.from(h.root).map((x, i) => (h.big[i] ? x : -1));
  });
}
process.stdout.write(JSON.stringify(out));
'''
        for path in self.scenes:
            scene = load(path)
            # juge d'échantillon : la première pause, celle de l'instant clé et la dernière (pas toutes)
            t = scene['timing']
            pauses = t['pauses']
            keyed = [p for p in pauses if p['t0'] <= t['key'] <= p['t1']]
            levels = sorted(set(p['r'] for p in pauses[:1] + keyed + pauses[-1:]))
            done = subprocess.run([node, '-e', script, str(ROOT / 'player' / 'duel.js'), str(path), json.dumps(levels)],
                                  capture_output=True, text=True, timeout=120)
            self.assertEqual(done.returncode, 0, done.stderr)
            got = json.loads(done.stdout)
            n = len(scene['gt'])
            for key in ('hgp', 'hdbscan'):
                m = scene['methods'][key]
                plateau, kinds = np.array(m['ev_plateau']), np.array(m['ev_kind'])
                ea, eb = np.array(m['ev_a']), np.array(m['ev_b'])
                for r, js in zip(levels, got[key]):
                    rp = ds.Replay(n, np.array(scene['gt']), np.array(scene['void'], dtype=bool), len(scene['objects']))
                    last = int(np.searchsorted(np.array(m['levels']), r, side='right'))  # plateaux de niveau <= r
                    upto = plateau < last
                    rp.apply([('enter', a) if kind == 0 else ('union', a, b) for kind, a, b in
                              zip(kinds[upto].tolist(), ea[upto].tolist(), eb[upto].tolist())])
                    roots = [rp.find(i) for i in range(n)]
                    py = [roots[i] if rp.entered[i] and rp.size[roots[i]] >= 2 else -1 for i in range(n)]
                    self.assertEqual(canonical(py), canonical(js), f'{path} {key} r = {r}')


def canonical(labels):
    """Partition sans les étiquettes : chaque bloc renommé par son plus petit point, -1 pour les points seuls."""
    first = {}
    for i, x in enumerate(labels):
        if x != -1 and x not in first:
            first[x] = i
    return [first[x] if x != -1 else -1 for x in labels]


if __name__ == '__main__':
    unittest.main()
