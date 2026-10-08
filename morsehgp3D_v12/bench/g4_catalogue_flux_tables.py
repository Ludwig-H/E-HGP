#!/usr/bin/env python3
"""Tableaux Markdown du rapport de la session G4 T2-d-C (publication seulement : le verdict est celui du juge,
g4_catalogue_flux_judge.py). Chaque execution est relue par le lecteur strict (g4_catalogue_flux_lecteur.py) avec la
commande de son etape ; une execution hors commande est publiee << - >>, jamais comme une mesure nulle. Bibliotheque
standard, Python 3.10 nu, aucun assert.
"""
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import g4_catalogue_flux_judge as J  # noqa: E402
import g4_catalogue_flux_lecteur as L  # noqa: E402

STAGE_NAMES = tuple(J.STAGES)


def median_ms(values):
    values = sorted(v for v in values if L.is_int(v))
    if not values:
        return '-'
    n = len(values)
    middle = values[n // 2] if n % 2 else (values[n // 2 - 1] + values[n // 2]) / 2
    return '%.3f' % (middle / 1e6)


def median_mb(values):
    values = sorted(v for v in values if L.is_int(v))
    return '%.1f' % (values[len(values) // 2] / 1e6) if values else '-'


def warm_passes(entries, spec_of, **fields):
    """Passes 2..P des executions conformes de l'etape qui portent ces champs, avec leur ligne << sorties >>."""
    out = []
    for e in entries if isinstance(entries, list) else []:
        if not isinstance(e, dict) or any(e.get(k) != v for k, v in fields.items()):
            continue
        state, _, parsed = L.read_catalogue(e.get('run'), spec_of(e))
        if state == 'ok':
            sorties = parsed['sorties'] or [None] * len(parsed['passes'])
            out += list(zip(parsed['passes'], sorties))[1:]
    return out


def stage_value(name, row, sorties):
    if name == 'sorties':
        return sorties['outputs_ns'] if sorties else None
    fields = dict(row['diagnostics'], transfer_ns=row['device']['transfer_ns'], publish_ns=row['device']['publish_ns'])
    return sum(fields[k] for k in J.STAGES[name])


def verdict_lines(report):
    verdict = report.get('verdict') or {}
    lines = ['# T2-d-C : transferts et publication du catalogue sur G4', '',
             'Verdict de REGLE_T2D_C : **%s**.' % verdict.get('verdict'), '']
    for kind, label in (('refused', 'refus'), ('rejected', 'rejet')):
        lines += ['- %s : %s' % (label, item) for item in verdict.get(kind) or []]
    mutant = (verdict.get('stats') or {}).get('mutant')
    lines += ['', 'Mutant appareil flux_sans_attente_appareil : %s.' % (mutant or 'non juge'), '',
              '## Leviers (moyenne geometrique des rapports par tour, IC 95 % ; decision sur les bornes non '
              'arrondies)', '', '| levier | de | vers | ng00 | ng01 | ng02 | verdict |',
              '| --- | --- | --- | ---: | ---: | ---: | --- |']
    for name, lever in ((verdict.get('stats') or {}).get('levers') or {}).items():
        cells = [(lever['frames'].get(f) or {}).get('affiche', '-') + ('' if (lever['frames'].get(f) or {}).get(
            'decisive', True) else ' (publie)') for f in J.FRAMES]
        lines.append('| %s | %s | %s | %s | %s |' % (name, lever['from'], lever['to'], ' | '.join(cells),
                                                    lever['verdict']))
    return lines


def campaign_lines(report):
    steps, opts = report.get('steps') or {}, report.get('options') or {}
    threads, passes = opts.get('threads'), opts.get('passes')
    lines = ['', '## Etage C a chaud, K5 (mediane des passes 2..P de tous les processus, ms ; transferts : partition '
             'du mur, pas une mesure du DMA ; sorties : reservation et premier toucher)', '',
             '| trame | bras | total | ' + ' | '.join(STAGE_NAMES) + ' | epinglee (Mo) | pic (Mo) |',
             '| --- | --- | ---: |' + ' ---: |' * (len(STAGE_NAMES) + 2)]
    for frame in J.FRAMES:
        for arm in J.ARMS:
            rows = warm_passes(steps.get('campaign'), lambda e: L.catalogue_spec(
                'device', 5, threads, passes, sorties=e.get('arm') not in J.HISTORICAL), frame=frame, arm=arm)
            parts = [median_ms([stage_value(n, r, s) for r, s in rows]) for n in STAGE_NAMES]
            lines.append('| %s | %s | %s | %s | %s | %s |' % (
                frame, arm, median_ms([r['wall_ns'] for r, _ in rows]), ' | '.join(parts),
                median_mb([r['device']['pinned_bytes'] for r, _ in rows]),
                median_mb([r['diagnostics']['peak_bytes'] for r, _ in rows])))
    return lines


def information_lines(report):
    steps, opts = report.get('steps') or {}, report.get('options') or {}
    threads, passes = opts.get('threads'), opts.get('passes')
    info = steps.get('informations') or {}
    lines = ['', '## Informations (ne decident rien)', '', '| mesure | trame | bras | mediane (ms) |',
             '| --- | --- | --- | ---: |']
    for key, k, count in (('k10', 10, 5), ('cache', 5, passes)):
        for frame in J.FRAMES:
            for arm in ('avant', 'apres') if key == 'k10' else ('apres', 'apres_cache'):
                rows = warm_passes(info.get(key), lambda e, k=k, count=count: L.catalogue_spec(
                    'device', k, threads, count, sorties=e.get('arm') not in J.HISTORICAL,
                    cache=4 << 30 if e.get('arm') == 'apres_cache' else 0), frame=frame, arm=arm)
                lines.append('| %s | %s | %s | %s |' % (key, frame, arm, median_ms([r['wall_ns'] for r, _ in rows])))
    for frame in J.FRAMES:
        for arm in ('avant', 'apres'):
            walls, c_ns = [], []
            for e in info.get('full') or []:
                if isinstance(e, dict) and e.get('frame') == frame and e.get('arm') == arm:
                    state, _, _, w, c = L.read_full(e.get('run'), frame, 5, threads, passes)
                    walls += w[1:] if state == 'ok' else []
                    c_ns += c[1:] if state == 'ok' else []
            lines.append('| mur FULL (dont C) | %s | %s | %s (%s) |' % (frame, arm, median_ms(walls), median_ms(c_ns)))
    v12 = info.get('v12set')
    if isinstance(v12, list):
        for frame in sorted({e.get('frame') for e in v12 if isinstance(e, dict)}):
            cells = [median_ms([r['wall_ns'] for r, _ in warm_passes(v12, lambda e: L.catalogue_spec(
                'device', 5, threads, 6, sorties=e.get('arm') not in J.HISTORICAL), frame=frame, arm=arm)])
                for arm in ('avant', 'apres')]
            lines.append('| v12set | %s | avant / apres | %s / %s |' % (frame, cells[0], cells[1]))
    skipped = info.get('non_joues') or []
    lines += ['', 'Informations non jouees (echeance) : %d processus.' % len(skipped)]
    return lines


def tables(report):
    return '\n'.join(verdict_lines(report) + campaign_lines(report) + information_lines(report)) + '\n'
