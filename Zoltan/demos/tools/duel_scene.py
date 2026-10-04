#!/usr/bin/env python3
"""Scène « duel » d'une variante d'exemple : la hiérarchie de points HGP et celle de HDBSCAN, côte à côte, même k.

    python3 Zoltan/demos/tools/duel_scene.py --export BUILD/mhgp11_points_export [--data DONNEES] \
        [--members SESSION/lidar] VARIANTE [VARIANTE ...] [--k 5]

VARIANTE est un sous-dossier d'exemple (`instances/` ou `sans_sol/`, écrits par tools/choisir_exemples.py) : bout.json
(schéma ehgp.zoltan.bout_variante.v1 : le groupe d'objets, la découpe, les meilleurs IoU mesurés) et data/. Sans --k,
l'ordre montré par la variante.

- HGP : hiérarchie de points H^r_{k+1} de morsehgp3D_v11 (docs/HIERARCHIE_POINTS.md), calculée par l'export natif
  bench/points_export.cpp (tour FULL exacte, kmax = 10, ordres 2, 3, 5, 10, comme la campagne G4) puis la règle
  bench/points_radius.py (marge en rayon, décisions exactes). Les blocs ne se réunissent qu'aux fusions de FULL.
- HDBSCAN : arbre du lien simple de l'atteignabilité mutuelle de scikit-learn (min_samples = k, soi compris), celui
  de bench/points_hierarchy.py.
- Objets suivis : les instances du groupe ; tout autre point (autre instance, mur, végétation, sol laissé par
  Patchwork++) est un point du fond, qui compte dans les IoU comme dans la mesure (points void exclus).
- Contrôle : le meilleur IoU de chaque objet doit être exactement celui de bout.json (tools/mesurer_bouts.py) et, si
  --members est donné, le meilleur bloc doit avoir exactement les points publiés par G4 ; sinon refus, code 1.

Niveau commun r, convention de la thèse : le rayon pour HGP, la distance d'atteignabilité mutuelle divisée par 2 pour
HDBSCAN (celle où les deux hiérarchies coïncident à k = 1, la liaison simple).

Suivi : pour chaque objet et chaque méthode, une graine prise dans le meilleur bloc de l'objet ; la branche suivie est
le bloc qui contient la graine, à chaque niveau. Parmi les points de l'objet dans ce meilleur bloc, la graine est
celle dont la branche recouvre le mieux l'objet avant ce bloc (aire sous la courbe d'IoU en log r) : la branche
passe donc toujours par le meilleur bloc, et son IoU maximal est le meilleur IoU publié (contrôlé).

Caméra : le côté du capteur (retrouvé dans la trame officielle, contrôlé par l'empreinte de la découpe), sauf si un
autre azimut sépare mieux les objets à l'écran ou évite qu'un point du fond les masque. Cadrage sur les objets.

Sorties : VARIANTE/data/duel_k<k>.js (coordonnées, ignoré par git) et VARIANTE/resultats_duel_k<k>.json (niveaux,
IoU, événements ; aucune coordonnée).
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import time

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'morsehgp3D_v11' / 'bench'))
import points_hierarchy as ph  # noqa: E402
import points_radius as prad  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
from kitti import FR  # noqa: E402

LETTERS = 'ABC'
NUMBER = {1: 'un', 2: 'deux', 3: 'trois'}
WORD = {'bicycle': 'vélo', 'person': 'piéton', 'car': 'voiture', 'bicyclist': 'cycliste'}
KMAX, ORDERS = 10, (2, 3, 5, 10)
NONE, FRAGMENT, MATCHED, FUSED = 0, 1, 2, 3


class Refus(RuntimeError):
    pass


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def objects_of(raw, keys):
    """Objet suivi de chaque point (rang de sa clé dans le groupe, -1 sinon) et masque des points void."""
    from kitti import VOID
    raw = np.asarray(raw, dtype=np.int64)
    obj = np.full(len(raw), -1, dtype=np.int64)
    for o, key in enumerate(keys):
        obj[raw == key] = o
    return obj, np.isin(raw & 0xFFFF, VOID)


def title(classes):
    seen = []
    for cl in classes:
        if cl not in seen:
            seen.append(cl)
    parts = []
    for cl in sorted(seen, key=lambda c: (c != 'person', c)):
        n, word = classes.count(cl), WORD.get(cl, cl)
        parts.append('un ' + word if n == 1 else '%s %ss' % (NUMBER[n], word))
    text = ' et '.join(parts)
    return text[0].upper() + text[1:]


# ------------------------------------------------------------------ hiérarchies

def export_native(binary, xyz, workers):
    """Export MHGP11PH de la tour FULL (mêmes arguments que bench/points_campaign.py)."""
    with tempfile.TemporaryDirectory(prefix='duel_') as work:
        paths = [Path(work) / name for name in ('sites.xyz', 'sites.ids', 'tower.ph')]
        np.asarray(xyz, dtype='<u4').tofile(paths[0])
        np.arange(len(xyz), dtype='<u4').tofile(paths[1])
        done = subprocess.run([str(binary), str(paths[0]), str(paths[1]), str(paths[2]), str(KMAX),
                               ','.join(map(str, ORDERS)), str(workers), str(48 << 30)],
                              capture_output=True, text=True, timeout=3600)
        if done.returncode != 0:
            raise Refus('export natif : code %d %s %s' % (done.returncode, done.stdout[-400:], done.stderr[-400:]))
        report = json.loads(done.stdout.strip().splitlines()[-1])
        return ph.read_export(paths[2]), report


def hgp_plateaus(hanging, ids):
    """Événements de H^r_{k+1} par plateau, dans l'ordre exact de ph.evaluate_hanging, sur les indices d'entrée :
    liste de (rayon en mm, [('enter', s) | ('union', a, b)])."""
    order = hanging.order
    kids, merges = order.merge_list()
    entries = sorted(range(order.n), key=lambda i: (int(hanging.floor[i]), bool(hanging.strict[i])))
    entries = ph.sort_strict_groups(hanging, entries)
    dsu, rep = list(range(order.size)), [-1] * order.size

    def find(x):
        root = x
        while dsu[root] != root:
            root = dsu[root]
        while dsu[x] != root:
            dsu[x], x = root, dsu[x]
        return root

    def enter(i, out):
        root, s = find(int(hanging.owner[i])), int(ids[i])
        out.append(('enter', s))
        if rep[root] < 0:
            rep[root] = s
        else:
            out.append(('union', rep[root], s))

    def merge(parts, v, out):
        roots = []
        for p in list(parts) + [v]:
            r = find(p)
            if r not in roots:
                roots.append(r)
        reps = [rep[r] for r in roots if rep[r] >= 0]
        for r in roots[1:]:
            dsu[r] = roots[0]
        rep[roots[0]] = reps[0] if reps else -1
        for s in reps[1:]:
            out.append(('union', reps[0], s))

    plateaus = []
    rank = order.rank.tolist()
    mi = ei = 0
    while mi < len(merges) or ei < len(entries):
        r = min(rank[merges[mi]] if mi < len(merges) else 1 << 62,
                int(hanging.floor[entries[ei]]) if ei < len(entries) else 1 << 62)
        out = []
        while mi < len(merges) and rank[merges[mi]] == r:
            merge(kids[merges[mi]], merges[mi], out)
            mi += 1
        while ei < len(entries) and int(hanging.floor[entries[ei]]) == r and not hanging.strict[entries[ei]]:
            enter(entries[ei], out)
            ei += 1
        if out:
            plateaus.append((math.sqrt(order.levels.approx[r]), out))
        while ei < len(entries) and int(hanging.floor[entries[ei]]) == r:
            i = entries[ei]
            out = []
            while ei < len(entries) and int(hanging.floor[entries[ei]]) == r and \
                    hanging.cmp_entries(entries[ei], i) == 0:
                enter(entries[ei], out)
                ei += 1
            plateaus.append((hanging.values[i].approx(), out))
    return plateaus


def hdbscan_plateaus(tree, n):
    """Événements de l'arbre du lien simple, fusions ex aequo d'un même plateau ; niveau = distance / 2 (mm)."""
    left = tree['left_node'] if tree.dtype.names else tree[:, 0]
    right = tree['right_node'] if tree.dtype.names else tree[:, 1]
    value = tree['value'] if tree.dtype.names else tree[:, 2]
    left, right, value = left.astype(np.int64).tolist(), right.astype(np.int64).tolist(), value.tolist()
    rep = list(range(n)) + [-1] * len(value)
    plateaus = [(0.0, [('enter', s) for s in range(n)])]
    j = 0
    while j < len(value):
        level, out = value[j], []
        while j < len(value) and value[j] == level:
            a, b = rep[left[j]], rep[right[j]]
            rep[n + j] = a
            out.append(('union', a, b))
            j += 1
        plateaus.append((level / 2.0, out))
    return plateaus


def strictly_increasing(levels):
    """Niveaux en mètres strictement croissants : deux plateaux distincts gardent leur ordre exact en flottant."""
    out = np.asarray(levels, dtype=np.float64) / 1000.0
    for p in range(1, len(out)):
        if out[p] <= out[p - 1]:
            out[p] = np.nextafter(out[p - 1], np.inf)
    return out


# ------------------------------------------------------------------ suivi des branches

class Replay(object):
    """Rejoue les plateaux : union-find des sites, comptes par objet (points void exclus des IoU)."""

    def __init__(self, n, obj, void, objects):
        self.n, self.objects = n, objects
        self.obj, self.void = obj, void
        self.dsu = np.arange(n)
        self.entered = np.zeros(n, dtype=bool)
        self.size = np.zeros(n, dtype=np.int64)  # sites entrés, void compris
        self.nonvoid = np.zeros(n, dtype=np.int64)
        self.count = np.zeros((n, objects), dtype=np.int64)
        self.total = np.array([int(np.sum((obj == o) & ~void)) for o in range(objects)], dtype=np.int64)

    def find(self, x):
        dsu = self.dsu
        root = x
        while dsu[root] != root:
            root = dsu[root]
        while dsu[x] != root:
            dsu[x], x = root, dsu[x]
        return root

    def apply(self, events):
        for ev in events:
            if ev[0] == 'enter':
                s = ev[1]
                self.entered[s] = True
                self.size[s] = 1
                if not self.void[s]:
                    self.nonvoid[s] = 1
                    if self.obj[s] >= 0:
                        self.count[s, self.obj[s]] = 1
            else:
                a, b = self.find(ev[1]), self.find(ev[2])
                if a == b:
                    continue
                if self.size[a] < self.size[b]:
                    a, b = b, a
                self.dsu[b] = a
                self.size[a] += self.size[b]
                self.nonvoid[a] += self.nonvoid[b]
                self.count[a] += self.count[b]

    def iou(self, root, o):
        c = int(self.count[root, o])
        if c == 0:
            return 0.0
        return c / (int(self.nonvoid[root]) + int(self.total[o]) - c)


def best_block_sites(plateaus, levels, n, obj, void, objects):
    """Meilleur bloc de chaque objet (premier plateau qui atteint le meilleur IoU) : (plateau, sites, IoU)."""
    rp = Replay(n, obj, void, objects)
    best = [(-1.0, None, None)] * objects
    for p, (_, events) in enumerate(plateaus):
        rp.apply(events)
        touched = set(rp.find(e[1]) for e in events)
        for root in touched:
            for o in range(objects):
                v = rp.iou(root, o)
                if v > best[o][0]:
                    best[o] = (v, p, root)
    out = []
    for o in range(objects):
        v, p, _ = best[o]
        rp = Replay(n, obj, void, objects)
        for q in range(p + 1):
            rp.apply(plateaus[q][1])
        roots = np.array([rp.find(s) for s in range(n)])
        # le bloc de ce plateau qui atteint v
        cand = [r for r in set(roots[rp.entered].tolist()) if abs(rp.iou(r, o) - v) == 0.0]
        root = min(cand, key=lambda r: int(np.min(np.flatnonzero(roots == r))))
        sites = np.flatnonzero((roots == root) & rp.entered)
        out.append((p, sites, v))
    return out


def choose_seeds(plateaus, levels, n, obj, void, objects, blocks):
    """Graine de chaque objet : dans son meilleur bloc, le point de l'objet dont la branche a la plus grande aire
    sous la courbe d'IoU (en log r) jusqu'à ce bloc ; ex aequo : le plus petit indice."""
    seeds = []
    logs = np.log(np.maximum(levels, 1e-6))
    for o in range(objects):
        p_best, sites, _ = blocks[o]
        cand = [s for s in sites.tolist() if obj[s] == o and not void[s]]
        rp = Replay(n, obj, void, objects)
        area = np.zeros(len(cand))
        for p in range(p_best + 1):
            rp.apply(plateaus[p][1])
            width = (logs[p + 1] - logs[p]) if p + 1 < len(logs) else 0.0
            if p == p_best:
                width = 0.0
            for j, s in enumerate(cand):
                if rp.entered[s]:
                    root = rp.find(s)
                    if rp.size[root] >= 2:
                        area[j] += width * rp.iou(root, o)
        j = int(np.argmax(area)) if len(cand) else -1
        seeds.append(cand[j] if j >= 0 else int(sites[0]))
    return seeds


def tracks(plateaus, levels, n, obj, void, objects, seeds):
    """État de chaque branche suivie, par plateau où il change : niveau, IoU, état, masque des objets réunis."""
    rp = Replay(n, obj, void, objects)
    out = [[] for _ in range(objects)]
    last = [None] * objects
    matched_once = [False] * objects
    best = [0.0] * objects
    fusions = []
    prev_mask = [1 << o for o in range(objects)]
    for p, (_, events) in enumerate(plateaus):
        rp.apply(events)
        was_matched = list(matched_once)  # état avant ce plateau
        roots = [rp.find(s) if rp.entered[s] else -1 for s in seeds]
        sizes = [int(rp.size[r]) if r >= 0 else 0 for r in roots]
        for o in range(objects):
            r = roots[o]
            mask = sum(1 << q for q in range(objects) if roots[q] == r and r >= 0 and sizes[q] >= 2)
            if r < 0 or sizes[o] < 2:
                state, iou, mask = NONE, 0.0, 1 << o
            else:
                iou = rp.iou(r, o)
                state = FUSED if mask != (1 << o) else (MATCHED if iou > 0.5 else FRAGMENT)
            best[o] = max(best[o], iou)
            row = (float(levels[p]), round(iou, 6), state, mask, sizes[o])
            if last[o] is None or row[1:] != last[o][1:]:
                out[o].append(row)
                last[o] = row
            if iou > 0.5:
                matched_once[o] = True
        # fusions de branches suivies : un masque qui grossit
        for o in range(objects):
            mask = last[o][3]
            if mask != prev_mask[o] and bin(mask).count('1') > bin(prev_mask[o]).count('1'):
                members = [q for q in range(objects) if mask >> q & 1]
                key = (float(levels[p]), tuple(members))
                if not any(f['r'] == key[0] and f['objects'] == list(key[1]) for f in fusions):
                    fusions.append(dict(r=key[0], objects=list(key[1]), before=[was_matched[q] for q in members]))
            prev_mask[o] = mask
    return out, best, fusions


def method_scene(plateaus, n, obj, void, objects, published, label):
    levels = strictly_increasing([lv for lv, _ in plateaus])
    blocks = best_block_sites(plateaus, levels, n, obj, void, objects)
    got = [round(v, 6) for _, _, v in blocks]
    if got != [round(x, 6) for x in published]:
        raise Refus('%s : meilleurs IoU %s, publiés %s' % (label, got, published))
    seeds = choose_seeds(plateaus, levels, n, obj, void, objects, blocks)
    track, best, fusions = tracks(plateaus, levels, n, obj, void, objects, seeds)
    if [round(b, 6) for b in best] != got:
        raise Refus('%s : la branche suivie ne passe pas par le meilleur bloc (%s contre %s)' % (label, best, got))
    kinds, ia, ib, pl = [], [], [], []
    for p, (_, events) in enumerate(plateaus):
        for ev in events:
            kinds.append(0 if ev[0] == 'enter' else 1)
            ia.append(int(ev[1]))
            ib.append(int(ev[2]) if ev[0] == 'union' else -1)
            pl.append(p)
    return dict(levels=[float(x) for x in levels], ev_plateau=pl, ev_kind=kinds, ev_a=ia, ev_b=ib, seeds=seeds,
                best=got, best_level=[float(levels[p]) for p, _, _ in blocks],
                best_sites=[sites.tolist() for _, sites, _ in blocks], tracks=track, fusions=fusions)


def first_level(track, cond):
    for row in track:
        if cond(row):
            return row[0]
    return None


# ------------------------------------------------------------------ géométrie de la vue

def sensor_offset(entry, variante, crop):
    """Position du capteur dans le repère de la découpe (mm) : la trame officielle redonne le coin de la découpe,
    contrôlé par l'empreinte des sites (mêmes calculs que tools/chercher_bouts.py). None si la trame est
    inaccessible (réseau absent : vue sans capteur)."""
    try:
        import kitti
        from chercher_bouts import crop_sans_sol, quantize
        if variante == 'sans_sol':
            q, _, _ = crop_sans_sol(entry, crop['marge'])
        else:
            xyzi, label, _ = kitti.load_frame(entry['seq'], entry['frame'], entry['velodyne_sha256'],
                                              entry['labels_sha256'])
            idx = np.concatenate([np.flatnonzero(label == k) for k in entry['keys']])
            idx.sort(kind='stable')
            q = quantize(xyzi[idx, :3])
            _, first = np.unique(q, axis=0, return_index=True)
            first.sort()
            q = q[first]
    except Exception as error:
        print('capteur inconnu pour %s : %s' % (entry['name'], error), file=sys.stderr)
        return None
    corner = q.min(axis=0)
    if hashlib.sha256((q - corner).astype('<u4').tobytes()).hexdigest() != crop['sites_sha256']:
        raise Refus('la trame officielle ne redonne pas les sites de la découpe')
    return -corner.astype(np.float64)


def frame_points(xyz_mm, sensor_mm, obj):
    """Mètres ; x et y centrés sur la boîte des objets suivis, leur point le plus bas à z = 0 ; capteur dans le même
    repère."""
    p = xyz_mm.astype(np.float64) / 1000.0
    o = p[obj >= 0] if np.any(obj >= 0) else p
    shift = np.r_[(o[:, :2].min(axis=0) + o[:, :2].max(axis=0)) / 2, o[:, 2].min()]
    sensor = None if sensor_mm is None else sensor_mm / 1000.0 - shift
    return p - shift, sensor


def screen_basis(az, el):
    """Axes écran (droite, haut) de la caméra du lecteur (player/duel.js : az = 0, caméra du côté y < 0)."""
    a, e = math.radians(az), math.radians(el)
    f = -np.array([math.cos(e) * math.sin(a), -math.cos(e) * math.cos(a), math.sin(e)])
    right = np.cross(f, [0.0, 0.0, 1.0])
    right /= np.linalg.norm(right)
    return right, np.cross(right, f)


def overlap(q, obj, objects, az, el):
    """Chevauchement à l'écran : pour chaque paire d'objets, part des cellules (2 % de l'étendue) occupées par les
    deux, rapportée au plus petit ; rend le pire."""
    right, up = screen_basis(az, el)
    u, v = q @ right, q @ up
    cell = 0.02 * max(np.ptp(u), np.ptp(v), 1e-6)
    keys = (np.floor(u / cell).astype(np.int64) << 32) + np.floor(v / cell).astype(np.int64)
    cells = [set(keys[obj == o].tolist()) for o in range(objects)]
    worst = 0.0
    for a in range(objects):
        for b in range(a + 1, objects):
            worst = max(worst, len(cells[a] & cells[b]) / max(1, min(len(cells[a]), len(cells[b]))))
    return worst


def occlusion(q, obj, az, el):
    """Part des points des objets devant lesquels se trouve, dans la même cellule de l'écran (1 % de l'étendue des
    objets), un point du fond plus proche de la caméra d'au moins 5 cm."""
    if not np.any(obj < 0):
        return 0.0
    right, up = screen_basis(az, el)
    a, e = math.radians(az), math.radians(el)
    toward = np.array([math.cos(e) * math.sin(a), -math.cos(e) * math.cos(a), math.sin(e)])  # vers la caméra
    u, v, depth = q @ right, q @ up, -(q @ toward)
    mine = obj >= 0
    cell = 0.01 * max(np.ptp(u[mine]), np.ptp(v[mine]), 1e-6)
    keys = (np.floor(u / cell).astype(np.int64) << 32) + np.floor(v / cell).astype(np.int64)
    front = {}
    for key, d in zip(keys[~mine].tolist(), depth[~mine].tolist()):
        if d < front.get(key, np.inf):
            front[key] = d
    hidden = sum(1 for key, d in zip(keys[mine].tolist(), depth[mine].tolist()) if front.get(key, np.inf) < d - 0.05)
    return hidden / int(np.sum(mine))


