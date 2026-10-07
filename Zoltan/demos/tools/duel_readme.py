#!/usr/bin/env python3
"""README des vidéos HGP contre HDBSCAN (Zoltan/demos/videos_hgp_hdbscan) et renvois depuis les bouts.

    python3 Zoltan/demos/tools/duel_readme.py [Zoltan/demos]

- README de chaque variante d'exemple (instances/, sans_sol/) : écrit en entier (vidéo, mesures à k = 5 et 10,
  tableau des événements, avec les textes mêmes des bandeaux de la vidéo) ;
- README de chaque exemple : section entre les marqueurs « video:début » et « video:fin » (le reste est écrit par
  tools/choisir_exemples.py) ;
- README de videos_hgp_hdbscan : liste des exemples entre les marqueurs « liste:début » et « liste:fin » ;
- README des bouts et des catégories de Zoltan/demos : une ligne de renvoi vers l'exemple de la même scène (appelé
  aussi par tools/choisir_bouts.py).
Une vidéo n'est citée que si ses deux thèmes et ses images fixes existent ; une variante peut en avoir une par ordre.
Chaque vidéo du duel a sa vidéo de la hiérarchie des supports au même ordre (tools/supports_scene.py, suffixe
« _supports »), citée à côté d'elle.
"""
import json
from pathlib import Path
import re
import sys

BEGIN, END = '<!-- video:début -->', '<!-- video:fin -->'
LIST_BEGIN, LIST_END = '<!-- liste:début -->', '<!-- liste:fin -->'
VIDEOS = 'videos_hgp_hdbscan'
LETTERS = 'ABC'
VARIANTS = ('instances', 'sans_sol')
VARIANT_TITLE = {'instances': 'instances de la vérité terrain seules',
                 'sans_sol': 'sol retiré automatiquement (Patchwork++)'}
OUTCOME = {'win': 'HGP réussit, HDBSCAN échoue', 'loss': 'HGP échoue, HDBSCAN réussit',
           'both_fail': 'les deux échouent', 'both_ok': 'les deux réussissent'}
SHORT = {'win': '**gain HGP**', 'loss': 'perte HGP', 'both_fail': 'deux échecs', 'both_ok': 'deux réussites'}


def fr(x):
    return ('%.2f' % x).replace('.', ',')


def cm(r):
    """Une décimale, comme le lecteur : des événements se suivent à quelques millimètres."""
    return ('%.1f cm' % (100 * r)).replace('.', ',')


def thousands(n):
    return '{:,}'.format(int(n)).replace(',', ' ')


def text(badge):
    return ''.join(part[0] for part in badge['parts']).strip()


def videos(variant):
    """[(k, résultats, préfixe des fichiers)] des vidéos complètes de la variante (deux thèmes, images fixes), par k."""
    out = []
    for res in sorted(variant.glob('resultats_duel_k*.json'), key=lambda p: int(re.search(r'k(\d+)', p.name).group(1))):
        k = int(re.search(r'k(\d+)', res.name).group(1))
        stem = '%s_%s_k%d' % (variant.parent.name, variant.name, k)
        needed = ['%s_%s%s' % (stem, theme, suffix) for theme in ('sombre', 'clair')
                  for suffix in ('.mp4', '_instant_cle.png', '_bilan.png')]
        if all((variant / name).is_file() for name in needed):
            out.append((k, json.loads(res.read_text(encoding='utf-8')), stem))
    return out


def supports_videos(variant):
    """{k: (résultats, préfixe)} des vidéos complètes de la hiérarchie des supports de la variante."""
    out = {}
    for res in variant.glob('resultats_supports_k*.json'):
        k = int(re.search(r'k(\d+)', res.name).group(1))
        stem = '%s_%s_k%d_supports' % (variant.parent.name, variant.name, k)
        needed = ['%s_%s%s' % (stem, theme, suffix) for theme in ('sombre', 'clair')
                  for suffix in ('.mp4', '_instant_cle.png', '_bilan.png')]
        if all((variant / name).is_file() for name in needed):
            out[k] = (json.loads(res.read_text(encoding='utf-8')), stem)
    return out


def supports_alt(result, k):
    p = key_pause(result)
    if p is None:
        return 'Hiérarchie des supports, image finale, k = %d' % k
    return 'Hiérarchie des supports, k = %d, r = %s : %s' % (k, cm(p['r']), ' ; '.join(text(b) for b in p['badges']).lstrip('✓✗ '))


