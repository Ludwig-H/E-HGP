#!/usr/bin/env python3
"""Tableaux Markdown des juges q2/q3 depuis un dossier de sorties de campagne (aucune assertion)."""
import re
import sys
from pathlib import Path


def th(x):
    return f"{int(x):,}".replace(',', ' ')


def fields(line):
    return dict(re.findall(r'(\w[\w.]*)=([^\s]+)', line))


def summary_line(path):
    lines = path.read_text().splitlines()
    status = next((l for l in lines if l.startswith('exit=')), 'exit=absent')
    summ = next((l for l in reversed(lines) if ' n=' in l and 'kmax=' in l), '')
    return fields(summ), fields(status), sum(1 for l in lines if 'LONG_SITE' in l)


def main(d):
    d = Path(d)
    print('### Portes du juge (codes attendus)\n')
    print('| cas | code | attendu |')
    print('| --- | ---: | ---: |')
    for name in ['q3_fixture_eq_compare', 'q3_fixture_eq_overprune', 'q3_fixture_eq_level', 'q3_fixture_eq_shell_dup',
                 'q3_fixture_eq_key', 'q2_lidar_s02_8000_k5_level', 'q2_lidar_s02_8000_k5_shell_dup',
                 'q2_lidar_s02_8000_k5_key', 'q3_lidar_s00_8000_k10_compare_isolated',
                 'q3_lidar_s00_8000_k10_overprune_isolated', 'q3_lidar_s02_8000_k10_compare_isolated',
                 'q3_lidar_s02_8000_k10_overprune_isolated']:
        p = d / f'{name}.txt'
        if not p.exists():
            print(f'| {name} | absent | — |')
            continue
        f, st, _ = summary_line(p)
        extra = (f" (désaccords d'élagage : {f.get('prune_disagreements', '—')}, incidences : {f.get('incidences', '—')}, "
                 f"dont ≥ 1 600 unités : {f.get('len_ge1600', '—')})") if 'compare' in name or 'overprune' in name else ''
        print(f"| `{name}`{extra} | {st.get('exit', '?')} | {st.get('expected', '?')} |")
    print('\n### Juge q3\n')
    print('| cas | Kmax | sites | incidences | trouvées | manquantes | recoupements faux | EXTRA | clés distinctes | clés q2 | clés p=Kmax−2 (arité 3) / population | ≥ 1 600 unités (dont p=Kmax−2) | code |')
    print('| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |')
    for p in sorted(d.glob('q3_*.txt')):
        if 'fixture' in p.name or 'isolated' in p.name:
            continue
        f, st, nlong = summary_line(p)
        if not f:
            print(f'| {p.stem} | — | — | — | — | — | — | — | — | — | — | — | {st.get("exit", "?")} |')
            continue
        print(f"| {p.stem} | {f['kmax']} | {f['sampled_sites']} | {th(f['incidences'])} | {th(f['found'])} | {f['missing']} | "
              f"{f['cross_fail']} | {f['extra']} | {th(f['unique_keys'])} | {th(f.get('q2_keys', 0))} | "
              f"{th(f['top_keys'])} / {th(f['top_population'])} | {th(f['len_ge1600'])} ({th(f['top_ge1600'])}) | "
              f"{st.get('exit', '?')} |")
    print('\n### Juge q2\n')
    print('| cas | Kmax | sites | incidences | trouvées | manquantes | recoupements faux | EXTRA | clés distinctes | clés p=Kmax−1 (arité 2) / population | code |')
    print('| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |')
    for p in sorted(d.glob('q2_*.txt')):
        if 'level' in p.name or 'shell_dup' in p.name or p.name.endswith('_key.txt'):
            continue
        f, st, _ = summary_line(p)
        if not f:
            print(f'| {p.stem} | — | — | — | — | — | — | — | — | — | {st.get("exit", "?")} |')
            continue
        print(f"| {p.stem} | {f['kmax']} | {f['sampled_sites']} | {th(f['incidences'])} | {th(f['found'])} | {f['missing']} | "
              f"{f['cross_fail']} | {f['extra']} | {th(f['unique_keys'])} | {th(f['top_keys'])} / {th(f['top_population'])} | "
              f"{st.get('exit', '?')} |")


def gates(d):
    """Tableau generique des portes : chaque cas du dossier, code et attendu, champs CRL (v7)."""
    d = Path(d)
    print('| cas | code | attendu | désaccords (dont CRL) | incidences CRL / triangles CRL distincts | clé CRL retirée déclarée manquante | marqueur |')
    print('| --- | ---: | ---: | ---: | ---: | --- | --- |')
    for p in sorted(d.glob('q*_*.txt')):
        f, st, _ = summary_line(p)
        text = p.read_text()
        marker = 'PRUNE_DISAGREES_CRL' if 'PRUNE_DISAGREES_CRL' in text else 'PRUNE_DISAGREES' if 'PRUNE_DISAGREES' in text \
            else 'MISSING' if 'MISSING a=' in text else '—'
        dis = f"{f.get('prune_disagreements', '—')} ({f.get('crl_disagreements', '—')})" if f else '—'
        crl = f"{f.get('crl', '—')} / {f.get('crl_unique', '—')}" if f else '—'
        tk = {'1': 'oui', '0': 'non'}.get(f.get('crl_target_killed', ''), '—') if f else '—'
        print(f"| `{p.stem}` | {st.get('exit', '?')} | {st.get('expected', '?')} | {dis} | {crl} | {tk} | {marker} |")


if __name__ == '__main__':
    if len(sys.argv) > 2 and sys.argv[1] == '--gates':
        gates(sys.argv[2])
        sys.exit(0)
    main(sys.argv[1] if len(sys.argv) > 1 else 'results/judges_v5')
