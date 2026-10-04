#!/usr/bin/env python3
"""Exemples vidéo HGP contre HDBSCAN, chacun en deux variantes : instances de la vérité terrain seules, et sol retiré
automatiquement (Patchwork++).

    python3 Zoltan/demos/tools/choisir_exemples.py --bouts LOT/bouts.json --sans-sol SANS_SOL/bouts_sans_sol.json \
        --mesures MESURES --data-instances LOT/data --data-sans-sol SANS_SOL/data --out Zoltan/demos/videos_hgp_hdbscan

Entrées : les groupes de vélos et de piétons de tools/chercher_bouts.py (bouts.json), leur variante sans sol
(--sans-sol, même outil) et les meilleurs IoU de tools/mesurer_bouts.py (MESURES/instances, MESURES/sans_sol).

Critères, fixés avant la lecture des résultats :
- à l'ordre k (5 et 10) et dans une variante, une méthode RÉUSSIT si chaque objet du groupe a un bloc d'IoU > 1/2 ;
  issue « HGP réussit, HDBSCAN échoue » (gain), l'inverse (perte), les deux échouent, les deux réussissent ;
- un groupe est un exemple s'il a au moins un gain (k = 5 ou 10, l'une ou l'autre variante) ;
- deux groupes d'une même séquence qui partagent une instance montrent la même scène (une rangée de vélos vue sous
  plusieurs groupes ou plusieurs trames voisines) : un seul exemple par scène, celui qui a le plus de gains, puis le
  plus de gains à k = 5, puis le plus d'objets, puis le premier par nom ;
- ordre de la vidéo d'une variante : 5 par défaut ; 10 si la variante n'a de gain qu'à k = 10.

Sorties : OUT/<exemple>/ avec README.md, instances/ et sans_sol/ (bout.json de la variante, schéma
ehgp.zoltan.bout_variante.v1 ; data/ local, ignoré par git, copie des points) ; OUT/exemples.json (issues des groupes,
scènes, choix) ; OUT/README.md. Les vidéos s'ajoutent ensuite (tools/duel_scene.py, tools/render_duel.cjs,
tools/duel_readme.py).
"""
import argparse
import json
from pathlib import Path
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from choisir_bouts import FR, LETTERS, slug, title  # noqa: E402

VARIANTS = ('instances', 'sans_sol')
ORDERS = ('5', '10')
OUTCOME = {'win': 'HGP réussit, HDBSCAN échoue', 'loss': 'HGP échoue, HDBSCAN réussit',
           'both_fail': 'les deux échouent', 'both_ok': 'les deux réussissent'}
VARIANT_TITLE = {'instances': 'instances de la vérité terrain seules',
                 'sans_sol': 'sol retiré automatiquement (Patchwork++)'}


def outcome(row):
    hdb_fails, hgp_succeeds = min(row['hdbscan']) <= 0.5, min(row['hgp']) > 0.5
    return {(True, True): 'win', (False, False): 'loss', (True, False): 'both_fail',
            (False, True): 'both_ok'}[(hdb_fails, hgp_succeeds)]


def fr(x):
    return ('%.2f' % x).replace('.', ',')


def thousands(n):
    """Entier avec espace fine insécable des milliers (typographie française)."""
    return '{:,}'.format(int(n)).replace(',', '\u202f')


def scenes_of(groups):
    """Composantes des groupes qui partagent une instance dans une même séquence."""
    parent = {g['name']: g['name'] for g in groups}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    owner = {}
    for g in groups:
        for key in g['keys']:
            tag = (g['seq'], key >> 16)
            if tag in owner:
                parent[find(g['name'])] = find(owner[tag])
            else:
                owner[tag] = g['name']
    out = {}
    for g in groups:
        out.setdefault(find(g['name']), []).append(g)
    return list(out.values())


def folder_name(g):
    return '%s_%s_%s_%s' % (g['seq'], g['frame'], slug(g['classes']), '_'.join(str(k >> 16) for k in g['keys']))


