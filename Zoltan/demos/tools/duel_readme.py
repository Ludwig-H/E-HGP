#!/usr/bin/env python3
"""Sections « vidéo » des README : vidéos HGP contre HDBSCAN des bouts de scène.

    python3 Zoltan/demos/tools/duel_readme.py Zoltan/demos/hgp_reussit_hdbscan_echoue [AUTRE_CATEGORIE ...]

Pour chaque bout qui a ses vidéos (<bout>_hgp_hdbscan_k<k>_sombre.mp4 et _clair.mp4, écrites par
tools/render_duel.cjs) et ses résultats (resultats_duel_k<k>.json, écrits par tools/duel_scene.py), la section est
réécrite entre les marqueurs « video:début » et « video:fin » du README du bout, avant « ## Images » : l'image de
l'instant clé, les liens et le tableau des événements, avec les textes mêmes des bandeaux de la vidéo. Le README de
la catégorie reçoit la liste des vidéos. tools/choisir_bouts.py appelle les mêmes fonctions : une régénération des
README garde ces sections.
"""
import json
from pathlib import Path
import re
import sys

BEGIN, END = '<!-- video:début -->', '<!-- video:fin -->'
GUIDE = 'vidéos-hgp-contre-hdbscan-des-bouts'  # ancre de la section du README de demos/


def videos(where):
    """[(k, résultats)] des vidéos complètes du bout (deux thèmes, images fixes, résultats)."""
    out = []
    for res in sorted(where.glob('resultats_duel_k*.json'), key=lambda p: int(re.search(r'k(\d+)', p.name).group(1))):
        k = int(re.search(r'k(\d+)', res.name).group(1))
        stem = '%s_hgp_hdbscan_k%d' % (where.name, k)
        needed = ['%s_%s%s' % (stem, theme, suffix) for theme in ('sombre', 'clair')
                  for suffix in ('.mp4', '_instant_cle.png', '_bilan.png')]
        if all((where / name).is_file() for name in needed):
            out.append((k, json.loads(res.read_text(encoding='utf-8'))))
    return out


def text(badge):
    return ''.join(part[0] for part in badge['parts']).strip()


def cm(r):
    """Une décimale, comme le lecteur : des événements se suivent à quelques millimètres."""
    return ('%.1f cm' % (100 * r)).replace('.', ',')


def key_pause(result):
    t = result['timing']
    return next(p for p in t['pauses'] if p['t0'] <= t['key'] <= p['t1'])


def bout_section(where):
    """Lignes Markdown de la section vidéo d'un bout, marqueurs compris ; [] s'il n'a pas de vidéo."""
    found = videos(where)
    if not found:
        return []
    lines = [BEGIN]
    for k, res in found:
        stem = '%s_hgp_hdbscan_k%d' % (where.name, k)
        key = key_pause(res)
        alt = 'Instant clé, k = %d, r = %s : HGP, %s ; HDBSCAN, %s' % (
            k, cm(key['r']), ' ; '.join(text(b) for b in key['badges']['hgp']).lstrip('✓✗ ') or '—',
            ' ; '.join(text(b) for b in key['badges']['hdbscan']).lstrip('✓✗ ') or '—')
        lines += ['## Vidéo : HGP contre HDBSCAN, k = %d' % k, '',
                  '<picture>',
                  '  <source media="(prefers-color-scheme: dark)" srcset="%s_sombre_instant_cle.png">' % stem,
                  '  <img alt="%s" src="%s_clair_instant_cle.png">' % (alt.replace('"', '&quot;'), stem),
                  '</picture>', '',
                  'Vidéo de %d s, 1920 × 1080 : [thème sombre](%s_sombre.mp4) · [thème clair](%s_clair.mp4) ; image '
                  'finale : [sombre](%s_sombre_bilan.png) · [clair](%s_clair_bilan.png).' % (
                      round(res['timing']['duration']), stem, stem, stem, stem), '',
                  'Mêmes %d points, même ordre k = %d : à gauche la hiérarchie de points HGP de `morsehgp3D_v11` '
                  '(Hʳₖ₊₁), à droite l\'arbre de HDBSCAN (scikit-learn 1.7.2, `min_samples` = %d). Le niveau r croît '
                  'pour les deux à la fois et s\'arrête à chaque événement des groupes qui suivent les objets (mêmes '
                  'textes que les bandeaux de la vidéo) :' % (res['sites'], k, k), '',
                  '| r | HGP | HDBSCAN |', '| --- | --- | --- |']
        for p in res['timing']['pauses']:
            row = [' ; '.join(text(b) for b in p['badges'][side]) for side in ('hgp', 'hdbscan')]
            lines.append('| %s | %s | %s |' % (cm(p['r']), row[0], row[1]))
        best = res['methods']
        lines += ['', 'Meilleur IoU de chaque objet : HGP %s ; HDBSCAN %s. Légende, convention de niveau et contrôle '
                  'des calculs : [README de `demos/`](../../README.md#%s) ; nombres : [`resultats_duel_k%d.json`]'
                  '(resultats_duel_k%d.json).' % (
                      ', '.join('%s %s' % ('ABC'[o], fr(v)) for o, v in enumerate(best['hgp']['best'])),
                      ', '.join('%s %s' % ('ABC'[o], fr(v)) for o, v in enumerate(best['hdbscan']['best'])),
                      GUIDE, k, k), '']
    lines.append(END)
    return lines


