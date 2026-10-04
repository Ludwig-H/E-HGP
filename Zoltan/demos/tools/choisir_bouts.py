#!/usr/bin/env python3
"""Range les exemples de Zoltan/demos par issue : la hiérarchie HGP réussit ou échoue, HDBSCAN réussit ou échoue.

    python3 Zoltan/demos/tools/choisir_bouts.py --bouts LOT/bouts.json --data LOT/data \
        --results SESSION1/lidar --members SESSION2/lidar --scene-data DONNEES_DES_DEMOS --out Zoltan/demos

Mesure (comme les démos de scène entière) : pour chaque objet, le meilleur IoU atteint par un nœud quelconque de la
hiérarchie, au même ordre k (HDBSCAN : `min_samples` = k, arbre de liaison simple de scikit-learn ; HGP : hiérarchie de
points H^r_{k+1} de morsehgp3D_v11, règle `margin_r`). Critères, fixés avant de lire les résultats :
- à l'ordre k, une méthode RÉUSSIT si tous les objets de l'exemple dépassent 1/2, et ÉCHOUE sinon ;
- catégorie d'un exemple, par priorité : « hgp_reussit_hdbscan_echoue » s'il existe un k où HGP réussit et HDBSCAN
  échoue ; sinon « hgp_echoue_hdbscan_reussit » s'il existe un k où l'inverse se produit ; sinon
  « hgp_echoue_hdbscan_echoue » s'il existe un k où les deux échouent ; sinon « hgp_reussit_hdbscan_reussit ».
Sous-dossiers d'exemples (bouts) : dans les trois premières catégories, tous les bouts, un par trame et par famille (le
plus grand groupe d'abord) ; dans la quatrième, des représentants (objets les plus serrés : cinq groupes de voitures et
la file de la démo 05, trois de vélos, deux de vélos et piétons). Les démos de scène entière sont classées sur leurs
objets suivis (demo.json) ; leur dossier reçoit resultats_hgp.json et des images, et tools/write_readmes.py écrit leur
section HGP.
Images : vue de dessus, trois panneaux (vérité ; meilleur nœud HDBSCAN de l'objet clé ; meilleur nœud HGP du même objet) :
vert = point de l'objet dans le nœud, rouge = point d'un autre objet dans le nœud, bleu = point de l'objet hors du nœud,
gris = autres points. Objet clé : celui que manque la méthode qui échoue (le pire des deux si les deux échouent, le plus
difficile si les deux réussissent). Les images sont des œuvres dérivées de KITTI et SemanticKITTI (CC BY-NC-SA).
"""
import argparse
import json
from pathlib import Path
import shutil
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'morsehgp3D_v11' / 'bench'))
from points_render import png  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
from kitti import FR  # noqa: E402
import duel_readme  # noqa: E402

ORDERS = ('2', '3', '5', '10')
CATEGORIES = ('hgp_reussit_hdbscan_echoue', 'hgp_echoue_hdbscan_reussit', 'hgp_echoue_hdbscan_echoue',
              'hgp_reussit_hdbscan_reussit')
TITLES = {'hgp_reussit_hdbscan_echoue': 'HGP réussit, HDBSCAN échoue',
          'hgp_echoue_hdbscan_reussit': 'HGP échoue, HDBSCAN réussit',
          'hgp_echoue_hdbscan_echoue': 'HGP et HDBSCAN échouent',
          'hgp_reussit_hdbscan_reussit': 'HGP et HDBSCAN réussissent'}
OUTCOME = {'win': 'HGP réussit, HDBSCAN échoue', 'loss': 'HGP échoue, HDBSCAN réussit',
           'both_fail': 'les deux échouent', 'both_ok': 'les deux réussissent'}