def table(g):
    """Tableau des meilleurs IoU : une ligne par variante et par ordre."""
    n = len(g['keys'])
    out = ['| variante | k | HGP, meilleur IoU (%s) | HDBSCAN, meilleur IoU | issue |' % ' / '.join(LETTERS[:n]),
           '| --- | --- | --- | --- | --- |']
    for v in VARIANTS:
        for k in ORDERS:
            row = g['mesures'][v][k]
            cell = lambda xs: ' / '.join(('**%s**' % fr(x)) if x <= 0.5 else fr(x) for x in xs)
            out.append('| %s | %s | %s | %s | %s |' % (VARIANT_TITLE[v], k, cell(row['hgp']), cell(row['hdbscan']),
                                                      OUTCOME[g['issues'][v][k]]))
    return out


SHORT = {'win': '**gain HGP**', 'loss': 'perte HGP', 'both_fail': 'deux échecs', 'both_ok': 'deux réussites'}


def scene_table(g, mates):
    """Autres groupes de la même scène (mêmes instances, trames voisines) et leurs issues."""
    if not mates:
        return []
    out = ['## Même scène', '',
           'Autres groupes de la recherche qui partagent une instance avec celui-ci (même rangée, trames voisines) ; un '
           'seul exemple est montré par scène.', '',
           '| groupe | trame | instances seules : k = 5 / 10 | sans sol : k = 5 / 10 |', '| --- | --- | --- | --- |']
    for m in mates:
        out.append('| %s | %s/%s | %s / %s | %s / %s |' % (
            ', '.join(str(key >> 16) for key in m['keys']),
            m['seq'], m['frame'], SHORT[m['issues']['instances']['5']], SHORT[m['issues']['instances']['10']],
            SHORT[m['issues']['sans_sol']['5']], SHORT[m['issues']['sans_sol']['10']]))
    return out + ['', 'Les groupes sont nommés par les numéros d\'instance SemanticKITTI de leurs objets.', '']