def key_pause(result):
    t = result['timing']
    return next((p for p in t['pauses'] if p['t0'] <= t['key'] <= p['t1']), None)


def key_alt(result, k):
    p = key_pause(result)
    if p is None:
        return 'Image finale, k = %d' % k
    side = lambda s: ' ; '.join(text(b) for b in p['badges'][s]).lstrip('✓✗ ') or '—'
    if p.get('method') == 'hdbscan':  # premier balayage : HGP attend
        return 'Instant clé, k = %d, r = %s : HDBSCAN, %s ; HGP en attente' % (k, cm(p['r']), side('hdbscan'))
    return 'Instant clé, k = %d, r = %s : HGP, %s ; HDBSCAN au même r, %s' % (k, cm(p['r']), side('hgp'), side('hdbscan'))


def picture(prefix, stem, alt):
    return ['<picture>',
            '  <source media="(prefers-color-scheme: dark)" srcset="%s%s_sombre_instant_cle.png">' % (prefix, stem),
            '  <img alt="%s" src="%s%s_clair_instant_cle.png">' % (alt.replace('"', '&quot;'), prefix, stem),
            '</picture>']


def measures(spec):
    n = len(spec['bout']['keys'])
    out = ['| k | HGP, meilleur IoU (%s) | HDBSCAN, meilleur IoU | issue |' % ' / '.join(LETTERS[:n]),
           '| --- | --- | --- | --- |']
    for k in ('5', '10'):
        row = spec['orders'][k]
        cell = lambda xs: ' / '.join(('**%s**' % fr(x)) if x <= 0.5 else fr(x) for x in xs)
        out.append('| %s | %s | %s | %s |' % (k, cell(row['hgp']), cell(row['hdbscan']), OUTCOME[spec['issues'][k]]))
    return out


def variant_readme(variant):
    spec = json.loads((variant / 'bout.json').read_text(encoding='utf-8'))
    entry, crop, v = spec['bout'], spec['decoupe'], spec['variante']
    other = [x for x in VARIANTS if x != v][0]
    found = videos(variant)
    sup = supports_videos(variant)
    head = (variant.parent / 'README.md').read_text(encoding='utf-8').splitlines()[0].lstrip('# ')
    out = ['# %s — %s' % (head, VARIANT_TITLE[v]), '',
           '[Exemple](../README.md) · autre variante : [%s](../%s/README.md) · [liste des exemples](../../README.md)'
           % (VARIANT_TITLE[other], other), '']
    for k, res, stem in found:
        win = spec['issues'][str(k)] == 'win'
        out += ['**k = %d** (%s) :' % (k, OUTCOME[spec['issues'][str(k)]] if win else
                                      OUTCOME[spec['issues'][str(k)]] + ', aucun gain HGP dans cette variante'), '']
        out += picture('', stem, key_alt(res, k)) + ['',
                'Vidéo de %d s, k = %d, 1920 × 1080 : [thème sombre](%s_sombre.mp4) · [thème clair](%s_clair.mp4) ; image '
                'finale : [sombre](%s_sombre_bilan.png) · [clair](%s_clair_bilan.png).' % (
                    round(res['timing']['duration']), k, stem, stem, stem, stem), '']
        if k in sup:
            sres, sstem = sup[k]
            c = sres['counts']
            out += picture('', sstem, supports_alt(sres, k)) + ['',
                    'Hiérarchie des supports q2, q3, q4 au même ordre, %d s : [thème sombre](%s_sombre.mp4) · '
                    '[thème clair](%s_clair.mp4) ; image finale : [sombre](%s_sombre_bilan.png) · [clair](%s_clair_bilan.png). '
                    'Arbre couvrant d\'ordre %d : %s nœuds, %s naissances et fusions, chacune avec son support S\\* (%s '
                    'supports : %s arêtes q2, %s triangles q3, %s tétraèdres q4).' % (round(sres['timing']['duration']), sstem, sstem, sstem, sstem, k, thousands(c['nodes']),
                              thousands(c['balls']), thousands(c['supports']), thousands(c['q2']), thousands(c['q3']),
                              thousands(c['q4'])), '']
    if v == 'sans_sol':
        removed = sum(crop['objets_retires_comme_sol'])
        out += ['Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des '
                'objets élargie de %s m, à toutes hauteurs : %s points, dont %s des objets (%s) ; %s points des objets '
                'retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu\'à mesurer.' % (
                    fr(crop['marge']).rstrip('0').rstrip(','), thousands(crop['sites']),
                    thousands(sum(crop['objets_gardes'])),
                    ', '.join('%s %d' % (LETTERS[j], c) for j, c in enumerate(crop['objets_gardes'])), removed), '']
    else:
        out += ['Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), %s points (%s) : ni sol, '
                'ni fond, ni autre objet.' % (thousands(crop['sites']), ', '.join(
                    '%s %d' % (LETTERS[j], c) for j, c in enumerate(entry['points']))), '']
    out += ['Meilleur IoU de chaque objet (meilleur bloc ; seuls les points non étiquetés et aberrants, classes 0 et 1, '
            'sont exclus : « autre structure » et « autre objet » comptent comme du fond) :', ''] + measures(spec)
    out += ['', 'En gras : objet à 0,5 ou moins, qu\'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une '
            'vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.', '']
    for k, res, stem in found:
        pauses = res['timing']['pauses']
        out += ['## Événements de la vidéo à k = %d' % k, '',
                'Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :', '',
                '| r | HDBSCAN |', '| --- | --- |']
        out += ['| %s | %s |' % (cm(p['r']), ' ; '.join(text(b) for b in p['badges']['hdbscan']))
                for p in pauses if p['method'] == 'hdbscan']
        out += ['', 'Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :', '',
                '| r | HGP | HDBSCAN au même r |', '| --- | --- | --- |']
        out += ['| %s | %s | %s |' % (cm(p['r']), ' ; '.join(text(b) for b in p['badges']['hgp']),
                                      ' ; '.join(text(b) for b in p['badges']['hdbscan']))
                for p in pauses if p['method'] == 'hgp']
        out += ['', 'Nombres : [`resultats_duel_k%d.json`](resultats_duel_k%d.json).' % (k, k), '']
        if k in sup:
            sres, sstem = sup[k]
            best = ' / '.join(fr(x) for x in sres['best'])
            out += ['## Hiérarchie des supports à k = %d' % k, '',
                    'Meilleur IoU des sites des supports du nœud qui suit chaque objet : %s (hiérarchie de points HGP : '
                    '%s). Événements, mêmes textes que les bandeaux :' % (best, ' / '.join(fr(x) for x in res['methods']['hgp']['best'])),
                    '', '| r | supports |', '| --- | --- |']
            out += ['| %s | %s |' % (cm(p['r']), ' ; '.join(text(b) for b in p['badges'])) for p in sres['timing']['pauses']]
            out += ['', 'Nombres : [`resultats_supports_k%d.json`](resultats_supports_k%d.json).' % (k, k), '']
    if found:
        out += ['Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).', '']
    out += ['## Données', '',
            '`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas '
            'versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles '
            '(README de la liste, « Reproduire »).', '']
    return '\n'.join(out)


