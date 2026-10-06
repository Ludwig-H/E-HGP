#!/usr/bin/env python3
"""Scène d'une vidéo « hiérarchie des supports » : l'arbre d'ordre k de morsehgp3D_v11 (sortie supports, format
MHGP11SP), dont chaque boule publie ses supports positifs minimaux : arêtes (q2), triangles (q3), tétraèdres (q4).

    python3 Zoltan/demos/tools/supports_scene.py --mhgp11 BIN [--k 5] [--travail DIR] <variante> [<variante> ...]

<variante> : videos_hgp_hdbscan/<exemple>/{instances,sans_sol}, dont la scène du duel (data/duel_k<k>.js, écrite par
tools/duel_scene.py) fournit les points, la vérité terrain, la caméra et les objets. Écrit data/supports_k<k>.js (non
versionné : coordonnées) et resultats_supports_k<k>.json.

- Supports : `mhgp11 --sortie=supports --k=<k>` sur les points de la découpe (PointId = rang dans la découpe), lu et
  contrôlé par morsehgp3D_v11/bench/mhgp11_formats.py (read_supports). Niveau d'une boule : rayon de la sphère de son
  premier support S*, en mètres (grille de 1 mm). Un support apparaît au niveau de sa boule.
- Réalisation d'un nœud v au niveau r : les sites des supports des boules rattachées à son sous-arbre, de niveau au
  plus r (docs/SORTIES.md § 6, « Lectures »). Les sites intérieurs n'y sont pas ; les IoU se comptent sur ces sites,
  points void exclus.
- Nœud qui suit un objet : le nœud de meilleur IoU (complet, juste avant la naissance de son parent) ; en dessous,
  l'enfant de meilleur IoU à chaque étage ; au-dessus, ses ancêtres. Au niveau r, la branche de l'objet est le nœud de
  cette chaîne vivant à r. Deux objets dont les branches sont le même nœud sont réunis.
- Pauses (tools/duel_scene.events_of, mêmes règles que les vidéos du duel) : IoU maximal de chaque objet reconnu,
  objets reconnus et encore séparés, effondrement de l'IoU (fond absorbé), fusion de branches.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import time

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'morsehgp3D_v11' / 'bench'))
import mhgp11_formats as fm  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
import duel_scene as ds  # noqa: E402

NONE, FRAGMENT, MATCHED, FUSED = ds.NONE, ds.FRAGMENT, ds.MATCHED, ds.FUSED
HOLD = dict(best=2.0, sep=2.8, chute=2.8, fusion_bad=3.4, fusion_good=2.6)


class Refus(Exception):
    pass


def load_duel(folder, k):
    path = folder / 'data' / ('duel_k%d.js' % k)
    if not path.is_file():
        raise Refus('scène du duel absente : %s (tools/duel_scene.py)' % path)
    text = path.read_text(encoding='utf-8')
    return json.loads(text[text.index('=') + 1:].strip().rstrip(';'))


def run_supports(binary, xyz, workdir, k):
    """Sortie supports de la v11 sur la découpe ; rend le fichier lu et contrôlé, et le manifeste."""
    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True)
    points, ids = workdir / 'points.u32le', workdir / 'ids.u32le'
    xyz.astype('<u4').tofile(points)
    np.arange(len(xyz), dtype='<u4').tofile(ids)
    out = workdir / 'sortie'
    run = subprocess.run([str(binary), '--sortie=supports', '--points=%s' % points, '--ids=%s' % ids,
                          '--dossier=%s' % out, '--k=%d' % k, '--fils=4'], capture_output=True, text=True)
    status = json.loads(run.stdout.strip().splitlines()[-1]) if run.stdout.strip() else {}
    if run.returncode != 0 or status.get('status') != 'ok':
        raise Refus('mhgp11 --sortie=supports : code %d, %s' % (run.returncode, status.get('reason', run.stderr[-300:])))
    bits = status['coord_bits']
    fm.check_directory(str(out), bits)
    data = (out / 'supports.mhgp11sp').read_bytes()
    manifest = json.loads((out / 'manifeste.json').read_text(encoding='utf-8'))
    return fm.read_supports(data, bits), manifest, status, hashlib.sha256(data).hexdigest()


def analyse(f, gt, void, raw, objects):
    """Chaînes, lignes de suivi, fusions et effondrements des branches qui suivent les objets."""
    crop = np.array(f.point_id, dtype=np.int64)
    order = sorted(range(f.N), key=lambda v: f.post[v])
    start, at = [0] * f.N, 0
    for v in order:
        start[v] = at
        at += f.ball_count[v]
    radius = [math.sqrt(float(f.level(b))) / 1000.0 for b in range(f.B)]  # rayon de S*, mètres
    level = [radius[f.first_ball[v]] if f.ball_count[v] else 0.0 for v in range(f.N)]
    ball_sites = []
    for b in range(f.B):
        s = set()
        for q in range(f.ball_at[b], f.ball_at[b] + f.support_count[b]):
            s.update(int(crop[x]) for x in f.points_of(q))
        ball_sites.append(s)
    sizes = [int(np.sum((gt == o) & ~void)) for o in range(objects)]

    def iou_of(sites):
        arr = np.fromiter(sites, dtype=np.int64, count=len(sites))
        arr = arr[~void[arr]] if len(arr) else arr
        out = []
        for o in range(objects):
            inter = int(np.sum(gt[arr] == o)) if len(arr) else 0
            union = len(arr) + sizes[o] - inter
            out.append(inter / union if union else 0.0)
        return out

    # réalisation complète de chaque nœud (petits ensembles versés dans les grands, en postordre) et son IoU
    full_iou, sets = [None] * f.N, {}
    for v in order:
        ch = f.children[v]
        if ch:
            big = max(ch, key=lambda c: len(sets[c]))
            s = sets.pop(big)
            for c in ch:
                if c != big:
                    s |= sets.pop(c)
        else:
            s = set()
        for b in range(start[v], start[v] + f.ball_count[v]):
            s |= ball_sites[b]
        sets[v] = s
        full_iou[v] = iou_of(s)
    sets.clear()
    chains = []
    for o in range(objects):
        best = max(range(f.N), key=lambda v: (full_iou[v][o], -level[v]))
        down, v = [], best
        while f.children[v]:
            v = max(f.children[v], key=lambda c: (full_iou[c][o], -level[c]))
            down.append(v)
        up, v = [best], best
        while v != f.root:
            v = f.parent[v]
            up.append(v)
        chains.append(list(reversed(down)) + up)
    # moments : boules propres des nœuds des chaînes, par niveau
    on_chain = {}
    for o, chain in enumerate(chains):
        for i, v in enumerate(chain):
            on_chain.setdefault(v, []).append((o, i))
    moments = sorted(set(radius[b] for v in on_chain for b in range(start[v], start[v] + f.ball_count[v])))

    def node_at(o, r):
        cur = None
        for v in chains[o]:
            if level[v] <= r:
                cur = v
            else:
                break
        return cur

    def realization(o, v, r):
        """Sites de la réalisation du nœud v au niveau r, tenue à jour par objet : au changement de nœud, tout le
        sous-arbre des enfants (leurs boules sont de niveau inférieur à celui de v) ; ensuite, les boules propres de v
        de niveau au plus r, dans leur ordre (rang croissant)."""
        st = held[o]
        if st['node'] != v:
            first = order[f.post[v] - f.size[v] + 1]
            s = set()
            for b in range(start[first], start[v]):
                s |= ball_sites[b]
            st.update(node=v, sites=s, next=start[v])
        end = start[v] + f.ball_count[v]
        while st['next'] < end and radius[st['next']] <= r:
            st['sites'] |= ball_sites[st['next']]
            st['next'] += 1
        return st['sites']

    sem = np.asarray(raw, dtype=np.int64) & 0xFFFF
    rows = [[] for _ in range(objects)]
    real = [set() for _ in range(objects)]
    held = [dict(node=None, sites=set(), next=0) for _ in range(objects)]
    fusions, seen_groups, found = [], set(), [False] * objects
    chutes = []
    for r in moments:
        nodes = [node_at(o, r) for o in range(objects)]
        groups = {}
        for o, v in enumerate(nodes):
            if v is not None:
                groups.setdefault(v, []).append(o)
        was_found = list(found)
        for v, members in groups.items():
            if len(members) > 1 and (v, tuple(members)) not in seen_groups:
                key = tuple(members)
                if not any(set(key) == set(g) for _, g in seen_groups):
                    fusions.append(dict(r=r, objects=list(key), before=[was_found[q] for q in key]))
                seen_groups.add((v, key))
        for o, v in enumerate(nodes):
            if v is None:
                continue
            sites = set(realization(o, v, r))
            iou = iou_of(sites)[o]
            mask = sum(1 << q for q in groups[v])
            state = FUSED if len(groups[v]) > 1 else (MATCHED if iou > 0.5 else FRAGMENT)
            prev = rows[o][-1] if rows[o] else None
            row = [r, round(iou, 6), state, mask, len(sites)]
            if prev and prev[1:4] == row[1:4]:
                real[o] = sites
                continue
            if prev and iou <= prev[1] - 0.10 and iou <= prev[1] * 0.75:
                new = np.fromiter(sites - real[o], dtype=np.int64)
                new = new[~void[new]] if len(new) else new
                others = {q: int(np.sum(gt[new] == q)) for q in range(objects) if q != o and np.any(gt[new] == q)}
                back = new[gt[new] < 0] if len(new) else new
                names, counts = np.unique(sem[back], return_counts=True)
                chutes.append(dict(r=r, objet=o, avant=prev[1], apres=row[1],
                                   fusion=bin(mask).count('1') > bin(prev[3]).count('1'), objets=others,
                                   fond=int(len(back)), classe=ds.class_name(int(names[np.argmax(counts)]))
                                   if len(names) else None))
            rows[o].append(row)
            real[o] = sites
            if iou > 0.5:
                found[o] = True
    best = [max((row[1] for row in rows[o]), default=0.0) for o in range(objects)]
    best_full = [max(full_iou[v][o] for v in range(f.N)) for o in range(objects)]
    return dict(order=order, start=start, radius=radius, level=level, chains=chains, tracks=rows, fusions=fusions,
                chutes=chutes, best=best, best_node=best_full)


def badges(m, pause):
    """Bandeaux d'une pause (une colonne), mêmes textes que les vidéos du duel."""
    out, r, collapsed = [], pause['r'], {}
    L = ds.LETTERS
    for role in pause['roles']:
        kind, _, what = role.split(':')
        if kind == 'best':
            o = int(what)
            row = ds.row_at(m['tracks'][o], r)
            out.append(dict(border='obj%d' % o, parts=[['✓ ', 'ok', True], [L[o], 'obj%d' % o, True],
                                                      [', IoU maximal : ' + ds.iou_text(row[1]), 'text', True]]))
        elif kind == 'sep':
            group = [int(x) for x in what.split('+')]
            out.append(dict(border='ok', parts=[['✓ ', 'ok', True],
                                                [ds.listing(group) + ' retrouvés, encore séparés', 'text', True]]))
        elif kind == 'chute':
            mask = ds.row_at(m['tracks'][int(what)], r)[3]
            collapsed.setdefault(mask, []).append(int(what))
        else:
            objs = [int(x) for x in what.split('+')]
            fu = next(x for x in m['fusions'] if x['r'] == r and x['objects'] == objs)
            if all(fu['before']):
                out.append(dict(border='ok', parts=[['✓ ', 'ok', True],
                                                    [ds.listing(objs) + ' réunis, chacun retrouvé avant', 'text', True]]))
                continue
            never = [o for o in objs if m['best'][o] <= 0.5]
            late = [o for j, o in enumerate(objs) if not fu['before'][j]]
            shown = never or late
            why = '%s %s retrouvé%s' % (ds.listing(shown), 'jamais' if never else 'pas encore', 's' if len(shown) > 1 else '')
            out.append(dict(border='fusion', parts=[['✗ ', 'fusion', True], [ds.listing(objs) + ' réunis : ', 'text', True],
                                                    [why, 'fusion', True]]))
    for objs in collapsed.values():
        c = next(c for c in m['chutes'] if c['r'] == r and c['objet'] == objs[0] and not c['fusion'])
        if c['fond'] >= sum(c['objets'].values()):
            taken = 'avec ' + ds.ABSORBED.get(c['classe'], 'le fond')
        else:
            taken = 'avec une partie de ' + L[max(c['objets'], key=c['objets'].get)]
        if len(objs) == 1:
            out.append(dict(border='fusion', parts=[['✗ ', 'fusion', True], [L[objs[0]], 'obj%d' % objs[0], True],
                                                    [' fusionne %s · IoU %s → %s' % (taken, ds.iou_text(c['avant']),
                                                                                     ds.iou_text(c['apres'])), 'text', True]]))
        else:
            out.append(dict(border='fusion', parts=[['✗ ', 'fusion', True],
                                                    ['%s, déjà réunis, fusionnent %s' % (ds.listing(objs), taken),
                                                     'text', True]]))
    return out


