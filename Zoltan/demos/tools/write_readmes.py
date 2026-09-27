#!/usr/bin/env python3
"""Écrit le README de chaque démo à partir de demo.json et des resultats_*.json.

Usage : ``python3 Zoltan/demos/tools/write_readmes.py``

Les chiffres ne sont jamais recopiés à la main : ils viennent des fichiers
de résultats écrits par build_scene.py. Le texte libre vient du champ
``description`` de demo.json.
"""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUN_NAMES = {'hdbscan_K5': 'HDBSCAN, K = 5', 'hdbscan_K10': 'HDBSCAN, K = 10', 'alpine_bev': 'ALPINE sans sémantique'}
THEMES = ('sombre', 'clair')  # comme Percolia.com : sombre (fond marine) et clair (fond blanc)


def fr(v, d=2):
    return '—' if v is None else f'{v:.{d}f}'.replace('.', ',')


def fl(v):
    """Niveau en mètres : trois décimales sous 1 m, comme dans les vidéos."""
    return '—' if v is None else fr(v, 3 if v < 1 else 2)


def nfmt(v):
    return f'{v:,}'.replace(',', '\u202f')


def run_tag(run):
    return f"hdbscan_K{run['K']}" if run['method'] == 'hdbscan' else 'alpine_bev'


def themed(base, suffix='', where=ROOT):
    """Liens « sombre · clair » vers les deux versions d'un fichier rendu.

    Refuse un lien vers un fichier absent : le README ne doit citer que ce qui
    a été rendu (tools/render_video.cjs)."""
    missing = [f'{base}_{th}{suffix}' for th in THEMES if not (where / f'{base}_{th}{suffix}').is_file()]
    if missing:
        raise SystemExit(f'média absent, lancer tools/render_video.cjs : {missing}')
    return ' · '.join(f"[{th}]({base}_{th}{suffix})" for th in THEMES)


def write(demo: Path):
    spec = json.loads((demo / 'demo.json').read_text(encoding='utf-8'))
    tags = [run_tag(r) for r in spec['runs']]
    res = {t: json.loads((demo / f'resultats_{t}.json').read_text(encoding='utf-8')) for t in tags}
    first = res[tags[0]]
    sym = {t: ('t' if t == 'alpine_bev' else 'r') for t in tags}
    ground = 'sol retiré par Patchwork++, paramètres de la v8' if spec.get('ground', 'patchwork_v8') != 'none' else 'sol conservé'
    lines = [f"# {spec['title']}", '']
    lines += [f"Trame SemanticKITTI `{spec['seq']}/{spec['frame']}` ({ground}) : "
              f"{nfmt(first['n_points_hierarchy'])} points dans la hiérarchie, trame entière. "
              f"Empreinte sha256 du `.bin` : `{first['velodyne_sha256']}`.", '']
    for para in spec.get('description', []):
        lines += [para, '']
    lines += ['## Objets suivis', '', '| objet | classe | vérité terrain | points |', '| --- | --- | --- | ---: |']
    for o in first['objects']:
        sel = o['select']
        lines.append(f"| {o['key']} | {o['name']} | sem {sel['sem']}, instance {sel['inst']} | {o['points']} |")
    lines += ['', '## Résultats', '',
              'Meilleur IoU d’un nœud de l’arbre (≤ 0,5 : aucune extraction ne rend l’objet), '
              'premier niveau apparié, premier niveau fusionné, en mètres.', '']
    head = '| méthode | ' + ' | '.join(f"{o['key']} : IoU / apparié / fusionné" for o in first['objects']) + ' | coupe commune |'
    lines += [head, '| --- |' + ' --- |' * (len(first['objects']) + 1)]
    for t in tags:
        r = res[t]
        cells = [f"{fr(o['best_iou'])} / {fl(o['first_match_m'])} / {fl(o['first_fusion_m'])}" for o in r['objects']]
        cut = r['best_common_cut']
        cut_txt = f"{cut['matched_objects']} sur {len(r['objects'])}"
        if cut['level'] is not None:
            cut_txt += (f" ({sym[t]} de {fl(cut['level'])} à {fl(cut['until'])})" if cut.get('until') is not None
                        else f" (dès {sym[t]} = {fl(cut['level'])})")
        lines.append(f"| {RUN_NAMES[t]} | " + ' | '.join(cells) + f' | {cut_txt} |')
    sw = first['sweep_best_iou']
    alpine = spec.get('ground', 'patchwork_v8') != 'none'
    lines += ['', 'Meilleur IoU pour chaque K = min_samples de HDBSCAN' + (', et pour ALPINE sans sémantique :' if alpine else
                                                                        ' (ALPINE omis : il suppose le sol retiré) :'), '',
              '| objet | ' + ' | '.join(f'K = {k}' for k in sw['hdbscan_K']) + (' | ALPINE |' if alpine else ' |'),
              '| --- |' + ' ---: |' * (len(sw['hdbscan_K']) + (1 if alpine else 0))]
    for o, ob in enumerate(first['objects']):
        lines.append(f"| {ob['key']} | " + ' | '.join(fr(v) for v in sw['hdbscan'][o])
                     + (f" | {fr(sw['alpine_bev'][o])} |" if alpine else ' |'))
    lines += ['', '## Vidéos', '']
    for run, t in zip(spec['runs'], tags):
        base = f'{demo.name}_{t}'
        if run.get('video', True):
            lines.append(f"- {RUN_NAMES[t]} : vidéo {themed(base, '.mp4', demo)} ; instant clé {themed(base, '_instant_cle.png', demo)} ; "
                         f"bilan {themed(base, '_bilan.png', demo)}")
        else:
            lines.append(f"- {RUN_NAMES[t]} : pas de vidéo (arbre calculé, chiffres ci-dessus seulement).")
    lines += ['', f"Fusions de branches suivies ({RUN_NAMES[tags[0]]}) : " +
              (' ; '.join(f"{'+'.join(b['objects'])} à {sym[tags[0]]} = {fl(b['level'])} m" for b in first['branch_merges_m']) or 'aucune') + '.',
              '', 'Chaque vidéo existe en thème sombre (fond marine) et clair (fond blanc), comme Percolia.com : '
              'prendre celui du fond des diapositives.',
              '', 'Régénérer : `python3 Zoltan/demos/tools/build_scene.py Zoltan/demos/' + demo.name + '`, puis '
              '`node Zoltan/demos/tools/render_video.cjs Zoltan/demos/' + demo.name + ' <étiquette>` '
              '(les deux thèmes ; `--theme clair` ou `--theme sombre` pour un seul).', '']
    (demo / 'README.md').write_text('\n'.join(lines), encoding='utf-8')