def choose_view(q, obj, objects, sensor):
    """Azimut et élévation de la caméra : objets le moins superposés à l'écran sur tout le panoramique (az ± 8°) et le
    moins masqués par le fond, puis caméra du côté du capteur, puis vue la plus basse (la plus « 3D »)."""
    best = None
    mine = obj >= 0
    for el in (22, 30, 40, 52, 65):
        for az in range(0, 360, 4):
            worst = max(overlap(q[mine], obj[mine], objects, az + d, el) for d in (-8, 0, 8))
            hidden = occlusion(q, obj, az, el)
            side = 0.0
            if sensor is not None and np.hypot(sensor[0], sensor[1]) > 1e-6:
                cam = np.array([math.sin(math.radians(az)), -math.cos(math.radians(az))])
                side = (1 - cam @ (sensor[:2] / np.hypot(sensor[0], sensor[1]))) / 2
            score = worst + 0.5 * hidden + 0.15 * side + 0.04 * (el - 22) / 43
            if best is None or score < best[0] - 1e-12:
                best = (score, az, el, worst, hidden)
    return dict(az=best[1], el=best[2], overlap=round(best[3], 3), hidden=round(best[4], 3))


# ------------------------------------------------------------------ calendrier

def events_of(method, m, objects):
    """Événements d'une hiérarchie, au niveau exact de leur plateau : objet retrouvé (première fois que l'IoU de sa
    branche dépasse 1/2, hors groupe réuni), tous les objets retrouvés et encore séparés, fusion de branches."""
    out = []
    first = []
    for o in range(objects):
        row = next((row for row in m['tracks'][o] if row[1] > 0.5), None)
        first.append(row)
        if row is not None and row[2] != FUSED:
            out.append((row[0], 'match:%s:%d' % (method, o)))
    # objets retrouvés et encore séparés : tous, sinon ceux de la première fusion (réunie après les avoir retrouvés)
    groups = [list(range(objects))]
    if m['fusions']:
        groups.append(m['fusions'][0]['objects'])
    for group in groups:
        rows = [first[o] for o in group]
        if len(group) < 2 or not all(rows):
            continue
        r_sep = max(row[0] for row in rows)
        if any(f['r'] <= r_sep and len(set(f['objects']) & set(group)) >= 2 for f in m['fusions']):
            continue
        out = [e for e in out if not (e[0] == r_sep and e[1].startswith('match:'))]
        out.append((r_sep, 'sep:%s:%s' % (method, '+'.join(map(str, group)))))
        break
    for f in m['fusions']:
        out.append((f['r'], 'fusion:%s:%s' % (method, '+'.join(map(str, f['objects'])))))
    return out