def schedule(m, objects, levels):
    """Un balayage log-linéaire de r, une pause par niveau d'événement, comme les vidéos du duel."""
    events = ds.events_of('hgp', m, objects)
    holds = []
    for r in sorted(set(r for r, _ in events)):
        roles = [role for rr, role in events if rr == r]
        hold = 0.0
        for role in roles:
            kind, _, what = role.split(':')
            if kind == 'fusion':
                fu = next(x for x in m['fusions'] if x['r'] == r and '+'.join(map(str, x['objects'])) == what)
                hold = max(hold, HOLD['fusion_good'] if all(fu['before']) else HOLD['fusion_bad'])
            else:
                hold = max(hold, HOLD[kind])
        holds.append((r, hold, roles))
    appear = [m['tracks'][o][0][0] for o in range(objects) if m['tracks'][o]]
    r0 = 0.75 * min(appear)
    r1 = 1.25 * max(r for r, _, _ in holds) if holds else 8 * r0
    intro, outro = 4.4, 5.0
    rate = math.log(r1 / r0) / 20.0  # environ 20 s de balayage hors pauses
    t = intro + 0.6
    sched, pauses, prev = [[0.0, r0], [t, r0]], [], r0
    for r, hold, roles in holds:
        t += max(0.6, math.log(r / prev) / rate)
        sched.append([round(t, 4), r])
        pause = dict(t0=round(t, 4), t1=round(t + hold, 4), r=r, roles=roles)
        pause['badges'] = badges(m, pause)
        pauses.append(pause)
        t += hold
        sched.append([round(t, 4), r])
        prev = r
    t += max(0.8, math.log(r1 / prev) / rate)
    sched.append([round(t, 4), r1])
    summary = t + 0.4

    def first(test):
        return next((p for p in pauses if any(test(x) for x in p['roles'])), None)
    key = first(lambda x: x.startswith('sep:')) or first(lambda x: x.startswith('best:')) or \
        (pauses[-1] if pauses else None)
    t_key = round((key['t0'] + key['t1']) / 2, 4) if key else round(summary - 1.0, 4)
    return dict(intro=intro, hold=ds.HOLD_INTRO, sweep=intro + 0.6, summary=round(summary, 4),
                duration=round(summary + outro, 4), schedule=sched, pauses=pauses, rmin=r0, rmax=r1, key=t_key)


