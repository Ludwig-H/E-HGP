#!/usr/bin/env python3
"""Tables Markdown des jeux prepares (pour RAPPORT.md / DONNEES.md), lues dans les manifestes.

    python3 -S summarize.py DATA_DIR > tables.md
"""
from __future__ import annotations

import json
import sys
from pathlib import Path


def fmt(n) -> str:
    return '{:,}'.format(int(n)).replace(',', ' ')


def scenes_table(manifest: dict) -> list:
    rows = ['| Scène | Retours | Positions distinctes | Doublons au mm | Étendue (m) | Bits | '
            'SHA-256 `<nom>.u32le` (16) | SHA-256 `.distinct` (16) |',
            '| --- | ---: | ---: | ---: | --- | ---: | --- | --- |']
    for c in manifest['cases']:
        ext = ' × '.join('%.1f' % (e / 1000) for e in c['extent_mm'])
        dup = c['count'] - c.get('n_distinct', c['count'])
        rows.append('| `%s` | %s | %s | %s | %s | %d | `%s` | %s |' % (
            c['name'], fmt(c['count']), fmt(c.get('n_distinct', c['count'])), fmt(dup), ext, c['bits_needed'],
            c['sha256'][:16], ('`%s`' % c['distinct']['sha256'][:16]) if c.get('distinct') else '—'))
    return rows


def crops_table(manifest: dict) -> list:
    if not manifest.get('crops'):
        return []
    rows = ['| Découpe | Sites distincts | Retours couverts | Côté du carré (m) | Bits | SHA-256 (16) |',
            '| --- | ---: | ---: | ---: | ---: | --- |']
    for c in manifest['crops']:
        side = 2 * c['crop']['radius_chebyshev_mm'] / 1000
        rows.append('| `%s` | %s | %s | %.1f | %d | `%s` |' % (c['name'], fmt(c['count']), fmt(c.get('returns',
                    c['count'])), side, c['bits_needed'], c['sha256'][:16]))
    return rows


def kitti_tables(root: Path) -> list:
    path = root / 'semantickitti' / 'manifest_v12set.json'
    if not path.is_file():
        return []
    m = json.loads(path.read_text())
    every = json.loads((root / 'semantickitti' / 'manifest.json').read_text())
    out = ['### semantickitti', '']
    for label, man in (('toutes les trames locales', every), ('sélection v12set', m)):
        s = man['summary']
        out.append('- %s : %d trames, séquences %s (%s) ; sites : min %s, médiane %s, max %s ; %d trames au-dessus '
                   'de 60 000 sites, %d sous 30 000' % (label, s['frames'], ', '.join(s['sequences']),
                                    ', '.join('%s : %d' % kv for kv in sorted(s['per_sequence'].items())),
                                    fmt(s['sites_min']), fmt(s['sites_median']), fmt(s['sites_max']),
                                    s['frames_over_60000'], s['frames_under_30000']))
    out += ['', '| Trame | Retours bruts | Sans sol (retours) | Sites | Bits | SHA-256 `.u32le` (16) | SHA-256 ids (16) |',
            '| --- | ---: | ---: | ---: | ---: | --- | --- |']
    for c in m['cases']:
        p = c['provenance']
        out.append('| `%s` | %s | %s | %s | %d | `%s` | `%s` |' % (
            c['name'], fmt(p['n_raw_returns']), fmt(p['n_without_ground_returns']), fmt(c['count']),
            c['bits_needed'], c['sha256'][:16], c['ids_sha256'][:16]))
    out.append('')
    return out


def small_tables(root: Path) -> list:
    path = root / 'small' / 'manifest.json'
    if not path.is_file():
        return []
    m = json.loads(path.read_text())
    out = ['### small', '', '| Sous-famille | Nuages | Sites (min - max) | Bits (min - max) |', '| --- | ---: | --- | --- |']
    subs = sorted({c['subfamily'] for c in m['cases']})
    for sub in subs:
        cs = [c for c in m['cases'] if c['subfamily'] == sub]
        out.append('| %s | %d | %s - %s | %d - %d |' % (sub, len(cs), fmt(min(c['count'] for c in cs)),
                                                    fmt(max(c['count'] for c in cs)),
                                                    min(c['bits_needed'] for c in cs),
                                                    max(c['bits_needed'] for c in cs)))
    out.append('')
    return out


def main(argv) -> int:
    root = Path(argv[0])
    out = kitti_tables(root) + small_tables(root)
    for path in sorted(root.glob('*/manifest.json')):
        manifest = json.loads(path.read_text())
        if manifest.get('family') != 'multi_millions':
            continue
        out.append('### %s' % path.parent.name)
        out.append('')
        out += scenes_table(manifest)
        out.append('')
        out += crops_table(manifest)
        out.append('')
    print('\n'.join(out))
    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))