HOLD = dict(match=1.4, sep=2.8, fusion_bad=3.4, fusion_good=2.6)
HOLD_INTRO = 1.6  # secondes de vérité terrain immobile au début de la vidéo


def listing(objs):
    keys = [LETTERS[o] for o in objs]
    return keys[0] if len(keys) == 1 else ', '.join(keys[:-1]) + ' et ' + keys[-1]


def iou_text(v):
    """Deux décimales, davantage près du seuil : 0,502 > 0,5 ne doit pas se lire « 0,50 » (même règle que le lecteur)."""
    d = 2
    while d < 6 and v != 0.5 and round(v, d) == 0.5:
        d += 1
    return ('%.*f' % (d, v)).replace('.', ',')


def row_at(track, r):
    """Ligne de suivi en vigueur au niveau r (coupe fermée)."""
    out = None
    for row in track:
        if row[0] <= r:
            out = row
        else:
            break
    return out


def fused_groups(m, objs, r):
    """Groupes d'objets de objs dont les branches suivies sont réunies au niveau r."""
    groups = {}
    for o in objs:
        row = row_at(m['tracks'][o], r)
        if row is not None and row[2] != NONE:
            key = row[3] & sum(1 << q for q in objs)
            groups.setdefault(key, []).append(o)
    return [g for g in groups.values() if len(g) > 1]