WANT = {CATEGORIES[0]: 'win', CATEGORIES[1]: 'loss', CATEGORIES[2]: 'both_fail', CATEGORIES[3]: 'both_ok'}
FAMILIES = {'voitures': 'voitures', 'velos': 'vélos', 'velos_pietons': 'vélos et piétons'}
OBJECT_COLORS = ((31, 119, 180), (255, 127, 14), (148, 103, 189))
KIND = dict(other=(200, 200, 200), fn=(40, 90, 230), fp=(220, 40, 40), tp=(30, 160, 60))
LETTERS = 'ABC'
NUMBER = {1: 'un', 2: 'deux', 3: 'trois'}
WORD = {'bicycle': ('vélo', 'velo'), 'bicyclist': ('cycliste', 'cycliste'), 'person': ('piéton', 'pieton'),
        'car': ('voiture', 'voiture')}
SESSION = 'mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771)'
SESSION_SCENES = 'session G4 `claudebouts2`, commit b72fe8771, scikit-learn 1.7.2'
DEFINITION = {
    CATEGORIES[0]: 'À un même ordre k au moins, la hiérarchie de HDBSCAN (`min_samples` = k) ne contient aucun groupe qui '
                   'recouvre l\'un des objets à plus de la moitié, alors que la hiérarchie de points HGP en contient un pour '
                   'chaque objet.',
    CATEGORIES[1]: 'À un même ordre k au moins, la hiérarchie HGP manque un objet que la hiérarchie de HDBSCAN retrouve, et '
                   'l\'inverse ne se produit à aucun ordre.',
    CATEGORIES[2]: 'À un ordre k au moins, les deux hiérarchies manquent un objet, et aucune ne réussit seule à un autre '
                   'ordre.',
    CATEGORIES[3]: 'À tous les ordres mesurés (k = 2, 3, 5, 10), les deux hiérarchies contiennent un groupe qui recouvre '
                   'chaque objet à plus de la moitié.'}
LEGEND = ('Vue de dessus, tournée selon l\'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur '
          'groupe de HDBSCAN pour l\'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l\'objet dans le '
          'groupe ; rouge : point d\'un autre objet dans le groupe ; bleu : point de l\'objet hors du groupe ; gris : autres '
          'points.')


def fr(x):
    return ('%.2f' % x).replace('.', ',')


def outcome(hdb, hgp):
    hf, gs = min(hdb) <= 0.5, min(hgp) > 0.5
    return {(True, True): 'win', (False, False): 'loss', (True, False): 'both_fail', (False, True): 'both_ok'}[(hf, gs)]


def classify(rows):
    outs = {k: outcome(r['hdbscan'], r['hgp']) for k, r in rows.items()}
    for category in CATEGORIES[:3]:
        if WANT[category] in outs.values():
            return category, outs
    return CATEGORIES[3], outs


def key_orders(category, outs):
    ks = [k for k in ORDERS if outs[k] == WANT[category]]
    return ks, ('5' if '5' in ks else ks[0])


def key_object(category, r):
    hdb, hgp = np.asarray(r['hdbscan']), np.asarray(r['hgp'])
    if category == CATEGORIES[0]:
        return int(np.argmin(hdb))
    if category == CATEGORIES[1]:
        return int(np.argmin(hgp))
    if category == CATEGORIES[2]:
        return int(np.argmin(np.maximum(hdb, hgp)))
    return int(np.argmin(np.minimum(hdb, hgp)))


def groups(classes):
    order = []
    for cl in classes:
        if cl not in order:
            order.append(cl)
    return [(cl, classes.count(cl)) for cl in sorted(order, key=lambda c: (c != 'person', c))]


def slug(classes):
    return '_'.join(WORD.get(cl, (cl, cl))[1] if n == 1 else '%s_%ss' % (NUMBER[n], WORD.get(cl, (cl, cl))[1])
                    for cl, n in groups(classes))


def title(classes):
    parts = []
    for cl, n in groups(classes):
        word = WORD.get(cl, (cl, cl))[0]
        parts.append('un ' + word if n == 1 else '%s %ss' % (NUMBER[n], word))
    text = ' et '.join(parts)
    return text[0].upper() + text[1:]