def _replace_block(path: Path, name: str, body: str):
    text = path.read_text(encoding='utf-8')
    a, b = f'<!-- {name}:début -->', f'<!-- {name}:fin -->'
    i, j = text.index(a) + len(a), text.index(b)
    path.write_text(text[:i] + '\n' + body + '\n' + text[j:], encoding='utf-8')


def catalogue(demos):
    """Tableau du catalogue (README principal) : meilleur IoU de chaque objet, en gras si ≤ 0,5."""
    def cell(vals):
        return ' / '.join(f'**{fr(v)}**' if v <= 0.5 else fr(v) for v in vals)
    rows = ['| démo | trame | objets | HDBSCAN K = 5 | HDBSCAN K = 10 | ALPINE sans sémantique | vidéos |',
            '| --- | --- | --- | --- | --- | --- | --- |']
    for demo in demos:
        spec = json.loads((demo / 'demo.json').read_text(encoding='utf-8'))
        res = {run_tag(r): json.loads((demo / f'resultats_{run_tag(r)}.json').read_text(encoding='utf-8')) for r in spec['runs']}
        sw = next(iter(res.values()))['sweep_best_iou']
        k5 = [sw['hdbscan'][o][sw['hdbscan_K'].index(5)] for o in range(len(spec['objects']))]
        k10 = [sw['hdbscan'][o][sw['hdbscan_K'].index(10)] for o in range(len(spec['objects']))]
        ground = spec.get('ground', 'patchwork_v8') != 'none'
        alp = cell(sw['alpine_bev']) if ground else '(sans objet : ALPINE suppose le sol retiré)'
        vids = ' ; '.join(f"{('K' + str(r['K'])) if r['method'] == 'hdbscan' else 'ALPINE'} "
                          + themed(f'{demo.name}/{demo.name}_{run_tag(r)}', '.mp4')
                          for r in spec['runs'] if r.get('video', True))
        label = demo.name[:2] + ' ' + spec['title'].split(' :')[0].lower()
        rows.append(f"| [{label}]({demo.name}/) | {spec['seq']}/{spec['frame']}, {'sans sol' if ground else '**sol conservé**'} "
                    f"| {spec.get('catalogue', '')} | {cell(k5)} | {cell(k10)} | {alp} | {vids} |")
    return '\n'.join(rows)


def class_stats():
    """Tableau par classe du criblage 1 sur 8 (instances d'au moins 50 points, seuil min_points de la PQ)."""
    names = {'car': 'voiture', 'person': 'piéton', 'bicycle': 'vélo', 'other-vehicle': 'autre véhicule',
             'motorcycle': 'moto', 'bicyclist': 'cycliste', 'truck': 'camion', 'motorcyclist': 'motard'}
    st = {}
    for line in (ROOT / 'recherche' / 'criblage_08.jsonl').read_text(encoding='utf-8').splitlines():
        row = json.loads(line)
        if row.get('origin') != 'criblage_1_sur_8':
            continue
        for i in row['instances']:
            if i['points'] < 50:
                continue
            b = i['best_iou']
            v = st.setdefault(i['cls'], [0, 0, 0, 0])
            v[0] += 1
            v[1] += b['5'] <= 0.5
            v[2] += b['10'] <= 0.5
            v[3] += max(b.values()) <= 0.5
    rows = ['| classe | instances | aucun nœud > 0,5 à K = 5 | à K = 10 | pour K = 1, 5 et 10 |',
            '| --- | ---: | ---: | ---: | ---: |']
    for cls, v in sorted(st.items(), key=lambda kv: -kv[1][0]):
        rows.append(f'| {names.get(cls, cls)} | {nfmt(v[0])} | {v[1]} | {v[2]} | {v[3]} |')
    return '\n'.join(rows)


def main():
    demos = sorted(p for p in ROOT.iterdir() if p.is_dir() and (p / 'demo.json').is_file())
    for demo in demos:
        write(demo)
        print('README', demo.name)
    _replace_block(ROOT / 'README.md', 'catalogue', catalogue(demos))
    stats = class_stats()
    _replace_block(ROOT / 'README.md', 'stats', stats)
    _replace_block(ROOT / 'recherche' / 'README.md', 'stats', stats)


if __name__ == '__main__':
    main()