def separate(m, objs, r):
    """Vrai si chaque objet de objs a sa branche au niveau r et qu'aucune n'est réunie à une autre de objs."""
    mask = sum(1 << q for q in objs)
    for o in objs:
        row = row_at(m['tracks'][o], r)
        if row is None or row[2] == NONE or row[3] & mask != 1 << o:
            return False
    return True


def badges(scene, pause):
    """Bandeaux d'une pause, par colonne : bordure et morceaux de texte [texte, rôle de couleur, gras]. Le lecteur
    les affiche tels quels ; les README en tirent le tableau des événements (une seule source de texte)."""
    out = dict(hgp=[], hdbscan=[])
    r = pause['r']
    for role in pause['roles']:
        kind, method, what = role.split(':')
        m = scene['methods'][method]
        other = 'hdbscan' if method == 'hgp' else 'hgp'
        mo = scene['methods'][other]
        if kind == 'match':
            o = int(what)
            row = row_at(m['tracks'][o], r)
            out[method].append(dict(border='obj%d' % o, parts=[['✓ ', 'ok', True], [LETTERS[o], 'obj%d' % o, True],
                                                               [' retrouvé · IoU ' + iou_text(row[1]), 'text', True]]))
        elif kind == 'sep':
            group = [int(x) for x in what.split('+')]
            out[method].append(dict(border='ok', parts=[['✓ ', 'ok', True],
                                                        [listing(group) + ' retrouvés, encore séparés', 'text', True]]))
            for objs in fused_groups(mo, group, r):
                out[other].append(dict(border='fusion', parts=[['✗ ', 'fusion', True],
                                                               [listing(objs) + ' déjà réunis', 'text', True]]))
        else:
            objs = [int(x) for x in what.split('+')]
            f = next(f for f in m['fusions'] if f['r'] == r and f['objects'] == objs)
            if all(f['before']):
                out[method].append(dict(border='ok', parts=[['✓ ', 'ok', True],
                                                            [listing(objs) + ' réunis, chacun retrouvé avant', 'text', True]]))
                continue
            never = [o for o in objs if m['best'][o] <= 0.5]  # aucun bloc d'IoU > 1/2, à aucun niveau
            late = [o for j, o in enumerate(objs) if not f['before'][j]]  # retrouvé après la fusion seulement
            shown = never or late
            why = '%s %s retrouvé%s' % (listing(shown), 'jamais' if never else 'pas encore', 's' if len(shown) > 1 else '')
            out[method].append(dict(border='fusion', parts=[['✗ ', 'fusion', True], [listing(objs) + ' réunis : ', 'text', True],
                                                            [why, 'fusion', True]]))
            if separate(mo, objs, r):
                out[other].append(dict(border='dim', parts=[[listing(objs) + ' encore séparés', 'text', True]]))
    return out