def frame_xy(xyz):
    """Vue de dessus tournée selon l'axe principal, origine au coin bas gauche."""
    xy = xyz[:, :2] - xyz[:, :2].mean(axis=0)
    _, vectors = np.linalg.eigh(np.cov(xy.T))
    xy = xy @ vectors[:, ::-1]
    return xy - xy.min(axis=0)


def canvas(xy, colors, width, height, margin=14):
    image = np.full((height, width, 3), 255, dtype=np.uint8)
    extent = np.maximum(xy.max(axis=0), 1e-6)
    scale = min((width - 2 * margin) / extent[0], (height - 2 * margin) / extent[1])
    ox, oy = (width - extent[0] * scale) / 2, (height - extent[1] * scale) / 2
    px = (ox + xy[:, 0] * scale).astype(int)
    py = (height - 1 - (oy + xy[:, 1] * scale)).astype(int)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            image[np.clip(py + dy, 0, height - 1), np.clip(px + dx, 0, width - 1)] = colors
    return image


def render(xyz, raw, labels, focus, members_hdb, members_hgp, out, margin_m=None):
    """Trois panneaux ; `focus` : étiquette de l'objet clé ; `margin_m` : fenêtre autour des objets suivis (scène entière)."""
    keep = np.ones(len(xyz), dtype=bool)
    if margin_m is not None:
        tracked = np.isin(raw, labels)
        lo, hi = xyz[tracked, :2].min(axis=0) - margin_m, xyz[tracked, :2].max(axis=0) + margin_m
        keep = np.all((xyz[:, :2] >= lo) & (xyz[:, :2] <= hi), axis=1)
    idx = np.flatnonzero(keep)
    xy = frame_xy(xyz[idx])
    extent = np.maximum(xy.max(axis=0), 1e-6)
    width = 520
    height = int(min(520, max(170, width * extent[1] / extent[0] + 28)))
    truth = np.zeros((len(idx), 3), dtype=np.uint8) + 200
    for j, label in enumerate(labels):
        truth[raw[idx] == label] = OBJECT_COLORS[j % 3]
    panels = [canvas(xy, truth, width, height)]
    inside = raw[idx] == focus
    for members in (members_hdb, members_hgp):
        block = np.zeros(len(xyz), dtype=bool)
        block[members] = True
        block = block[idx]
        colors = np.zeros((len(idx), 3), dtype=np.uint8)
        order = []
        for name, mask in (('other', ~inside & ~block), ('fn', inside & ~block), ('fp', ~inside & block),
                           ('tp', inside & block)):
            colors[mask] = KIND[name]
            order.append(np.flatnonzero(mask))
        sel = np.concatenate(order)  # vert et rouge dessinés au-dessus du gris et du bleu
        panels.append(canvas(xy[sel], colors[sel], width, height))
    gap = np.full((height, 10, 3), 255, dtype=np.uint8)
    png(out, np.concatenate([panels[0], gap, panels[1], gap, panels[2]], axis=1))


def rows_of(result, indices):
    rows = {}
    for k in ORDERS:
        o = result['orders'][k]
        hdb = [o['hdbscan']['best'][j] for j in indices]
        hgp = [o['margin_r']['best'][j] for j in indices]
        rows[k] = dict(hdbscan=hdb, hgp=hgp, hdbscan_mean=sum(hdb) / len(hdb), hgp_mean=sum(hgp) / len(hgp))
    return rows


def table(rows, n, outs):
    out = ['| k | HDBSCAN, meilleur IoU par objet (%s) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |' % (
        ' / '.join(LETTERS[:n])), '| --- | --- | --- | --- | --- |']
    for k in ORDERS:
        r = rows[k]
        out.append('| %s | %s | %s | %s / %s | %s |' % (
            k, ' / '.join(('**%s**' % fr(x)) if x <= 0.5 else fr(x) for x in r['hdbscan']),
            ' / '.join(('**%s**' % fr(x)) if x <= 0.5 else fr(x) for x in r['hgp']),
            fr(r['hdbscan_mean']), fr(r['hgp_mean']), OUTCOME[outs[k]]))
    return out


