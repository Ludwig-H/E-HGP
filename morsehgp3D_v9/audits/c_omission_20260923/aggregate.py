#!/usr/bin/env python3
"""Agrège les sorties de omission_tower_probe (results/*.txt) en tableaux Markdown.

Usage : python3 aggregate.py results/ > results/TABLEAUX.md
Aucune assertion : une ligne manquante ou un code de sortie non nul est
écrit dans le tableau, jamais ignoré.
"""
import re
import sys
from pathlib import Path

HEAD = re.compile(r'^(\S+) n=(\d+) kmax=(\d+) balls=(\d+) regular=(\d+) extended=(\d+) top_within=(\d+) '
                  r'euler_visible_regular=(\d+) joint_blind_regular=(\d+) extended_top_over=(\d+)')
STRAT = re.compile(r'^(\S+) stratum top_within=(\d) regular=(\d) q=(\d) population=(\d+) sampled=(\d+) refused=(\d+)'
                   r'(?: accepted_digest_changed=(\d+) accepted_digest_same=(\d+))?(.*)$')


def th(x):
    return f"{int(x):,}".replace(',', ' ')


def fmt_pct(a, b):
    return f"{100.0 * a / b:.1f} %".replace('.', ',') if b else '—'


def main(directory):
    files = sorted(Path(directory).glob('*.txt'))
    pop_rows, samp = [], {}
    for f in files:
        text = f.read_text().splitlines()
        exit_code = next((l.split('=', 1)[1] for l in text if l.startswith('exit=')), None)
        case = f.stem
        if exit_code is None:
            pop_rows.append(f"| {case} | — | — | — | — | — | en cours, non agrégé |")
            continue
        head = next((HEAD.match(l) for l in text if HEAD.match(l)), None)
        if head is None:
            pop_rows.append(f"| {case} | — | — | — | — | — | sortie {exit_code} |")
            continue
        _, n, kmax, balls, reg, ext, within, ev, blind, ext_over = head.groups()
        balls = int(balls)
        pop_rows.append(f"| {case} | {kmax} | {th(balls)} | {fmt_pct(int(within), balls)} | "
                        f"{fmt_pct(int(ev), balls)} | {th(blind)} ({fmt_pct(int(blind), balls)}) | "
                        f"{int(ext)} (dont {ext_over} d'ordre haut > Kmax) ; sortie {exit_code} |")
        for l in text:
            m = STRAT.match(l)
            if not m:
                continue
            _, within_s, regular, q, population, sampled, refused, changed, same, reasons = m.groups()
            key = (int(kmax), within_s == '1', regular == '1', int(q))
            acc = samp.setdefault(key, [0, 0, 0, set(), None, None])
            acc[0] += int(population)
            acc[1] += int(sampled)
            acc[2] += int(refused)
            if changed is not None:
                acc[4] = (acc[4] or 0) + int(changed)
                acc[5] = (acc[5] or 0) + int(same)
            for part in reasons.split():
                acc[3].add(part.split('=')[0])
    print('## Populations du catalogue\n')
    print('| cas | Kmax | boules | ordre haut ≤ Kmax | vues par Euler (régulières) | angle mort conjoint (régulières) | coquilles étendues |')
    print('| --- | ---: | ---: | ---: | ---: | ---: | --- |')
    print('\n'.join(pop_rows))
    print('\n## Retraits isolés, sommés sur les cas\n')
    print('| Kmax | ordre haut p+u ≤ Kmax | coquille | q | population | retraits | refusés | acceptés, condensé changé | acceptés, condensé égal | issues |')
    print('| ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |')
    for key in sorted(samp):
        kmax, within, regular, q = key
        pop, sampled, refused, reasons, changed, same = samp[key]
        ch = '—' if changed is None else str(changed)
        sa = '—' if same is None else str(same)
        print(f"| {kmax} | {'oui' if within else 'non'} | {'régulière' if regular else 'étendue'} | {q} | "
              f"{th(pop)} | {sampled} | {refused} | {ch} | {sa} | {', '.join(sorted(reasons))} |")


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'results')