def badge_text(b):
    return ''.join(part[0] for part in b['parts']).strip()


def schedule(scene):
    """Temps -> niveau : introduction, balayage log-linéaire ralenti à l'approche de chaque pause, une pause par
    niveau d'événement (les deux colonnes s'arrêtent ensemble), conclusion."""
    hgp, hdb = scene['methods']['hgp'], scene['methods']['hdbscan']
    objects = len(scene['objects'])
    events = events_of('hgp', hgp, objects) + events_of('hdbscan', hdb, objects)
    levels = sorted(set(r for r, _ in events))
    holds = []
    for r in levels:
        roles = [role for rr, role in events if rr == r]
        hold = 0.0
        for role in roles:
            kind = role.split(':')[0]
            if kind == 'fusion':
                method, what = role.split(':')[1:]
                f = next(f for f in scene['methods'][method]['fusions'] if f['r'] == r and
                         '+'.join(map(str, f['objects'])) == what)
                hold = max(hold, HOLD['fusion_good'] if all(f['before']) else HOLD['fusion_bad'])
            else:
                hold = max(hold, HOLD[kind])
        holds.append((r, hold, roles))
    appear = [first_level(m['tracks'][o], lambda row: row[2] != NONE) for m in (hgp, hdb) for o in range(objects)]
    r0 = 0.75 * min(a for a in appear if a)
    r1 = 1.25 * max(levels) if levels else 8 * r0
    # introduction : la vérité terrain immobile (HOLD_INTRO s, objets en couleur, nommés), puis une courte orbite
    intro, outro = 4.4, 5.0
    rate = math.log(r1 / r0) / 17.0  # environ 17 s de balayage hors pauses
    t = intro + 0.6
    sched = [[0.0, r0], [t, r0]]
    pauses, prev = [], r0
    for r, hold, roles in holds:
        t += max(0.6, math.log(r / prev) / rate)
        sched.append([round(t, 4), r])
        pauses.append(dict(t0=round(t, 4), t1=round(t + hold, 4), r=r, roles=roles))
        t += hold
        sched.append([round(t, 4), r])
        prev = r
    t += max(0.8, math.log(r1 / prev) / rate)
    sched.append([round(t, 4), r1])
    summary = t + 0.4
    # image fixe : HGP retrouve tous les objets, encore séparés, quand HDBSCAN les a déjà réunis ; sinon la fusion
    # trop précoce de HDBSCAN
    for p in pauses:
        p['badges'] = badges(scene, p)
    def first(test):
        return next((p for p in pauses if any(test(r) for r in p['roles'])), None)

    def bad(role):  # fusion d'objets dont l'un n'avait pas encore été retrouvé
        kind, method, what = role.split(':')
        return kind == 'fusion' and not all(next(f for f in scene['methods'][method]['fusions'] if
                                                 '+'.join(map(str, f['objects'])) == what)['before'])
    key = first(lambda r: r.startswith('sep:hgp')) or first(lambda r: r.startswith('fusion:hdbscan') and bad(r)) \
        or first(bad) or first(lambda r: r.startswith('sep:')) or (pauses[-1] if pauses else None)
    t_key = round((key['t0'] + key['t1']) / 2, 4) if key else round(summary - 1.0, 4)
    return dict(intro=intro, hold=HOLD_INTRO, sweep=intro + 0.6, summary=round(summary, 4),
                duration=round(summary + outro, 4), schedule=sched, pauses=pauses, rmin=r0, rmax=r1, key=t_key)