def example_readme(g, shown, mates=()):
    ss = g['sans_sol']
    out = ['# %s (trame %s/%s)' % (title(g['classes']), g['seq'], g['frame']), '',
           'Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence %s, trame %s. Deux '
           'variantes, chacune dans son sous-dossier :' % (g['seq'], g['frame']), '',
           '- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), %s points ;' % thousands(g['sites']),
           '- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets '
           'élargie de %s m, %s points (aucune étiquette ne sert au nettoyage).' % (fr(ss['marge']).rstrip('0').rstrip(','),
                                                                                 thousands(ss['sites'])), '',
           '| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |',
           '| --- | --- | --- | --- | --- |']
    for j, (cl, pts) in enumerate(zip(g['classes'], g['points'])):
        out.append('| %s | %s | %d | %d | %d |' % (LETTERS[j], FR.get(cl, cl), pts, ss['objets_gardes'][j],
                                                  ss['objets_retires_comme_sol'][j]))
    letters = {key: LETTERS[j] for j, key in enumerate(g['keys'])}
    gaps = ', '.join('%s–%s : %s m' % (letters[int(a)], letters[int(b)], fr(v))
                     for (a, b), v in ((tuple(p.split('-')), v) for p, v in sorted(g['gaps'].items())))
    others = ss['autres_instances']
    out += ['', 'Écarts (plus courte distance entre les points de deux objets) : %s. Dans la découpe sans sol : %s ; '
            '%d points void ; %d points de sol retirés.' % (
                gaps, ('%d autre%s instance%s (%s points)' % (len(others), 's' if len(others) > 1 else '',
                                                             's' if len(others) > 1 else '',
                                                             ', '.join(str(v) for v in others.values())))
                if others else 'aucune autre instance', ss['void'], ss['sol_retire']), '']
    out += table(g)
    out += ['', 'En gras : objet à 0,5 ou moins, qu\'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. '
            'Vidéos : k = %s (instances), k = %s (sans sol). Objets : instances SemanticKITTI %s.' % (
                shown['instances'], shown['sans_sol'],
                ', '.join('%s = %d' % (LETTERS[j], key >> 16) for j, key in enumerate(g['keys']))), '']
    out += scene_table(g, mates)
    return '\n'.join(out)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--bouts', type=Path, required=True)
    p.add_argument('--sans-sol', type=Path, required=True)
    p.add_argument('--mesures', type=Path, required=True)
    p.add_argument('--data-instances', type=Path, required=True)
    p.add_argument('--data-sans-sol', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    lot = {e['name']: e for e in json.loads(args.bouts.read_text())['bouts']}
    sans_sol = {e['name']: e for e in json.loads(args.sans_sol.read_text())['bouts']}
    groups = []
    for name, e in sorted(sans_sol.items()):
        g = dict(lot[name], sans_sol=e['sans_sol'], mesures={}, issues={})
        for v in VARIANTS:
            m = json.loads((args.mesures / v / (name + '.json')).read_text())
            if m['status'] != 'ok':
                raise SystemExit('mesure en échec : %s %s' % (v, name))
            g['mesures'][v] = {k: dict(hgp=m['orders'][k]['hgp'], hdbscan=m['orders'][k]['hdbscan']) for k in ORDERS}
            g['issues'][v] = {k: outcome(m['orders'][k]) for k in ORDERS}
        g['gains'] = sum(g['issues'][v][k] == 'win' for v in VARIANTS for k in ORDERS)
        groups.append(g)
    counts = {v: {k: {o: sum(g['issues'][v][k] == o for g in groups) for o in OUTCOME} for k in ORDERS} for v in VARIANTS}
    candidates = [g for g in groups if g['gains']]
    chosen = []
    for scene in scenes_of(candidates):
        best = sorted(scene, key=lambda g: (-g['gains'], -sum(g['issues'][v]['5'] == 'win' for v in VARIANTS),
                                            -len(g['keys']), g['name']))[0]
        chosen.append(dict(best, scene=sorted(x['name'] for x in scene)))
    chosen.sort(key=lambda g: (g['seq'], g['frame'], g['name']))
    args.out.mkdir(parents=True, exist_ok=True)
    for g in chosen:
        where = args.out / folder_name(g)
        shown = {}
        for v in VARIANTS:
            wins = [k for k in ORDERS if g['issues'][v][k] == 'win']
            shown[v] = '5' if '5' in wins or not wins else wins[0]
            sub = where / v
            (sub / 'data').mkdir(parents=True, exist_ok=True)
            crop = g['sans_sol'] if v == 'sans_sol' else {key: g[key] for key in ('sites', 'duplicates', 'sites_sha256',
                                                                                 'crop_labels_sha256')}
            src = args.data_sans_sol if v == 'sans_sol' else args.data_instances
            for suffix in ('_sites.u32le', '_labels.u32le'):
                shutil.copyfile(src / (g['name'] + suffix), sub / 'data' / (g['name'] + suffix))
            definition = {key: g[key] for key in ('name', 'kind', 'seq', 'frame', 'velodyne_sha256', 'labels_sha256',
                                                  'keys', 'classes', 'points', 'gaps', 'difficulty')}
            (sub / 'bout.json').write_text(json.dumps(dict(
                schema='ehgp.zoltan.bout_variante.v1', variante=v, titre=VARIANT_TITLE[v], bout=definition,
                decoupe=crop, orders=g['mesures'][v], issues=g['issues'][v], ordre_montre=shown[v],
                mesure='locale (codespace), morsehgp3D_v11 export natif + bench/points_radius.py, scikit-learn 1.7.2 ; '
                       'variante instances identique aux sessions G4 claudebouts1'), indent=1, ensure_ascii=False) + '\n')
        mates = sorted((x for x in groups if x['name'] in g['scene'] and x['name'] != g['name']),
                       key=lambda x: (x['frame'], x['name']))
        (where / 'README.md').write_text(example_readme(g, shown, mates) + '\n')
        g['folder'], g['shown'] = where.name, shown
    keep = ('name', 'seq', 'frame', 'kind', 'keys', 'classes', 'points', 'issues', 'mesures', 'gains')
    (args.out / 'exemples.json').write_text(json.dumps(dict(
        schema='ehgp.zoltan.exemples_videos.v1', criteres=__doc__.split('Sorties')[0], comptes=counts,
        groupes=[{key: g[key] for key in keep} for g in groups],
        exemples=[dict({key: g[key] for key in keep}, folder=g['folder'], shown=g['shown'], scene=g['scene'])
                  for g in chosen]), indent=1, ensure_ascii=False) + '\n')
    print(json.dumps(counts, indent=1, ensure_ascii=False))
    for g in chosen:
        print(g['folder'], g['gains'], g['shown'], len(g['scene']))


if __name__ == '__main__':
    main()