def build(args, folder):
    spec = json.loads((folder / 'bout.json').read_text(encoding='utf-8'))
    entry, crop = spec['bout'], spec['decoupe']
    k = int(args.k or spec['ordre_montre'])
    duel = load_duel(folder, k)
    data = folder / 'data'
    sites, labels = data / (entry['name'] + '_sites.u32le'), data / (entry['name'] + '_labels.u32le')
    if ds.sha256(sites) != crop['sites_sha256'] or ds.sha256(labels) != crop['crop_labels_sha256']:
        raise Refus('empreintes des points ou des étiquettes de la découpe')
    xyz = np.fromfile(sites, dtype='<u4').reshape(-1, 3).astype(np.int64)
    raw = np.fromfile(labels, dtype='<u4').astype(np.int64)
    gt, void = np.array(duel['gt'], dtype=np.int64), np.array(duel['void'], dtype=bool)
    if len(gt) != len(xyz):
        raise Refus('scène du duel et découpe de tailles différentes')
    objects = len(duel['objects'])
    t0 = time.monotonic()
    work = Path(args.travail) / ('%s_%s_k%d' % (folder.parent.name, folder.name, k))
    f, manifest, status, digest = run_supports(args.mhgp11, xyz, work, k)
    a = analyse(f, gt, void, raw, objects)
    seconds = time.monotonic() - t0
    crop_of = np.array(f.point_id, dtype=np.int64)
    # supports par niveau croissant : le lecteur en dessine un préfixe
    sup = []
    for b in range(f.B):
        v = None
        for q in range(f.ball_at[b], f.ball_at[b] + f.support_count[b]):
            sup.append((a['radius'][b], b, q))
    node_of = f.node_of
    sup.sort()
    lv, pa, ar, flat = [], [], [], []
    for r, b, q in sup:
        lv.append(r)
        pa.append(int(f.post[node_of[b]]))
        ar.append(int(f.arity[q]))
        flat.extend(int(crop_of[x]) for x in f.points_of(q))
    arity = [ar.count(2), ar.count(3), ar.count(4)]
    m = dict(tracks=a['tracks'], fusions=a['fusions'], chutes=a['chutes'], best=a['best'])
    chains = [[[a['level'][v], int(f.post[v]), int(f.size[v])] for v in chain] for chain in a['chains']]
    timing = schedule(m, objects, a['level'])
    meta = dict(duel['meta'])
    detail = meta['subtitle'].split(' · ', 1)[1] if ' · ' in meta['subtitle'] else meta['subtitle']
    meta['subtitle'] = 'arbre d\'ordre k = %d de Morse HGP 3D v11 (sortie supports) · %s' % (k, detail)
    meta['counts'] = dict(nodes=f.N, balls=f.B, supports=f.S, q2=arity[0], q3=arity[1], q4=arity[2])
    scene = dict(schema='ehgp.zoltan.supports.v1', variante=folder.name, meta=meta, objects=duel['objects'],
                 gt=duel['gt'], void=duel['void'], sensor=duel.get('sensor'), view=duel['view'], points=duel['points'],
                 supports=dict(level=lv, post=pa, arity=ar, sites=flat), chains=chains, tracks=a['tracks'],
                 fusions=a['fusions'], chutes=a['chutes'], best=a['best'], timing=timing)
    (data / ('supports_k%d.js' % k)).write_text('window.SUPPORTS_SCENE = ' + json.dumps(scene, separators=(',', ':'))
                                                + ';\n', encoding='utf-8')
    result = dict(
        schema='ehgp.zoltan.resultats_supports.v1', exemple=folder.parent.name, variante=folder.name,
        bout=entry['name'], k=k, sites=len(xyz),
        sortie=dict(commande='mhgp11 --sortie=supports --k=%d' % k, status=status.get('status'),
                    coord_bits=status.get('coord_bits'), manifest_sha256=status.get('manifest_sha256'),
                    supports_sha256=digest, tree_k_sha256=manifest.get('tree_k_sha256'), stages_ns=status.get('stages_ns')),
        counts=meta['counts'],
        regle='branche d\'un objet : nœud de meilleur IoU des sites de ses supports (points void exclus), ses enfants '
              'de meilleur IoU en dessous, ses ancêtres au-dessus',
        best=a['best'], best_node_iou=[round(x, 6) for x in a['best_node']], chains=chains, fusions=a['fusions'],
        chutes=a['chutes'], tracks=a['tracks'],
        timing={key: timing[key] for key in ('duration', 'pauses', 'rmin', 'rmax', 'key')})
    (folder / ('resultats_supports_k%d.json' % k)).write_text(json.dumps(result, indent=1, ensure_ascii=False) + '\n',
                                                              encoding='utf-8')
    return dict(folder='%s/%s' % (folder.parent.name, folder.name), k=k, n=len(xyz), seconds=round(seconds, 2),
                counts=meta['counts'], best=a['best'], hgp=duel['methods']['hgp']['best'],
                duration=timing['duration'])


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('variantes', nargs='+', type=Path)
    p.add_argument('--mhgp11', type=Path, required=True, help='exécutable mhgp11 de morsehgp3D_v11')
    p.add_argument('--k', type=int, default=None)
    p.add_argument('--travail', default=str(ROOT / 'build' / 'v11-persist' / 'videos' / 'supports'))
    args = p.parse_args()
    code = 0
    for folder in args.variantes:
        try:
            print(json.dumps(build(args, folder.resolve()), ensure_ascii=False), flush=True)
        except Refus as error:
            print('refus %s : %s' % (folder, error), file=sys.stderr, flush=True)
            code = 1
    return code


if __name__ == '__main__':
    raise SystemExit(main())