def example_section(example):
    """Section vidéo du README d'un exemple : les vidéos des deux variantes, une affiche par variante."""
    found = {v: videos(example / v) for v in VARIANTS}
    if not any(found.values()):
        return []
    out = [BEGIN, '## Vidéos', '', '| | %s | %s |' % (VARIANT_TITLE['instances'], VARIANT_TITLE['sans_sol']),
           '| --- | --- | --- |']
    cells = []
    for v in VARIANTS:
        sup = supports_videos(example / v)
        links = ['k = %d : [sombre](%s/%s_sombre.mp4) · [clair](%s/%s_clair.mp4)' % (k, v, stem, v, stem)
                 + ('' if k not in sup else ' ; supports : [sombre](%s/%s_sombre.mp4) · [clair](%s/%s_clair.mp4)' % (
                     v, sup[k][1], v, sup[k][1]))
                 for k, res, stem in found[v]]
        cells.append('[README](%s/README.md)%s' % (v, (' · ' + ' ; '.join(links)) if links else ''))
    out.append('| vidéos | %s | %s |' % tuple(cells))
    for v in VARIANTS:
        if found[v]:
            k, res, stem = found[v][0]
            label = VARIANT_TITLE[v][0].upper() + VARIANT_TITLE[v][1:]
            out += ['', '**%s**, k = %d :' % (label, k), ''] + picture('%s/' % v, stem, key_alt(res, k))
    out += ['', END]
    return out


def replace_section(text_in, lines, before, begin=BEGIN, end=END):
    """Remplace la section entre les marqueurs, ou l'insère avant la ligne `before` (à la fin si absente)."""
    body = '\n'.join(lines)
    if begin in text_in:
        start, stop = text_in.index(begin), text_in.index(end) + len(end)
        tail = text_in[stop:].lstrip('\n')
        if not lines:
            return text_in[:start] + tail
        return text_in[:start] + body + ('\n\n' + tail if tail else '\n')
    if not lines:
        return text_in
    at = text_in.find('\n' + before) if before else -1
    if at < 0:
        return text_in.rstrip('\n') + '\n\n' + body + '\n'
    return text_in[:at + 1] + body + '\n\n' + text_in[at + 1:]