def example_readme(folder, category, entry, rows, outs, images, where=None):
    letters = {key: LETTERS[j] for j, key in enumerate(entry['keys'])}
    out = ['# %s (trame %s/%s)' % (title(entry['classes']), entry['seq'], entry['frame']), '',
           'Catégorie : [%s](../README.md). Bout de scène SemanticKITTI, séquence %s, trame %s, réduit aux seuls points de '
           'ses objets : ni sol, ni fond, ni autre objet ; %s.' % (TITLES[category], entry['seq'], entry['frame'], SESSION),
           '', '| objet | classe | points |', '| --- | --- | --- |']
    for j, (cl, pts) in enumerate(zip(entry['classes'], entry['points'])):
        out.append('| %s | %s | %d |' % (LETTERS[j], FR.get(cl, cl), pts))
    gaps = ', '.join('%s–%s : %s m' % (letters[int(a)], letters[int(b)], fr(g))
                     for (a, b), g in ((tuple(p.split('-')), g) for p, g in sorted(entry['gaps'].items())))
    out += ['', 'Écarts (plus courte distance entre les points de deux objets) : %s. Sites au millimètre : %d.' % (
        gaps, entry['sites']), ''] + table(rows, len(entry['keys']), outs)
    out += ['', 'En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.', '']
    video = duel_readme.bout_section(where) if where is not None else []  # vidéos HGP contre HDBSCAN (render_duel.cjs)
    out += (video + ['']) if video else []
    out += ['## Images', '', LEGEND, '']
    for k, (image, obj) in sorted(images.items(), key=lambda x: int(x[0])):
        out += ['k = %s, objet clé %s :' % (k, LETTERS[obj]), '', '![k = %s](%s)' % (k, image), '']
    out += ['## Données', '',
            '`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, '
            'instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : '
            '`data/` est ignoré par git. Pour les refaire à l\'identique depuis les archives officielles :', '', '```sh',
            'python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/%s/%s \\' % (category, folder),
            '    --rebuild Zoltan/demos/%s/%s/bout.json' % (category, folder), '```', '']
    return '\n'.join(out)