def fr(v):
    return ('%.2f' % v).replace('.', ',')


def category_section(folder):
    """Lignes Markdown de la liste des vidéos d'une catégorie ; [] si aucun bout n'en a."""
    rows = []
    for where in sorted(p for p in folder.iterdir() if p.is_dir() and p.name.startswith('bout_')):
        spec = json.loads((where / 'bout.json').read_text(encoding='utf-8'))
        entry = spec['bouts'][0]
        for k, res in videos(where):
            stem = '%s/%s_hgp_hdbscan_k%d' % (where.name, where.name, k)
            key = key_pause(res)
            rows.append('| [%s/%s](%s/README.md) | %d | %d s | [sombre](%s_sombre.mp4) · [clair](%s_clair.mp4) | '
                        'HGP : %s ; HDBSCAN : %s |' % (
                            entry['seq'], entry['frame'], where.name, k, round(res['timing']['duration']), stem, stem,
                            ' ; '.join(text(b) for b in key['badges']['hgp']).lstrip('✓✗ '),
                            ' ; '.join(text(b) for b in key['badges']['hdbscan']).lstrip('✓✗ ')))
    if not rows:
        return []
    return [BEGIN, '## Vidéos : HGP contre HDBSCAN', '',
            'Une vidéo par bout, à l\'ordre montré, en thème sombre et clair : les deux hiérarchies côte à côte, au même '
            'ordre k, le niveau r commun croissant, une pause à chaque événement. Lecture : [README de `demos/`]'
            '(../README.md#%s).' % GUIDE, '',
            '| bout (trame) | k | durée | vidéo | instant clé |', '| --- | --- | --- | --- | --- |'] + rows + ['', END]


def replace_section(text_in, lines, before):
    """Remplace la section entre les marqueurs, ou l'insère avant la ligne `before` (à la fin si absente)."""
    body = '\n'.join(lines)
    if BEGIN in text_in and not lines:  # plus de vidéo : la section disparaît
        start, end = text_in.index(BEGIN), text_in.index(END) + len(END)
        return text_in[:start] + text_in[end:].lstrip('\n')
    if BEGIN in text_in:
        start, end = text_in.index(BEGIN), text_in.index(END) + len(END)
        tail = text_in[end:].lstrip('\n')
        return text_in[:start] + body + ('\n\n' + tail if tail else '\n')
    if not lines:
        return text_in
    at = text_in.find('\n' + before)
    if at < 0:
        return text_in.rstrip('\n') + '\n\n' + body + '\n'
    return text_in[:at + 1] + body + '\n\n' + text_in[at + 1:]


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    for category in map(Path, sys.argv[1:]):
        for where in sorted(p for p in category.iterdir() if p.is_dir() and p.name.startswith('bout_')):
            readme = where / 'README.md'
            new = replace_section(readme.read_text(encoding='utf-8'), bout_section(where), '## Images')
            readme.write_text(new, encoding='utf-8')
        readme = category / 'README.md'
        new = replace_section(readme.read_text(encoding='utf-8'), category_section(category),
                              'Critères, mesure et légende des images')
        readme.write_text(new, encoding='utf-8')
        print(category, 'README mis à jour')


if __name__ == '__main__':
    main()