# ------------------------------------------------------------------ principal

def build(args, folder):
    spec = json.loads((folder / 'bout.json').read_text())
    if spec.get('schema') != 'ehgp.zoltan.bout_variante.v1':
        raise Refus('bout.json de variante attendu (tools/choisir_exemples.py)')
    variante, entry, crop = spec['variante'], spec['bout'], spec['decoupe']
    k = int(args.k or spec['ordre_montre'])
    data = folder / 'data'
    sites = data / (entry['name'] + '_sites.u32le')
    labels = data / (entry['name'] + '_labels.u32le')
    if not sites.is_file() and args.data:
        sites, labels = args.data / sites.name, args.data / labels.name
    if sha256(sites) != crop['sites_sha256'] or sha256(labels) != crop['crop_labels_sha256']:
        raise Refus('empreintes des points ou des étiquettes de la découpe')
    xyz = np.fromfile(sites, dtype='<u4').reshape(-1, 3).astype(np.int64)
    raw = np.fromfile(labels, dtype='<u4').astype(np.int64)
    obj, void = objects_of(raw, entry['keys'])
    objects, n = len(entry['keys']), len(xyz)
    t0 = time.monotonic()
    tower, report = export_native(args.export, xyz, args.workers)
    ids = tower['ids']
    hanging = prad.hang_margin_radius(tower['orders'][k], k + 1, 'margin_r')
    published = spec['orders'][str(k)]
    hgp = method_scene(hgp_plateaus(hanging, ids), n, obj, void, objects, published['hgp'], 'HGP')
    tree = ph.hdbscan_tree(xyz, k)
    hdb = method_scene(hdbscan_plateaus(tree, n), n, obj, void, objects, published['hdbscan'], 'HDBSCAN')
    if args.members and variante == 'instances' and (args.members / (entry['name'] + '.json')).is_file():
        g4 = json.loads((args.members / (entry['name'] + '.json')).read_text())['orders'][str(k)]['members']
        for o in range(objects):
            for mine, theirs, label in ((hgp, g4['margin_r'], 'HGP'), (hdb, g4['hdbscan'], 'HDBSCAN')):
                if sorted(mine['best_sites'][o]) != sorted(theirs[o]):
                    raise Refus('%s : meilleur bloc de %s différent de celui de G4' % (label, LETTERS[o]))
    seconds = time.monotonic() - t0
    q, sensor = frame_points(xyz, sensor_offset(entry, variante, crop), obj)
    view = choose_view(q, obj, objects, sensor)
    objs = []
    for o in range(objects):
        sel = obj == o
        objs.append(dict(key=LETTERS[o], name=FR.get(entry['classes'][o], entry['classes'][o]),
                         points=int(np.sum(sel & ~void)), center=[round(float(v), 4) for v in q[sel].mean(axis=0)],
                         top=round(float(q[sel, 2].max()), 4),
                         box=[round(float(v), 4) for v in np.r_[q[sel].min(axis=0), q[sel].max(axis=0)]]))
    if variante == 'sans_sol':
        detail = 'sol retiré automatiquement (Patchwork++) · %s points, dont %s des objets' % (
            thousands(n), thousands(int(np.sum(obj >= 0))))
    else:
        detail = 'instances de la vérité terrain seules · %s points' % thousands(n)
    scene = dict(
        schema='ehgp.zoltan.duel.v2', variante=variante,
        meta=dict(title='%s · SemanticKITTI %s/%s' % (title(entry['classes']), entry['seq'], entry['frame']),
                  subtitle='même ordre k = %d pour les deux hiérarchies · %s' % (k, detail),
                  k=k, exemple=folder.parent.name, sites=n, gaps=entry['gaps']),
        objects=objs, gt=obj.tolist(), void=void.astype(int).tolist(),
        sensor=None if sensor is None else [round(float(v), 3) for v in sensor], view=view,
        points=dict(x=[round(float(v), 4) for v in q[:, 0]], y=[round(float(v), 4) for v in q[:, 1]],
                    z=[round(float(v), 4) for v in q[:, 2]]),
        methods=dict(hgp=hgp, hdbscan=hdb))
    scene['timing'] = schedule(scene)
    data.mkdir(exist_ok=True)
    (data / ('duel_k%d.js' % k)).write_text('window.DUEL_SCENE = ' + json.dumps(scene, separators=(',', ':')) + ';\n')
    result = dict(
        schema='ehgp.zoltan.resultats_duel.v2', exemple=folder.parent.name, variante=variante, bout=entry['name'],
        k=k, sites=n,
        hgp=dict(regle='H^r_{k+1} (morsehgp3D_v11, bench/points_radius.py, margin_r)', export=dict(
            status=report.get('status'), levels=report.get('levels'), nodes=report['orders'][ORDERS.index(k)]['nodes']
            if 'orders' in report else None)),
        hdbscan=dict(regle='scikit-learn HDBSCAN(min_samples=k), arbre du lien simple ; niveau = distance / 2'),
        convention='r = rayon (HGP) ; r = distance d\'atteignabilité mutuelle / 2 (HDBSCAN), convention de la thèse',
        vue=view,
        methods={name: dict(best=m['best'], best_level_m=m['best_level'], seeds=m['seeds'], fusions=m['fusions'],
                            tracks=m['tracks'], plateaus=len(m['levels']))
                 for name, m in (('hgp', hgp), ('hdbscan', hdb))},
        timing={key: scene['timing'][key] for key in ('duration', 'pauses', 'rmin', 'rmax', 'key')})
    (folder / ('resultats_duel_k%d.json' % k)).write_text(json.dumps(result, indent=1, ensure_ascii=False) + '\n')
    return dict(folder='%s/%s' % (folder.parent.name, folder.name), k=k, n=n, seconds=round(seconds, 2),
                hgp=hgp['best'], hdbscan=hdb['best'], fusions=dict(hgp=hgp['fusions'], hdbscan=hdb['fusions']),
                view=view, duration=scene['timing']['duration'])


def thousands(n):
    """Entier avec espace fine insécable des milliers (typographie française)."""
    return '{:,}'.format(int(n)).replace(',', '\u202f')


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('examples', nargs='+', type=Path)
    p.add_argument('--export', type=Path, required=True, help='binaire mhgp11_points_export de morsehgp3D_v11')
    p.add_argument('--data', type=Path, help='dossier des points des bouts, si EXEMPLE/data/ est vide')
    p.add_argument('--members', type=Path, help='résultats --members-all de la session G4 (contrôle des blocs)')
    p.add_argument('--k', type=int)
    p.add_argument('--workers', type=int, default=2)
    args = p.parse_args()
    code = 0
    for folder in args.examples:
        try:
            print(json.dumps(build(args, folder), ensure_ascii=False), flush=True)
        except Refus as error:
            print('refus %s : %s' % (folder.name, error), file=sys.stderr, flush=True)
            code = 1
    return code


if __name__ == '__main__':
    raise SystemExit(main())