def category_readme(category, counts, scenes, examples, where=None):
    lines = ['# %s' % TITLES[category], '', DEFINITION[category], '']
    here = [s for s in scenes if s[4] == category]
    if here:
        lines += ['## Démos de scène entière', '',
                  '| démo | trame | objets suivis | issue par ordre (k = 2 / 3 / 5 / 10) | pire objet HDBSCAN / HGP à k = 5 |',
                  '| --- | --- | --- | --- | --- |']
        for demo, spec, rows, outs, _ in here:
            lines.append('| [%s](%s/README.md) | %s/%s | %s | %s | %s / %s |' % (
                spec['title'].split(' :')[0], demo.name, spec['seq'], spec['frame'], spec.get('catalogue', ''),
                ' / '.join(OUTCOME[outs[k]] for k in ORDERS), fr(min(rows['5']['hdbscan'])), fr(min(rows['5']['hgp']))))
        lines.append('')
    total = sum(counts[category].values())
    lines += ['## Bouts de scène', '',
              'Parmi les 360 bouts mesurés, %d tombent dans cette catégorie (%s). %s' % (
                  total, ', '.join('%d de %s' % (v, FAMILIES[k]) for k, v in sorted(counts[category].items())),
                  'Chacun a un sous-dossier, à raison d\'un par trame et par famille (le plus grand groupe d\'abord).'
                  if category != CATEGORIES[3] else
                  'Seuls des représentants ont un sous-dossier : les objets les plus serrés de chaque famille et la file de '
                  'trois voitures de la démo 05, isolée. La liste complète est dans '
                  '[`../bouts_evalues.json`](../bouts_evalues.json).'), '',
              '| exemple | trame | objets (points) | écart | ordres concernés | k montré : pire objet HDBSCAN / HGP | IoU moyen HDBSCAN / HGP |',
              '| --- | --- | --- | --- | --- | --- | --- |']
    for folder, e, rows, outs, shown in sorted(examples, key=lambda x: x[0]):
        r = rows[shown]
        lines.append('| [%s](%s/README.md) | %s/%s | %s | %s m | %s | k = %s : %s / %s | %s / %s |' % (
            title(e['classes']), folder, e['seq'], e['frame'],
            ', '.join('%s %s (%d)' % (LETTERS[j], FR.get(cl, cl), n) for j, (cl, n) in enumerate(zip(e['classes'], e['points']))),
            fr(min(e['gaps'].values())), ', '.join(k for k in ORDERS if outs[k] == WANT[category]), shown,
            fr(min(r['hdbscan'])), fr(min(r['hgp'])), fr(r['hdbscan_mean']), fr(r['hgp_mean'])))
    video = duel_readme.category_section(where) if where is not None else []
    lines += [''] + (video + [''] if video else [])
    lines += ['Critères, mesure et légende des images : [README de `demos/`](../README.md).', '']
    return '\n'.join(lines)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--bouts', type=Path, required=True, help='définitions de tous les bouts mesurés')
    p.add_argument('--data', type=Path, required=True, help='points des bouts')
    p.add_argument('--results', type=Path, required=True, help='résultats de tous les bouts (session 1)')
    p.add_argument('--members', type=Path, required=True, help='résultats --members-all (session 2)')
    p.add_argument('--scene-data', type=Path, required=True, help='points des démos de scène entière')
    p.add_argument('--out', type=Path, required=True, help='Zoltan/demos')
    args = p.parse_args()
    bouts = {e['name']: e for e in json.loads(args.bouts.read_text())['bouts']}
    every, counts = [], {c: {} for c in CATEGORIES}
    for f in sorted(args.results.glob('*.json')):
        result = json.loads(f.read_text())
        entry = bouts.get(result.get('name'))
        if entry is None or result.get('status') != 'ok':
            continue
        category, outs = classify(rows_of(result, range(len(entry['keys']))))
        counts[category][entry['kind']] = counts[category].get(entry['kind'], 0) + 1
        every.append(dict(entry, category=category, outcomes=outs,
                          orders={k: dict(hdbscan=result['orders'][k]['hdbscan']['best'],
                                          hgp=result['orders'][k]['margin_r']['best'],
                                          cover=result['orders'][k].get('cover', {}).get('best'),
                                          core=result['orders'][k].get('core', {}).get('best')) for k in ORDERS}))
    (args.out / 'bouts_evalues.json').write_text(json.dumps(dict(
        schema='ehgp.zoltan.bouts_evalues.v2', criteres=__doc__.split('Images')[0], comptes=counts, bouts=every),
        indent=1, ensure_ascii=False) + '\n')
    members = {}
    for f in sorted(args.members.glob('*.json')):
        result = json.loads(f.read_text())
        if result.get('status') == 'ok':
            members[result['name']] = result
    index = {c: [] for c in CATEGORIES}
    for e in every:  # exemples à sous-dossier : ceux de la session à blocs publiés
        result = members.get(e['name'])
        if result is None:
            continue
        rows = rows_of(result, range(len(e['keys'])))
        category, outs = classify(rows)
        if category != e['category']:
            raise SystemExit('catégorie instable entre les deux sessions : ' + e['name'])
        folder = 'bout_%s_%s_%s_%s' % (e['seq'], e['frame'], slug(e['classes']), '_'.join(str(k >> 16) for k in e['keys']))
        where = args.out / category / folder
        where.mkdir(parents=True, exist_ok=True)
        ks, shown = key_orders(category, outs)
        xyz = np.fromfile(args.data / (e['name'] + '_sites.u32le'), dtype='<u4').reshape(-1, 3).astype(np.float64) / 1000
        raw = np.fromfile(args.data / (e['name'] + '_labels.u32le'), dtype='<u4').astype(np.int64)
        images = {}
        for k in ks:
            obj = key_object(category, rows[k])
            mem = result['orders'][k]['members']
            if mem['hdbscan'][obj] is None or mem['margin_r'][obj] is None:
                continue
            render(xyz, raw, e['keys'], e['keys'][obj], mem['hdbscan'][obj], mem['margin_r'][obj], where / ('k%s.png' % k))
            images[k] = ('k%s.png' % k, obj)
        (where / 'data').mkdir(exist_ok=True)  # copie locale des points, ignorée par git
        for suffix in ('_sites.u32le', '_labels.u32le'):
            shutil.copyfile(args.data / (e['name'] + suffix), where / 'data' / (e['name'] + suffix))
        definition = {key: e[key] for key in ('name', 'kind', 'seq', 'frame', 'velodyne_sha256', 'labels_sha256', 'keys',
                                              'classes', 'points', 'sites', 'gaps', 'difficulty', 'sites_sha256',
                                              'crop_labels_sha256')}
        (where / 'bout.json').write_text(json.dumps(dict(
            schema='ehgp.zoltan.bout_hgp.v2', bouts=[definition], categorie=category, issues=outs, ordre_montre=shown,
            images={k: v[0] for k, v in images.items()}, orders=e['orders']), indent=1, ensure_ascii=False) + '\n')
        (where / 'README.md').write_text(example_readme(folder, category, e, rows, outs, images, where))
        index[category].append((folder, e, rows, outs, shown))
    scenes = []
    for spec_path in sorted(args.out.glob('*/0*_*/demo.json')):  # démos de scène entière, classées sur leurs objets suivis
        demo = spec_path.parent
        spec = json.loads(spec_path.read_text())
        result = members['zoltan_' + demo.name]
        keys = [o['select']['sem'] | (o['select']['inst'] << 16) for o in spec['objects']]
        indices = [result['meta']['objects'].index(key) for key in keys]
        rows = rows_of(result, indices)
        category, outs = classify(rows)
        if demo.parent.name != category:
            raise SystemExit('démo %s rangée dans %s, catégorie mesurée %s' % (demo.name, demo.parent.name, category))
        xyz = np.fromfile(args.scene_data / ('zoltan_%s_sites.u32le' % demo.name), dtype='<u4').reshape(-1, 3) / 1000.0
        raw = np.fromfile(args.scene_data / ('zoltan_%s_labels.u32le' % demo.name), dtype='<u4').astype(np.int64)
        images = {}
        for k in ORDERS:
            obj = key_object(category, rows[k])
            mem = result['orders'][k]['members']
            hm, gm = mem['hdbscan'][indices[obj]], mem['margin_r'][indices[obj]]
            if hm is None or gm is None:
                continue
            render(xyz, raw, keys, keys[obj], hm, gm, demo / ('hgp_k%s.png' % k), margin_m=2.0)
            images[k] = dict(image='hgp_k%s.png' % k, object=LETTERS[obj])
        (demo / 'resultats_hgp.json').write_text(json.dumps(dict(
            schema='ehgp.zoltan.resultats_hgp.v1', categorie=category, session=SESSION_SCENES,
            objets=[o['key'] for o in spec['objects']], issues=outs,
            orders={k: dict(hdbscan=[round(x, 4) for x in rows[k]['hdbscan']], hgp=[round(x, 4) for x in rows[k]['hgp']])
                    for k in ORDERS}, images=images), indent=1, ensure_ascii=False) + '\n')
        scenes.append((demo, spec, rows, outs, category))
    for category in CATEGORIES:
        (args.out / category).mkdir(exist_ok=True)
        (args.out / category / 'README.md').write_text(category_readme(category, counts, scenes, index[category],
                                                                       args.out / category))
    print(json.dumps(counts, indent=1, ensure_ascii=False))
    print('exemples', {c: len(v) for c, v in index.items()}, 'demos', [(s[0].name, s[4]) for s in scenes])


if __name__ == '__main__':
    main()