def examples_of(root):
    path = root / VIDEOS / 'exemples.json'
    return json.loads(path.read_text(encoding='utf-8'))['exemples'] if path.is_file() else []


def list_section(root):
    rows = []
    for ex in examples_of(root):
        where = root / VIDEOS / ex['folder']
        links = []
        for v in VARIANTS:
            sup = supports_videos(where / v)
            for k, res, stem in videos(where / v):
                links.append('%s k = %d : [sombre](%s/%s/%s_sombre.mp4) · [clair](%s/%s/%s_clair.mp4)' % (
                    'instances' if v == 'instances' else 'sans sol', k, ex['folder'], v, stem, ex['folder'], v, stem)
                    + ('' if k not in sup else ', supports [sombre](%s/%s/%s_sombre.mp4) · [clair](%s/%s/%s_clair.mp4)' % (
                        ex['folder'], v, sup[k][1], ex['folder'], v, sup[k][1])))
        objs = ', '.join('%s %s' % (LETTERS[j], {'bicycle': 'vélo', 'person': 'piéton'}.get(c, c))
                         for j, c in enumerate(ex['classes']))
        rows.append('| [%s/%s](%s/README.md) | %s | %s / %s | %s / %s | %s |' % (
            ex['seq'], ex['frame'], ex['folder'], objs, SHORT[ex['issues']['instances']['5']],
            SHORT[ex['issues']['instances']['10']], SHORT[ex['issues']['sans_sol']['5']],
            SHORT[ex['issues']['sans_sol']['10']], ' ; '.join(links)))
    return [LIST_BEGIN, '| exemple | objets | instances seules : k = 5 / 10 | sans sol : k = 5 / 10 | vidéos |',
            '| --- | --- | --- | --- | --- |'] + rows + [LIST_END]


def bout_pointer(where):
    """Renvoi d'un bout de Zoltan/demos/<catégorie>/ vers l'exemple vidéo de la même scène, s'il existe."""
    root = where.parent.parent
    spec = json.loads((where / 'bout.json').read_text(encoding='utf-8'))
    name = spec['bouts'][0]['name']
    for ex in examples_of(root):
        if name in ex['scene']:
            same = 'de ce groupe' if ex['name'] == name else 'd\'un groupe de la même scène (%s)' % ex['folder']
            return [BEGIN, 'Vidéos HGP contre HDBSCAN %s, en deux variantes (instances seules, sol retiré '
                    'automatiquement) : [`%s/%s`](../../%s/%s/README.md).' % (same, VIDEOS, ex['folder'], VIDEOS,
                                                                            ex['folder']), END]
    return []


def category_pointer(category):
    root = category.parent
    if not examples_of(root):
        return []
    return [BEGIN, 'Vidéos HGP contre HDBSCAN, en deux variantes par exemple : [`%s/`](../%s/README.md).' % (VIDEOS, VIDEOS),
            END]


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
    videos = root / VIDEOS
    for ex in examples_of(root):
        where = videos / ex['folder']
        for v in VARIANTS:
            (where / v / 'README.md').write_text(variant_readme(where / v) + '\n', encoding='utf-8')
        readme = where / 'README.md'
        readme.write_text(replace_section(readme.read_text(encoding='utf-8'), example_section(where), ''),
                          encoding='utf-8')
    readme = videos / 'README.md'
    if readme.is_file():
        readme.write_text(replace_section(readme.read_text(encoding='utf-8'), list_section(root), '', LIST_BEGIN, LIST_END),
                          encoding='utf-8')
    for category in sorted(p for p in root.iterdir() if p.is_dir() and re.match(r'hgp_(reussit|echoue)_', p.name)):
        for where in sorted(p for p in category.iterdir() if (p / 'bout.json').is_file()):
            path = where / 'README.md'
            path.write_text(replace_section(path.read_text(encoding='utf-8'), bout_pointer(where), '## Images'),
                            encoding='utf-8')
        path = category / 'README.md'
        path.write_text(replace_section(path.read_text(encoding='utf-8'), category_pointer(category),
                                        'Critères, mesure et légende des images'), encoding='utf-8')
    print('README mis à jour sous', root)


if __name__ == '__main__':
    main()
